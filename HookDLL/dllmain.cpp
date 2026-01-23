#include "pch.h"
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <string>
#include "MinHook.h"
#include <Psapi.h>
#include <sstream>
#include <iostream>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "SDK.h" 

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "Psapi.lib")

#define RVA_FIXED_UPDATE  0x28D2E0 
#define OFF_SPEED_X       0x3C
#define OFF_SPEED_Y       0x40
#define OFF_GRAVITY       0x34

bool  g_ShowMenu = true;
bool  g_FlyMode = false;
bool  g_GodMode = false;
bool  g_EnableESP = false;
float g_SpeedMultiplier = 1.0f;
float g_FlySpeed = 10.0f;

float g_ESP_Height = 1.8f; 
float g_ESP_Width = 0.8f; 

void* g_CurrentRoom = nullptr;
std::vector<std::string> g_LogConsole;

ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;
HWND window = nullptr;
bool initImgui = false;
WNDPROC oWndProc = nullptr;
void* g_LocalPlayer = nullptr;
void* g_CachedCamera = nullptr; 

typedef void* (__fastcall* GetGameObject_t)(void* component);
GetGameObject_t Call_GetGameObject = nullptr;

typedef Il2CppString* (__fastcall* GetName_t)(void* object);
GetName_t Call_GetName = nullptr;

typedef void* (__fastcall* GetMainCamera_t)();
GetMainCamera_t fnGetMainCamera = nullptr;

typedef Vector3(__fastcall* WorldToScreen_t)(void* camera, Vector3 position);
WorldToScreen_t fnWorldToScreen = nullptr;
typedef Vector3(__fastcall* GetPosition_t)(void* transform);
GetPosition_t fnGetPosition = nullptr;

typedef void* (__fastcall* GetTransform_t)(void* component);
GetTransform_t fnGetTransform = nullptr;
typedef void(__fastcall* GetVec2_Injected_t)(void* _unity_self, Vector2* outResult);
GetVec2_Injected_t fnGetOffset_Injected = nullptr;
GetVec2_Injected_t fnGetSize_Injected = nullptr;
bool IsValidPtr(void* p) {
    return (p != nullptr && (uintptr_t)p > 0x10000 && (uintptr_t)p < 0x7FFFFFFFFFFF);
}

Vector3 GetEnemyPosition(void* enemyPtr) {
    if (!enemyPtr || !fnGetTransform || !fnGetPosition) return { 0, 0, 0 };
    void* transform = fnGetTransform(enemyPtr);
    if (!transform) return { 0, 0, 0 };
    return fnGetPosition(transform);
}
bool WorldToScreen_Safe(void* camera, Vector3 worldPos, Vector2& screenPos) {
    if (!camera || !fnWorldToScreen) return false;
    Vector3 result = { 0,0,0 };
    __try {
        result = fnWorldToScreen(camera, worldPos);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }

    if (result.z <= 0) return false;

    float screenHeight = ImGui::GetIO().DisplaySize.y;
    screenPos.x = result.x;
    screenPos.y = screenHeight - result.y; 
    return true;
}
Bounds GetEnemyBounds_Manual(void* enemyPtr) {
    if (!IsValidPtr(enemyPtr)) return { {0,0,0}, {0,0,0} };
    void* nativeCollider = nullptr;
    __try {
        void* physicsMover = *(void**)((uintptr_t)enemyPtr + 0x40);
        if (!IsValidPtr(physicsMover)) {
            return {{0,0,0}, {0,0,0}};
        }
        void* managedCollider = *(void**)((uintptr_t)physicsMover + 0x20);
        if (!IsValidPtr(managedCollider)) return { {0,0,0}, {0,0,0} };

        nativeCollider = *(void**)((uintptr_t)managedCollider + 0x10);
    }
    __except (1) { 
        return { {0,0,0}, {0,0,0} };
    }

    if (!IsValidPtr(nativeCollider)) return { {0,0,0}, {0,0,0} };

    Vector2 size = { 1.0f, 1.0f }; 
    Vector2 offset = { 0.0f, 0.0f }; 

    if (fnGetSize_Injected) {
        __try { fnGetSize_Injected(nativeCollider, &size); }
        __except (1) {
            size = { 1.0f, 1.0f }; }
    }

    if (fnGetOffset_Injected) {
        __try { fnGetOffset_Injected(nativeCollider, &offset); }
        __except (1) { 
            offset = { 0.0f, 0.0f }; }
    }

    Vector3 worldPos = GetEnemyPosition(enemyPtr);
    if (worldPos.x == 0 && worldPos.y == 0) return { {0,0,0}, {0,0,0} };

    Bounds result;

    result.center.x = worldPos.x + offset.x;
    result.center.y = worldPos.y + offset.y;
    result.center.z = worldPos.z;

    result.extents.x = size.x / 2.0f;
    result.extents.y = size.y / 2.0f;
    result.extents.z = 0.5f; 

    std::cout << "[ESP] Raw Size: " << size.x << "x" << size.y << std::endl;

    return result;
}
void DrawColliderBox(void* camera, Bounds bounds) {
    Vector3 worldTR;
    worldTR.x = bounds.center.x + bounds.extents.x;
    worldTR.y = bounds.center.y + bounds.extents.y;
    worldTR.z = bounds.center.z;

    Vector3 worldBL;
    worldBL.x = bounds.center.x - bounds.extents.x;
    worldBL.y = bounds.center.y - bounds.extents.y;
    worldBL.z = bounds.center.z;

    Vector2 screenTR, screenBL;
    if (!WorldToScreen_Safe(camera, worldTR, screenTR)) return;
    if (!WorldToScreen_Safe(camera, worldBL, screenBL)) return;

    float x = screenBL.x; 
    float y = screenTR.y; 

    float w = abs(screenTR.x - screenBL.x);
    float h = abs(screenBL.y - screenTR.y);

    ImGui::GetBackgroundDrawList()->AddRect(
        ImVec2(x, y),
        ImVec2(x + w, y + h),
        IM_COL32(0, 255, 0, 255), 
        1.5f                     
    );
}

