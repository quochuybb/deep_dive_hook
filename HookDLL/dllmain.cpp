#include "pch.h"

#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <string>
#include "MinHook.h"

#ifndef STB_TEXT_HAS_SELECTION
#define STB_TEXT_HAS_SELECTION(s)   ((s)->select_start != (s)->select_end)
#endif

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include <iostream>

#pragma comment(lib, "d3d11.lib")


#define RVA_FIXED_UPDATE  0x28D2E0 

#define OFF_SPEED_X       0x3C
#define OFF_SPEED_Y       0x40
#define OFF_GRAVITY       0x34

bool  g_ShowMenu = true;    
bool  g_FlyMode = false;    
float g_SpeedMultiplier = 1.0f;    
float g_FlySpeed = 10.0f;

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
Present_t oPresent = nullptr;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;
HWND                    window = nullptr;
bool                    initImgui = false;
WNDPROC                 oWndProc = nullptr;

typedef void(__fastcall* FixedUpdate_t)(void* instance);
FixedUpdate_t original_FixedUpdate = nullptr;

void __fastcall Hooked_FixedUpdate(void* instance) {
    if (instance != nullptr) {
        if (g_SpeedMultiplier != 1.0f) {
            float* pSpeedX = (float*)((uintptr_t)instance + OFF_SPEED_X);

            if (*pSpeedX > 0.01f) {
                *pSpeedX = 5.0f * g_SpeedMultiplier;
            }
            else if (*pSpeedX < -0.01f) {
                *pSpeedX = -5.0f * g_SpeedMultiplier;
            }
        }

        if (g_FlyMode) {
            float* pGravity = (float*)((uintptr_t)instance + OFF_GRAVITY);
            float* pSpeedY = (float*)((uintptr_t)instance + OFF_SPEED_Y);

            *pGravity = 0.0f;

            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                *pSpeedY = g_FlySpeed;
            }
            else {
                *pSpeedY = 0.0f;
            }
        }
    }

    return original_FixedUpdate(instance);
}



extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN || wParam == VK_INSERT) {
        std::cout << "[LOG] Key Pressed" << std::endl;
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
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 5.0f; 

            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX11_Init(pDevice, pContext);
            initImgui = true;
        }
        else {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    if (g_ShowMenu) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::GetIO().MouseDrawCursor = g_ShowMenu;
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        ImGui::Begin("Game Inspector v1.0", &g_ShowMenu);

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "System Status: HOOKED SUCCESS");
        ImGui::Separator();

        ImGui::Checkbox("Enable Fly Mode (Press SPACE)", &g_FlyMode);
        if (g_FlyMode) {
            ImGui::Indent();
            ImGui::SliderFloat("Fly Power", &g_FlySpeed, 5.0f, 50.0f);
            ImGui::Unindent();
        }

        ImGui::Spacing();

        ImGui::Text("Movement Speed Multiplier:");
        ImGui::SliderFloat("##Speed", &g_SpeedMultiplier, 1.0f, 20.0f, "%.1f x");

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Reset All")) {
            g_FlyMode = false;
            g_SpeedMultiplier = 1.0f;
        }

        ImGui::End();

        ImGui::Render();
        pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}


DWORD WINAPI InitThread(LPVOID lpParam) {
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    std::cout << "Console Allocated!" << std::endl;
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

    if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, levels, 1, D3D11_SDK_VERSION, &scd, &d3dSwapChain, &d3dDevice, &obtained, &d3dContext))) {
        DestroyWindow(hWnd);
        return 0;
    }

    DWORD_PTR* pVTable = (DWORD_PTR*)d3dSwapChain;
    pVTable = (DWORD_PTR*)pVTable[0];
    void* presentAddr = (void*)pVTable[8]; 

    d3dSwapChain->Release();
    d3dDevice->Release();
    d3dContext->Release();
    DestroyWindow(hWnd);

    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    while (!hGameAssembly) {
        Sleep(100);
        hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    }
    void* fixedUpdateAddr = (void*)((uintptr_t)hGameAssembly + RVA_FIXED_UPDATE);

    if (MH_Initialize() != MH_OK) return 1;

    MH_CreateHook(presentAddr, &hkPresent, (LPVOID*)&oPresent);

    MH_CreateHook(fixedUpdateAddr, &Hooked_FixedUpdate, (LPVOID*)&original_FixedUpdate);

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