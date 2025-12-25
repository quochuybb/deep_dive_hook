#include "pch.h" 
#include <windows.h>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "MinHook.h"
#include "json.hpp"

#pragma comment(lib, "user32.lib")

using json = nlohmann::json;

static std::mutex qmtx;
static std::queue<std::string> q;
static std::condition_variable qcv;
static bool running = true;
static HANDLE pipeHandle = INVALID_HANDLE_VALUE;

void EnqueueEvent(const json& j) {
    if (!running) return;
    std::string s = j.dump();
    std::lock_guard<std::mutex> lk(qmtx);
    if (q.size() < 1000) q.push(s);
    qcv.notify_one();
}

void SenderThread() {
    while (running) {
        if (pipeHandle == INVALID_HANDLE_VALUE) {
            pipeHandle = CreateFileA("\\\\.\\pipe\\GZHookPipe", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (pipeHandle == INVALID_HANDLE_VALUE) { Sleep(500); continue; }
        }
        std::unique_lock<std::mutex> lk(qmtx);
        qcv.wait_for(lk, std::chrono::milliseconds(200));
        while (!q.empty()) {
            auto s = q.front(); q.pop();
            lk.unlock();
            DWORD written = 0;
            if (WriteFile(pipeHandle, s.c_str(), (DWORD)s.size(), &written, NULL)) {
                WriteFile(pipeHandle, "\n", 1, &written, NULL);
            }
            else {
                CloseHandle(pipeHandle); pipeHandle = INVALID_HANDLE_VALUE;
            }
            lk.lock();
        }
    }
    if (pipeHandle != INVALID_HANDLE_VALUE) CloseHandle(pipeHandle);
}


#define ADDR_JUMP_ENTER  3321632  
#define OFFSET_IMPULSE   0x94 

typedef void(__fastcall* StateJump_Enter_t)(void* instance, void* method);
StateJump_Enter_t original_StateJump_Enter = nullptr;

void __fastcall Hooked_StateJump(void* instance, void* method) {
    original_StateJump_Enter(instance, method);
    if (instance != nullptr) {
		float* impulsePtr = (float*)((uintptr_t)instance + OFFSET_IMPULSE);
        float oldVal = *impulsePtr;

        *impulsePtr = 50000.0f;

        float newVal = *impulsePtr;

        json j;
        j["event"] = "Jump_Debug";
        j["1_Old_Value"] = oldVal;
        j["2_New_Value"] = newVal;
        EnqueueEvent(j);
    }
    //return original_StateJump_Enter(instance, method);

}


DWORD WINAPI InitThread(LPVOID hModule) {
    std::thread(SenderThread).detach();

    if (MH_Initialize() != MH_OK) return 1;

    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    while (!hGameAssembly && running) {
        Sleep(100);
        hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    }

    if (hGameAssembly) {
        uintptr_t gameBase = (uintptr_t)hGameAssembly;
        uintptr_t targetAddr = gameBase + ADDR_JUMP_ENTER;

        if (MH_CreateHook((LPVOID)targetAddr, &Hooked_StateJump, (LPVOID*)&original_StateJump_Enter) == MH_OK) {
            MH_EnableHook((LPVOID)targetAddr);

            json j; j["status"] = "HOOK SUCCESS"; j["target"] = targetAddr;
            EnqueueEvent(j);
        }
        else {
            json j; j["status"] = "HOOK FAILED";
            EnqueueEvent(j);
        }
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        CreateThread(NULL, 0, InitThread, hinstDLL, 0, NULL);
    }
    else if (fdwReason == DLL_PROCESS_DETACH) {
        running = false;
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
    return TRUE;
}