void DrawEnemyESP() {
    if (!g_CurrentRoom || !IsValidPtr(g_CurrentRoom)) return;

    if (fnGetMainCamera) {
        __try { g_CachedCamera = fnGetMainCamera(); }
        __except (1) { g_CachedCamera = nullptr; }
    }
    if (!IsValidPtr(g_CachedCamera)) return;

    __try {
        RoomSDK* pRoom = (RoomSDK*)g_CurrentRoom;
        RoomEventStateDataSDK* pData = pRoom->m_CurrentRoomData;
        if (!IsValidPtr(pData)) return;

        UnityArray<EnemySDK*>* pEnemyArray = pData->Enemies;
        if (!IsValidPtr(pEnemyArray)) return;

        int enemyCount = pEnemyArray->GetSize();

        if (enemyCount <= 0 || enemyCount > 200) return;

        for (int i = 0; i < enemyCount; i++) {
            EnemySDK* enemy = pEnemyArray->Get(i);
            if (!IsValidPtr(enemy)) continue;

            Bounds bounds = GetEnemyBounds_Manual(enemy);

            if (bounds.extents.x < 0.01f || bounds.extents.y < 0.01f) {
                Vector3 feetPos = GetEnemyPosition(enemy);
                if (feetPos.x == 0 && feetPos.y == 0) continue;

                bounds.center = feetPos;
                bounds.center.y += 1.0f; 
                bounds.extents.x = 10.0f; 
                bounds.extents.y = 10.0f; 
            }

            DrawColliderBox(g_CachedCamera, bounds);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

    }
}


typedef void(__fastcall* OnRoomEnter_t)(void* roomInstance, void* player);
OnRoomEnter_t oOnRoomEnter = nullptr;
void __fastcall hkOnRoomEnter(void* roomInstance, void* player) {
    if (roomInstance != nullptr) {
        g_CurrentRoom = roomInstance;
    }
    return oOnRoomEnter(roomInstance, player);
}

typedef void(__fastcall* UnityLog_t)(void* message);
UnityLog_t oUnityLog = nullptr;
void __fastcall hkUnityLog(void* message) {
    if (message != nullptr) {
        Il2CppString* unityStr = (Il2CppString*)message;
        std::string logMsg = UseString(unityStr);
        if (g_LogConsole.size() >= 100) g_LogConsole.erase(g_LogConsole.begin());
        g_LogConsole.push_back(logMsg);
    }
    return oUnityLog(message);
}

typedef void(__fastcall* PlayerUpdate_t)(void* instance);
PlayerUpdate_t oPlayerUpdate = nullptr;
void __fastcall hkPlayerUpdate(void* instance) {
    if (instance != nullptr) g_LocalPlayer = instance;
    return oPlayerUpdate(instance);
}

typedef void(__fastcall* InflictDamage_Struct_t)(void* instance, void* damageObj, void* inflictor);
InflictDamage_Struct_t oInflictDamageStruct = nullptr;
void __fastcall hkInflictDamageStruct(void* instance, void* damageObj, void* inflictor) {
    std::string objectName = "Unknown";
    if (instance != nullptr && Call_GetGameObject && Call_GetName) {
        void* gameObject = Call_GetGameObject(instance);

        if (gameObject) {
            Il2CppString* unityStr = Call_GetName(gameObject);
            objectName = UseString(unityStr);
        }
    }
    std::cout << "[LOG] Hit! Instance: " << objectName << std::endl;
    if (g_GodMode && instance != nullptr && instance == g_LocalPlayer) return;
    return oInflictDamageStruct(instance, damageObj, inflictor);
}

typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
ResizeBuffers_t oResizeBuffers = nullptr;
HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    if (mainRenderTargetView) {
        pContext->OMSetRenderTargets(0, 0, 0);
        mainRenderTargetView->Release();
        mainRenderTargetView = nullptr;
    }
    return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

typedef void(__fastcall* FixedUpdate_t)(void* instance);
FixedUpdate_t original_FixedUpdate = nullptr;
void __fastcall Hooked_FixedUpdate(void* instance) {
    if (instance != nullptr) {
        if (g_SpeedMultiplier != 1.0f) {
            float* pSpeedX = (float*)((uintptr_t)instance + OFF_SPEED_X);
            if (*pSpeedX > 0.01f) *pSpeedX = 5.0f * g_SpeedMultiplier;
            else if (*pSpeedX < -0.01f) *pSpeedX = -5.0f * g_SpeedMultiplier;
        }
        if (g_FlyMode) {
            float* pGravity = (float*)((uintptr_t)instance + OFF_GRAVITY);
            float* pSpeedY = (float*)((uintptr_t)instance + OFF_SPEED_Y);
            *pGravity = 0.0f;
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) *pSpeedY = g_FlySpeed;
            else *pSpeedY = 0.0f;
        }
    }
    return original_FixedUpdate(instance);
}


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && wParam == VK_F1) {
        g_ShowMenu = !g_ShowMenu;
        ImGui::GetIO().MouseDrawCursor = g_ShowMenu;
        return 0;
    }
    if (g_ShowMenu) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        return true;
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
Present_t oPresent = nullptr;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!initImgui) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            ImGui::StyleColorsDark();
            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX11_Init(pDevice, pContext);
            initImgui = true;
        }
        else return oPresent(pSwapChain, SyncInterval, Flags);
    }

    if (mainRenderTargetView == NULL) {
        ID3D11Texture2D* pBackBuffer;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
        pBackBuffer->Release();
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_EnableESP) {
        DrawEnemyESP();
    }

    if (g_ShowMenu) {
        ImGui::GetIO().MouseDrawCursor = true;
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Game Inspector - HARDCODED EDITION", &g_ShowMenu);

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "System Status: SAFE & STABLE");
        ImGui::Separator();
        ImGui::Checkbox("Enable Fly Mode", &g_FlyMode);
        if (g_FlyMode) ImGui::SliderFloat("Fly Power", &g_FlySpeed, 5.0f, 50.0f);
        ImGui::Checkbox("Enable God Mode", &g_GodMode);
        ImGui::Separator();

        ImGui::Checkbox("Enable ESP (Box 2D)", &g_EnableESP);
        if (g_EnableESP) {
            ImGui::Indent();
            ImGui::SliderFloat("Box Height", &g_ESP_Height, 5.0f, 15.0f, "%.1fm");
            ImGui::SliderFloat("Box Width", &g_ESP_Width, 1.0f, 9.0f, "%.1fm");
            ImGui::Unindent();
        }

        ImGui::Spacing();
        ImGui::SliderFloat("Speed Multiplier", &g_SpeedMultiplier, 1.0f, 20.0f, "%.1f x");

        ImGui::Separator();
        if (ImGui::Button("Clear Console")) g_LogConsole.clear();
        ImGui::BeginChild("LogRegion", ImVec2(0, 100), true);
        for (const auto& log : g_LogConsole) ImGui::TextUnformatted(log.c_str());
        ImGui::EndChild();

        ImGui::End();
    }
    else {
        ImGui::GetIO().MouseDrawCursor = false;
    }

    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}


