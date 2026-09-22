// x64dbg MCP Bridge Plugin (C API rewrite for MinGW compatibility)
// Exposes x64dbg/x32dbg functionality over TCP localhost:27042 for MCP integration.
// Uses ONLY C-style bridge functions to avoid MSVC/GCC ABI incompatibility.

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Bridge API (C-style, compatible with MinGW)
// On x86, MinGW and MSVC have incompatible import naming conventions.
// Disable __declspec(dllimport) and link directly instead.
#ifdef _WIN64
#include "../pluginsdk/bridgemain.h"
#include "../pluginsdk/_plugins.h"
#else
#define BRIDGE_IMPEXP
#define PLUG_IMPEXP
#include "../pluginsdk/bridgemain.h"
#include "../pluginsdk/_plugins.h"
#undef BRIDGE_IMPEXP
#undef PLUG_IMPEXP
#endif

#define PLUGIN_NAME    "x64dbg-mcp-bridge"
#define PLUGIN_VERSION 1
#ifndef SERVER_PORT
#ifdef _WIN64
#define SERVER_PORT 27042
#else
#define SERVER_PORT 27043
#endif
#endif

// Architecture-specific format for duint (hex without prefix for x64dbg commands)
#ifdef _WIN64
#define DUFMT  "%llX"
#define DUFMTX "0x%llX"
#else
#define DUFMT  "%lX"
#define DUFMTX "0x%lX"
#endif

static int               g_pluginHandle = -1;
static SOCKET            g_listenSock   = INVALID_SOCKET;
static std::thread       g_serverThread;
static std::atomic<bool> g_running{false};

// ── Minimal JSON helpers ────────────────────────────────────────────────────

static std::string jsonGetStr(const std::string& json, const char* key)
{
    std::string needle = std::string("\"") + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        pos++;
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos+1 < json.size()) { pos++; }
            val += json[pos++];
        }
        return val;
    }
    std::string val;
    while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != '\n') {
        val += json[pos++];
    }
    while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) val.pop_back();
    return val;
}

static unsigned long long jsonGetUint(const std::string& json, const char* key, unsigned long long def = 0)
{
    std::string s = jsonGetStr(json, key);
    if (s.empty()) return def;
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return (unsigned long long)strtoull(s.c_str() + 2, nullptr, 16);
    return (unsigned long long)strtoull(s.c_str(), nullptr, 10);
}

static std::string hexEncode(const unsigned char* data, size_t len)
{
    static const char h[] = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        out += h[(data[i] >> 4) & 0xF];
        out += h[data[i] & 0xF];
    }
    return out;
}

static std::vector<unsigned char> hexDecode(const std::string& hex)
{
    std::vector<unsigned char> out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned int byte = 0;
        sscanf(hex.c_str() + i, "%02x", &byte);
        out.push_back((unsigned char)byte);
    }
    return out;
}

// ── Command dispatcher ──────────────────────────────────────────────────────

