// controller/src/main.cpp
#include <windows.h>
#include <iostream>
#include <string>
#include "nlohmann/json.hpp"
using json = nlohmann::json;

int main() {
    const char* pipeName = R"(\\.\pipe\GZHookPipe)";
    HANDLE hPipe = CreateNamedPipeA(pipeName,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1, 4096, 4096, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) { std::cerr << "CreateNamedPipe failed\n"; return 1; }
    std::cout << "Controller: waiting for hook DLL...\n";
    BOOL ok = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!ok) { std::cerr << "ConnectNamedPipe failed\n"; CloseHandle(hPipe); return 1; }
    std::cout << "Connected.\n";
    std::string accum;
    char buf[4096];
    DWORD read;
    while (ReadFile(hPipe, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
        accum.append(buf, buf + read);
        // split by newline
        size_t pos;
        while ((pos = accum.find('\n')) != std::string::npos) {
            std::string line = accum.substr(0, pos);
            accum.erase(0, pos + 1);
            try {
                auto j = json::parse(line);
                std::cout << "[EVENT] " << j.dump() << "\n";
            }
            catch (...) {
                // ignore
            }
        }
    }
    CloseHandle(hPipe);
    return 0;
}