std::vector<int> PatternToByte(const char* pattern) {
    std::vector<int> bytes;
    char* start = const_cast<char*>(pattern);
    char* end = const_cast<char*>(pattern) + strlen(pattern);
    for (char* current = start; current < end; ++current) {
        if (*current == '?') {
            ++current;
            if (*current == '?') ++current;
            bytes.push_back(-1);
        }
        else {
            bytes.push_back(strtoul(current, &current, 16));
        }
    }
    return bytes;
}

uintptr_t FindPattern(HMODULE hModule, const char* pattern) {
    MODULEINFO modInfo;
    GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));
    uintptr_t startAddress = (uintptr_t)modInfo.lpBaseOfDll;
    size_t size = modInfo.SizeOfImage;
    std::vector<int> patternBytes = PatternToByte(pattern);
    uintptr_t patternLength = patternBytes.size();
    int* patternData = patternBytes.data();

    for (uintptr_t i = 0; i < size - patternLength; ++i) {
        bool found = true;
        for (uintptr_t j = 0; j < patternLength; ++j) {
            unsigned char byte = *(unsigned char*)(startAddress + i + j);
            if (patternData[j] != -1 && patternData[j] != byte) {
                found = false;
                break;
            }
        }
        if (found) return startAddress + i;
    }
    return 0;
}