static std::string handleRequest(const std::string& req)
{
    std::string cmd = jsonGetStr(req, "cmd");

    if (cmd == "ping") {
        bool dbg = DbgIsDebugging();
        bool run = DbgIsRunning();
        std::string j = "{\"ok\":true,\"name\":\"" PLUGIN_NAME "\",\"version\":1";
        j += ",\"debugging\":"; j += dbg ? "true" : "false";
        j += ",\"running\":";   j += run ? "true" : "false";
        j += "}\n";
        return j;
    }

    // ── Execution control (via DbgCmdExecDirect) ─────────────────────────
    if (cmd == "run") {
        DbgCmdExecDirect("run");
        return "{\"ok\":true}\n";
    }
    if (cmd == "pause") {
        DbgCmdExecDirect("pause");
        return "{\"ok\":true}\n";
    }
    if (cmd == "stop") {
        DbgCmdExecDirect("stop");
        return "{\"ok\":true}\n";
    }
    if (cmd == "stepin") {
        DbgCmdExecDirect("StepInto");
        _plugin_waituntilpaused();
        return "{\"ok\":true}\n";
    }
    if (cmd == "stepover") {
        DbgCmdExecDirect("StepOver");
        _plugin_waituntilpaused();
        return "{\"ok\":true}\n";
    }
    if (cmd == "stepout") {
        DbgCmdExecDirect("StepOut");
        _plugin_waituntilpaused();
        return "{\"ok\":true}\n";
    }

    // ── Breakpoints (via DbgCmdExecDirect) ───────────────────────────────
    if (cmd == "setbp") {
        duint addr = (duint)jsonGetUint(req, "addr");
        char buf[64];
        snprintf(buf, sizeof(buf), "bp " DUFMT, addr);
        // Format: "bp 0x%llx" (x64dbg uses hex format for bp command)
        // Actually, x64dbg accepts raw hex addresses without 0x prefix in some contexts
        // Use "SetBreakpoint <addr>" for reliable breakpoint setting
        bool ok = DbgCmdExecDirect(buf);
        return ok ? "{\"ok\":true}\n" : "{\"ok\":false,\"error\":\"setbp failed\"}\n";
    }
    if (cmd == "delbp") {
        duint addr = (duint)jsonGetUint(req, "addr");
        char buf[64];
        snprintf(buf, sizeof(buf), "bc " DUFMT, addr);
        DbgCmdExecDirect(buf);
        return "{\"ok\":true}\n";
    }
    if (cmd == "disablebp") {
        duint addr = (duint)jsonGetUint(req, "addr");
        char buf[64];
        snprintf(buf, sizeof(buf), "bpd " DUFMT, addr);
        DbgCmdExecDirect(buf);
        return "{\"ok\":true}\n";
    }
    if (cmd == "sethwbp") {
        duint addr = (duint)jsonGetUint(req, "addr");
        std::string typeStr = jsonGetStr(req, "type");
        char buf[128];
        if (typeStr == "access")
            snprintf(buf, sizeof(buf), "bphws " DUFMT ", r", addr);
        else if (typeStr == "write")
            snprintf(buf, sizeof(buf), "bphws " DUFMT ", w", addr);
        else
            snprintf(buf, sizeof(buf), "bphws " DUFMT, addr);
        DbgCmdExecDirect(buf);
        return "{\"ok\":true}\n";
    }

    // ── Memory (C bridge API) ────────────────────────────────────────────
    if (cmd == "readmem") {
        duint addr = (duint)jsonGetUint(req, "addr");
        duint size = (duint)jsonGetUint(req, "size", 64);
        if (size > 65536) size = 65536;
        std::vector<unsigned char> buf(size);
        bool ok = DbgMemRead(addr, buf.data(), size);
        if (!ok) return "{\"ok\":false,\"error\":\"read failed\"}\n";
        std::string hex = hexEncode(buf.data(), size);
        return "{\"ok\":true,\"data\":\"" + hex + "\",\"size\":" + std::to_string(size) + "}\n";
    }
    if (cmd == "writemem") {
        duint addr   = (duint)jsonGetUint(req, "addr");
        std::string hex = jsonGetStr(req, "data");
        auto bytes = hexDecode(hex);
        if (bytes.empty()) return "{\"ok\":false,\"error\":\"no data\"}\n";
        bool ok = DbgMemWrite(addr, bytes.data(), bytes.size());
        return ok ? ("{\"ok\":true,\"written\":" + std::to_string(bytes.size()) + "}\n")
                  : "{\"ok\":false,\"error\":\"write failed\"}\n";
    }
    if (cmd == "validptr") {
        duint addr = (duint)jsonGetUint(req, "addr");
        bool ok = DbgMemIsValidReadPtr(addr);
        return ok ? "{\"ok\":true,\"valid\":true}\n" : "{\"ok\":true,\"valid\":false}\n";
    }

    // ── Registers (DbgGetRegDumpEx + DbgCmdExecDirect) ───────────────────
    if (cmd == "getregs") {
#ifdef _WIN64
        REGDUMP_AVX512 rd = {};
#else
        // x86 uses smaller struct; REGDUMP_AVX512 works but we only read base fields
        REGDUMP_AVX512 rd = {};
#endif
        if (!DbgGetRegDumpEx(&rd, sizeof(rd)))
            return "{\"ok\":false,\"error\":\"getregs failed\"}\n";

        auto& rc = rd.regcontext;
        std::string j = "{\"ok\":true,\"regs\":{";
#ifdef _WIN64
        j += "\"rax\":"  + std::to_string(rc.cax);
        j += ",\"rbx\":" + std::to_string(rc.cbx);
        j += ",\"rcx\":" + std::to_string(rc.ccx);
        j += ",\"rdx\":" + std::to_string(rc.cdx);
        j += ",\"rsi\":" + std::to_string(rc.csi);
        j += ",\"rdi\":" + std::to_string(rc.cdi);
        j += ",\"rbp\":" + std::to_string(rc.cbp);
        j += ",\"rsp\":" + std::to_string(rc.csp);
        j += ",\"rip\":" + std::to_string(rc.cip);
        j += ",\"r8\":"  + std::to_string(rc.r8);
        j += ",\"r9\":"  + std::to_string(rc.r9);
        j += ",\"r10\":" + std::to_string(rc.r10);
        j += ",\"r11\":" + std::to_string(rc.r11);
        j += ",\"r12\":" + std::to_string(rc.r12);
        j += ",\"r13\":" + std::to_string(rc.r13);
        j += ",\"r14\":" + std::to_string(rc.r14);
        j += ",\"r15\":" + std::to_string(rc.r15);
#else
        j += "\"eax\":"  + std::to_string(rc.cax);
        j += ",\"ebx\":" + std::to_string(rc.cbx);
        j += ",\"ecx\":" + std::to_string(rc.ccx);
        j += ",\"edx\":" + std::to_string(rc.cdx);
        j += ",\"esi\":" + std::to_string(rc.csi);
        j += ",\"edi\":" + std::to_string(rc.cdi);
        j += ",\"ebp\":" + std::to_string(rc.cbp);
        j += ",\"esp\":" + std::to_string(rc.csp);
        j += ",\"eip\":" + std::to_string(rc.cip);
#endif
        j += ",\"eflags\":" + std::to_string(rc.eflags);
        j += ",\"dr0\":" + std::to_string(rc.dr0);
        j += ",\"dr1\":" + std::to_string(rc.dr1);
        j += ",\"dr2\":" + std::to_string(rc.dr2);
        j += ",\"dr3\":" + std::to_string(rc.dr3);
        j += ",\"dr6\":" + std::to_string(rc.dr6);
        j += ",\"dr7\":" + std::to_string(rc.dr7);
        j += "}}\n";
        return j;
    }

    if (cmd == "setreg") {
        std::string name  = jsonGetStr(req, "reg");
        duint value = (duint)jsonGetUint(req, "value");
        char buf[128];
        snprintf(buf, sizeof(buf), "setreg %s " DUFMT, name.c_str(), value);
        DbgCmdExecDirect(buf);
        return "{\"ok\":true}\n";
    }

    // ── Expression evaluator (DbgValFromString) ──────────────────────────
    if (cmd == "eval") {
        std::string expr = jsonGetStr(req, "expr");
        duint value = DbgValFromString(expr.c_str());
        return "{\"ok\":true,\"value\":" + std::to_string(value) + "}\n";
    }

    // ── Run x64dbg command ───────────────────────────────────────────────
    if (cmd == "exec") {
        std::string xcmd = jsonGetStr(req, "xcmd");
        bool ok = DbgCmdExecDirect(xcmd.c_str());
        // For session-starting commands (attach/init/run-to-...), the debuggee
        // is still resuming when DbgCmdExecDirect returns, so register/memory
        // reads would fail. Wait (bounded) until it is attached AND paused so
        // subsequent readmem/getregs work. This fixes the attach-after flow.
        bool waitPause = (xcmd.rfind("attach", 0) == 0) ||
                         (xcmd.rfind("Attach", 0) == 0) ||
                         (xcmd.rfind("init", 0) == 0)   ||
                         (xcmd.rfind("InitDebug", 0) == 0);
        if (ok && waitPause) {
            for (int i = 0; i < 200; i++) {            // up to ~10 s
                if (DbgIsDebugging() && !DbgIsRunning()) break;
                Sleep(50);
            }
        }
        bool dbg = DbgIsDebugging();
        bool run = DbgIsRunning();
        std::string j = "{\"ok\":"; j += ok ? "true" : "false";
        j += ",\"debugging\":"; j += dbg ? "true" : "false";
        j += ",\"running\":";   j += run ? "true" : "false";
        j += "}\n";
        return j;
    }

    // ── Wait until the debuggee is paused (bounded) ──────────────────────
    // Use after "run" once a breakpoint is expected to fire, so we can read
    // state exactly when the target stops (e.g. at the decrypt routine).
    if (cmd == "waitpaused") {
        int timeoutMs = (int)jsonGetUint(req, "timeout", 15000);
        int waited = 0;
        while (waited < timeoutMs) {
            if (DbgIsDebugging() && !DbgIsRunning()) break;
            Sleep(50); waited += 50;
        }
        bool dbg = DbgIsDebugging();
        bool run = DbgIsRunning();
        std::string j = "{\"ok\":true,\"debugging\":"; j += dbg ? "true" : "false";
        j += ",\"running\":"; j += run ? "true" : "false";
        j += ",\"paused\":";  j += (dbg && !run) ? "true" : "false";
        j += "}\n";
        return j;
    }

    // ── Disassemble ──────────────────────────────────────────────────────
    if (cmd == "disasm") {
        duint addr  = (duint)jsonGetUint(req, "addr");
        int   count = (int)jsonGetUint(req, "count", 10);
        if (count > 100) count = 100;

        std::string j = "{\"ok\":true,\"instructions\":[";
        for (int i = 0; i < count; i++) {
            BASIC_INSTRUCTION_INFO info = {};
            DbgDisasmFastAt(addr, &info);
            if (info.size <= 0) break;

            // Read raw bytes
            std::vector<unsigned char> raw((size_t)info.size);
            DbgMemRead(addr, raw.data(), (duint)raw.size());
            std::string hex = hexEncode(raw.data(), raw.size());

            // Escape the instruction string
            std::string instr;
            for (char c : std::string(info.instruction)) {
                if (c == '"') instr += "\\\"";
                else if (c == '\\') instr += "\\\\";
                else instr += c;
            }

            if (i > 0) j += ",";
            j += "{\"addr\":" + std::to_string(addr);
            j += ",\"bytes\":\"" + hex + "\"";
            j += ",\"text\":\"" + instr + "\"";
            j += ",\"size\":" + std::to_string(info.size) + "}";

            addr += info.size;
        }
        j += "]}\n";
        return j;
    }

    // ── Assemble ─────────────────────────────────────────────────────────
    if (cmd == "assemble") {
        duint addr  = (duint)jsonGetUint(req, "addr");
        std::string instr = jsonGetStr(req, "instr");
        bool ok = DbgAssembleAt(addr, instr.c_str());
        return ok ? "{\"ok\":true}\n" : "{\"ok\":false,\"error\":\"assemble failed\"}\n";
    }

    // ── Modules ──────────────────────────────────────────────────────────
    if (cmd == "getmodules") {
        // Use DbgFunctions()->ModNameFromAddr to scan for modules
        // This is a workaround since there's no direct C API for listing all modules.
        // We try to find executable images by scanning common patterns.
        // For a complete module list, use dbg_exec with "savemodlist" or similar.
        // For now, return a basic result.
        return "{\"ok\":true,\"modules\":[],\"note\":\"use dbg_exec command: modlist\"}\n";
    }

    // ── Symbol lookup (DbgValFromString works for module!api syntax) ─────
    if (cmd == "getprocaddr") {
        std::string module = jsonGetStr(req, "module");
        std::string api    = jsonGetStr(req, "api");
        std::string expr   = module + ":" + api;
        duint addr = DbgValFromString(expr.c_str());
        if (addr == 0) return "{\"ok\":false,\"error\":\"not found\"}\n";
        return "{\"ok\":true,\"addr\":" + std::to_string(addr) + "}\n";
    }

    if (cmd == "resolvelabel") {
        std::string label = jsonGetStr(req, "label");
        duint addr = DbgValFromString(label.c_str());
        if (addr == 0) return "{\"ok\":false,\"error\":\"not found\"}\n";
        return "{\"ok\":true,\"addr\":" + std::to_string(addr) + "}\n";
    }

    // ── Memory info (DbgMemFindBaseAddr) ────────────────────────────────
    if (cmd == "meminfo") {
        duint addr = (duint)jsonGetUint(req, "addr");
        duint size = 0;
        duint base = DbgMemFindBaseAddr(addr, &size);
        return "{\"ok\":true,\"base\":" + std::to_string(base)
             + ",\"size\":" + std::to_string(size)
             + ",\"protect\":0}\n";
    }

    return "{\"ok\":false,\"error\":\"unknown cmd: " + cmd + "\"}\n";
}

// ── TCP server thread ───────────────────────────────────────────────────────

static void serverLoop()
{
    WSADATA wsa = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;

    g_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSock == INVALID_SOCKET) { WSACleanup(); return; }

    int opt = 1;
    setsockopt(g_listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in sa = {};
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port        = htons(SERVER_PORT);

    if (bind(g_listenSock, (sockaddr*)&sa, sizeof(sa)) != 0) {
        closesocket(g_listenSock);
        WSACleanup();
        return;
    }
    listen(g_listenSock, 4);

    _plugin_logprintf("[MCP] Bridge listening on localhost:%d\n", SERVER_PORT);

    while (g_running.load()) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(g_listenSock, &fds);
        timeval tv = {0, 200000}; // 200 ms polling
        if (select(0, &fds, nullptr, nullptr, &tv) <= 0) continue;

        SOCKET client = accept(g_listenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        // Read until newline (one request per connection)
        std::string buf;
        char tmp[1024];
        while (buf.find('\n') == std::string::npos) {
            int n = recv(client, tmp, sizeof(tmp) - 1, 0);
            if (n <= 0) break;
            tmp[n] = '\0';
            buf += tmp;
        }

        if (!buf.empty()) {
            std::string resp = handleRequest(buf);
            send(client, resp.c_str(), (int)resp.size(), 0);
        }

        closesocket(client);
    }

    closesocket(g_listenSock);
    g_listenSock = INVALID_SOCKET;
    WSACleanup();
    _plugin_logprintf("[MCP] Bridge stopped\n");
}

// ── Plugin entry points ─────────────────────────────────────────────────────

#define PLUG_EXPORT extern "C" __declspec(dllexport)

PLUG_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = PLUGIN_VERSION;
    initStruct->sdkVersion    = PLUG_SDKVERSION;
    strncpy(initStruct->pluginName, PLUGIN_NAME, 256);
    g_pluginHandle = initStruct->pluginHandle;

    g_running = true;
    g_serverThread = std::thread(serverLoop);

    _plugin_logprintf("[MCP] Plugin initialized (port %d)\n", SERVER_PORT);
    return true;
}

PLUG_EXPORT void plugsetup(PLUG_SETUPSTRUCT* setupStruct)
{
    (void)setupStruct;
}

PLUG_EXPORT bool plugstop()
{
    g_running = false;
    if (g_listenSock != INVALID_SOCKET) {
        closesocket(g_listenSock);
        g_listenSock = INVALID_SOCKET;
    }
    if (g_serverThread.joinable())
        g_serverThread.join();
    return true;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    return TRUE;
}