DWORD WINAPI InitThread(LPVOID lpParam) {
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
	std::cout << "[+] Console allocated." << std::endl;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"DX11 Dummy", NULL };
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow(L"DX11 Dummy", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL obtained;
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    IDXGISwapChain* d3dSwapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(scd));
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, levels, 1, D3D11_SDK_VERSION, &scd, &d3dSwapChain, &d3dDevice, &obtained, &d3dContext);

    DWORD_PTR* pVTable = (DWORD_PTR*)d3dSwapChain;
    pVTable = (DWORD_PTR*)pVTable[0];
    void* presentAddr = (void*)pVTable[8];
    void* resizeAddr = (void*)pVTable[13];

    d3dSwapChain->Release();
    d3dDevice->Release();
    d3dContext->Release();
    DestroyWindow(hWnd);

    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    while (!hGameAssembly) {
        Sleep(100);
        hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    }

    uintptr_t baseAddr = (uintptr_t)hGameAssembly;

    void* addrPlayerUpdate = (void*)(baseAddr + 0x321C10);
    void* addrInflictDamage = (void*)(baseAddr + 0x28A8D0);
    void* addrUnityLog = (void*)(baseAddr + 0x11FBBA0);
    void* addrRoomEnter = (void*)(baseAddr + 0x2AB8E0);

    Call_GetGameObject = (GetGameObject_t)(baseAddr + 0x12376E0);
    Call_GetName = (GetName_t)(baseAddr + 0x12427A0);
    fnGetTransform = (GetTransform_t)(baseAddr + 0x1237620);
    fnGetPosition = (GetPosition_t)(baseAddr + 0x124C3B0);
    fnGetMainCamera = (GetMainCamera_t)(baseAddr + 0x11F8250);
    fnWorldToScreen = (WorldToScreen_t)(baseAddr + 0x11F7BA0);
    fnGetOffset_Injected = (GetVec2_Injected_t)(baseAddr + 0x12BEBE0);
    fnGetSize_Injected = (GetVec2_Injected_t)(baseAddr + 0x12BF000);

    const char* fixedUpdateSig = "40 53 48 81 EC C0 00 00 00 80 3D 4A 7F AD 01 00";
    uintptr_t foundAddr = FindPattern(hGameAssembly, fixedUpdateSig);

    MH_Initialize();
    if (foundAddr != 0) MH_CreateHook((void*)foundAddr, &Hooked_FixedUpdate, (LPVOID*)&original_FixedUpdate);

    MH_CreateHook(addrRoomEnter, &hkOnRoomEnter, (LPVOID*)&oOnRoomEnter);
    MH_CreateHook(addrUnityLog, &hkUnityLog, (LPVOID*)&oUnityLog);
    MH_CreateHook(addrInflictDamage, &hkInflictDamageStruct, (LPVOID*)&oInflictDamageStruct);
    MH_CreateHook(resizeAddr, &hkResizeBuffers, (LPVOID*)&oResizeBuffers);
    MH_CreateHook(presentAddr, &hkPresent, (LPVOID*)&oPresent);
    MH_CreateHook(addrPlayerUpdate, &hkPlayerUpdate, (LPVOID*)&oPlayerUpdate);

    MH_EnableHook(MH_ALL_HOOKS);
    return 0;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, InitThread, NULL, 0, NULL);
    }
    return TRUE;
}