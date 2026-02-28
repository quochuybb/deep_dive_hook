#include "pch.h"
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <map>
#include "MinHook.h"
#include "IL2CPP_Resolver.h" 
#include "UnityAPI.h"        

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
std::vector<void*> g_Players;
std::vector<void*> g_Enemies;
std::vector<void*> g_Others;
void CreateDebugConsole() {
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    SetConsoleTitleA("QA Tester - Debug Console");
}
void Log(const char* msg) {
    if (msg) {
        printf("%s\n", msg);
    }
}

bool g_ShowMenu = true;
bool g_EnableESP = false;
void* g_SelectedObject = nullptr;
std::vector<void*> g_RootTransforms;
static char SearchUnityTypeValue[256] = "Rigidbody2D";

ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;
HWND window = nullptr;
bool initImgui = false;
WNDPROC oWndProc = nullptr;

bool IsValidPtr(void* ptr) {
    if (!ptr) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return false;
    return (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_READONLY || mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_EXECUTE_READ || mbi.Protect & PAGE_EXECUTE_READWRITE));
}


void* GetUnityType(const char* typeName) {
    if (!il2cpp_domain_get || !il2cpp_domain_get_assemblies || !il2cpp_class_from_name) return nullptr;
    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain) return nullptr;
    size_t size = 0;
    Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);
    if (!assemblies || size == 0) return nullptr;

    for (size_t i = 0; i < size; i++) {
        if (!IsValidPtr(assemblies) || !IsValidPtr(assemblies[i])) continue;
        Il2CppImage* image = assemblies[i]->image;
        if (!IsValidPtr(image)) continue;

        Il2CppClass* klass = il2cpp_class_from_name(image, "UnityEngine", typeName);
        if (klass) return il2cpp_type_get_object(il2cpp_class_get_type(klass));
    }
    return nullptr;
}
void SafeGetNameViaReflection(void* obj, char* outBuf, size_t bufSize) {
    outBuf[0] = '\0'; 
    if (!obj) return;

    static const MethodInfo* getNameMethod = nullptr;

    if (!getNameMethod) {
        Il2CppDomain* domain = il2cpp_domain_get();
        if (!domain) return;
        size_t size = 0;
        Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);
        if (!assemblies) return;

        for (size_t i = 0; i < size; i++) {
            if (!assemblies[i] || !assemblies[i]->image) continue;
            Il2CppClass* objClass = il2cpp_class_from_name(assemblies[i]->image, "UnityEngine", "Object");
            if (objClass) {
                getNameMethod = il2cpp_class_get_method_from_name(objClass, "get_name", 0);
                if (getNameMethod) break;
            }
        }
    }

    if (!getNameMethod) return;

    __try {
        Il2CppString* strObj = (Il2CppString*)il2cpp_runtime_invoke(getNameMethod, obj, nullptr, nullptr);
        if (strObj) {
            int len = *(int*)((uintptr_t)strObj + 0x10);
            if (len > 0 && len < (int)bufSize) {
                wchar_t* chars = (wchar_t*)((uintptr_t)strObj + 0x14);
                WideCharToMultiByte(CP_UTF8, 0, chars, len, outBuf, (int)bufSize, NULL, NULL);
                outBuf[len] = '\0';
            }
        }
    }
    __except (1) {}
}

std::string GetNameViaReflection(void* obj) {
    char buf[256];
    SafeGetNameViaReflection(obj, buf, sizeof(buf));

    if (buf[0] == '\0') {
        return "Unnamed Object";
    }
    return std::string(buf);
}
void SafeGetTagViaReflection(void* go, char* outBuf, size_t bufSize) {
    outBuf[0] = '\0';
    if (!go) return;

    static const MethodInfo* getTagMethod = nullptr;

    if (!getTagMethod) {
        Il2CppDomain* domain = il2cpp_domain_get();
        if (!domain) return;
        size_t size = 0;
        Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);
        if (!assemblies) return;

        for (size_t i = 0; i < size; i++) {
            if (!assemblies[i] || !assemblies[i]->image) continue;
            Il2CppClass* goClass = il2cpp_class_from_name(assemblies[i]->image, "UnityEngine", "GameObject");
            if (goClass) {
                getTagMethod = il2cpp_class_get_method_from_name(goClass, "get_tag", 0);
                if (getTagMethod) break;
            }
        }
    }

    if (!getTagMethod) return;

    __try {
        Il2CppString* strObj = (Il2CppString*)il2cpp_runtime_invoke(getTagMethod, go, nullptr, nullptr);
        if (strObj) {
            int len = *(int*)((uintptr_t)strObj + 0x10);
            if (len > 0 && len < (int)bufSize) {
                wchar_t* chars = (wchar_t*)((uintptr_t)strObj + 0x14);
                WideCharToMultiByte(CP_UTF8, 0, chars, len, outBuf, (int)bufSize, NULL, NULL);
                outBuf[len] = '\0';
            }
        }
    }
    __except (1) {}
}

std::string GetTagViaReflection(void* go) {
    char buf[128];
    SafeGetTagViaReflection(go, buf, sizeof(buf));
    if (buf[0] == '\0') return "Untagged";
    return std::string(buf);
}

void SafeGetGameObjectViaReflection(void* component, void** outGameObject) {
    *outGameObject = nullptr;
    if (!component) return;

    static const MethodInfo* getGOMethod = nullptr;

    if (!getGOMethod) {
        Il2CppDomain* domain = il2cpp_domain_get();
        if (domain) {
            size_t size = 0;
            Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);
            if (assemblies) {
                for (size_t i = 0; i < size; i++) {
                    if (!assemblies[i] || !assemblies[i]->image) continue;

                    Il2CppClass* compClass = il2cpp_class_from_name(assemblies[i]->image, "UnityEngine", "Component");
                    if (compClass) {
                        getGOMethod = il2cpp_class_get_method_from_name(compClass, "get_gameObject", 0);
                        if (getGOMethod) break;
                    }
                }
            }
        }
    }

    if (!getGOMethod) return;

    __try {
        *outGameObject = il2cpp_runtime_invoke(getGOMethod, component, nullptr, nullptr);
    }
    __except (1) {
        Log("[CRASH PREVENTED] Error invoking get_gameObject!");
    }
}

void* GetGameObjectViaReflection(void* component) {
    void* gameObject = nullptr;
    SafeGetGameObjectViaReflection(component, &gameObject);
    return gameObject;
}
void SafeGetTransformViaReflection(void* go, void** outTransform) {
    *outTransform = nullptr;
    if (!go) return;

    static const MethodInfo* getTransformMethod = nullptr;

    if (!getTransformMethod) {
        Il2CppDomain* domain = il2cpp_domain_get();
        if (domain) {
            size_t size = 0;
            Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);
            if (assemblies) {
                for (size_t i = 0; i < size; i++) {
                    if (!assemblies[i] || !assemblies[i]->image) continue;

                    Il2CppClass* goClass = il2cpp_class_from_name(assemblies[i]->image, "UnityEngine", "GameObject");
                    if (goClass) {
                        getTransformMethod = il2cpp_class_get_method_from_name(goClass, "get_transform", 0);
                        if (getTransformMethod) break;
                    }
                }
            }
        }
    }

    if (!getTransformMethod) return;

    __try {
        *outTransform = il2cpp_runtime_invoke(getTransformMethod, go, nullptr, nullptr);
    }
    __except (1) {
        Log("[CRASH PREVENTED] Error invoking get_transform!");
    }
}

void* GetTransformViaReflection(void* go) {
    void* transform = nullptr;
    SafeGetTransformViaReflection(go, &transform);
    return transform;
}
void SafeGetPositionViaReflection(void* transform, Unity::Vector3* outPos) {
    if (!transform || !outPos) return;

    static const MethodInfo* getPositionMethod = nullptr;

    if (!getPositionMethod) {
        Il2CppDomain* domain = il2cpp_domain_get();
        if (domain) {
            size_t size = 0;
            Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);
            if (assemblies) {
                for (size_t i = 0; i < size; i++) {
                    if (!assemblies[i] || !assemblies[i]->image) continue;

                    Il2CppClass* transformClass = il2cpp_class_from_name(assemblies[i]->image, "UnityEngine", "Transform");
                    if (transformClass) {
                        getPositionMethod = il2cpp_class_get_method_from_name(transformClass, "get_position", 0);
                        if (getPositionMethod) break;
                    }
                }
            }
        }
    }

    if (!getPositionMethod) return;

    __try {
        void* boxedVec3 = il2cpp_runtime_invoke(getPositionMethod, transform, nullptr, nullptr);

        if (boxedVec3) {
            *outPos = *(Unity::Vector3*)((uintptr_t)boxedVec3 + 0x10);
        }
    }
    __except (1) {
    }
}
void* GetMainCameraSafe() {
    static const MethodInfo* getMainCamMethod = nullptr;
    if (!getMainCamMethod) {
        Il2CppDomain* domain = il2cpp_domain_get();
        if (domain) {
            size_t size = 0;
            Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);
            if (assemblies) {
                for (size_t i = 0; i < size; i++) {
                    if (!assemblies[i] || !assemblies[i]->image) continue;
                    Il2CppClass* camClass = il2cpp_class_from_name(assemblies[i]->image, "UnityEngine", "Camera");
                    if (camClass) {
                        getMainCamMethod = il2cpp_class_get_method_from_name(camClass, "get_main", 0);
                        if (getMainCamMethod) break;
                    }
                }
            }
        }
    }
    if (!getMainCamMethod) return nullptr;

    void* camera = nullptr;
    __try { camera = il2cpp_runtime_invoke(getMainCamMethod, nullptr, nullptr, nullptr); }
    __except (1) {}
    return camera;
}
bool WorldToScreenSafe(void* camera, Unity::Vector3 worldPos, Unity::Vector3& outScreenPos) {
    if (!camera) return false;

    static const MethodInfo* w2sMethod = nullptr;

    if (!w2sMethod) {
        Log("WorldToScreenSafe: Resolving WorldToScreenPoint method...");

        Il2CppDomain* domain = il2cpp_domain_get();
        if (!domain) return false;

        size_t size = 0;
        Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);

        if (!assemblies || size == 0) {
            Log("[ERROR] Assemblies array is null or empty!");
            return false;
        }

        Il2CppClass* camClass = nullptr;

        for (size_t i = 0; i < size; i++) {
            if (assemblies[i] && assemblies[i]->image) {
                camClass = il2cpp_class_from_name(assemblies[i]->image, "UnityEngine", "Camera");
                if (camClass) {
                    Log("WorldToScreenSafe: Found Camera class!");
                    break; 
                }
            }
        }

        if (camClass) {
            w2sMethod = il2cpp_class_get_method_from_name(camClass, "WorldToScreenPoint", 1);
            if (w2sMethod) {
                Log("WorldToScreenSafe: Successfully resolved method!");
            }
        }
    }

    if (!w2sMethod) {
        Log("[ERROR] Failed to resolve WorldToScreenPoint method!");
        return false;
    }

    __try {
        void* args[1] = { &worldPos };
        void* result = il2cpp_runtime_invoke(w2sMethod, camera, args, nullptr);

        if (result) {
            outScreenPos = *(Unity::Vector3*)((uintptr_t)result + 0x10);
            return true;
        }
    }
    __except (1) {
        Log("[CRASH PREVENTED] Error invoking WorldToScreenPoint!");
    }

    return false;
}
bool GetPositionFromGameObjectSafe(void* gameObject, Unity::Vector3& outPos) {
    if (!IsValidPtr(gameObject)) return false;
    if (!IsValidPtr((void*)((uintptr_t)gameObject + 0x10)) || *(void**)((uintptr_t)gameObject + 0x10) == nullptr) return false;

    void* transform = GetTransformViaReflection(gameObject);
    if (!IsValidPtr(transform)) return false;

    SafeGetPositionViaReflection(transform, &outPos);

    return true;
}
typedef Il2CppArray* (*t_FindObjectsOfType)(void* type);

void RefreshLivingEntities(std::string unityType) {
    auto start = std::chrono::high_resolution_clock::now();

    g_Players.clear();
    g_Enemies.clear();
    g_Others.clear();
    g_SelectedObject = nullptr;

    if (il2cpp_thread_attach) il2cpp_thread_attach(il2cpp_domain_get());

    void* typeObj = GetUnityType(SearchUnityTypeValue);
    if (!typeObj) { Log("[ERROR] Unity type not found!"); return; }

    t_FindObjectsOfType findFunc = (t_FindObjectsOfType)il2cpp_resolve_icall("UnityEngine.Object::FindObjectsOfType");
    if (!findFunc) {
        Log("[ERROR] Failed to resolve icall: FindObjectsOfType");
        return;
    }

    Il2CppArray* objects = findFunc(typeObj);
    if (!IsValidPtr(objects)) {
        Log("[ERROR] FindObjectsOfType returned null or an invalid pointer");
        return;
    }

    uintptr_t baseAddr = (uintptr_t)objects;
    if (!IsValidPtr((void*)(baseAddr + 0x18))) {
		return;
    }

    uint32_t count = *(uint32_t*)(baseAddr + 0x18);
    void** dataArray = (void**)(baseAddr + 0x20);

    for (uint32_t i = 0; i < count; i++) {
        if (!IsValidPtr(&dataArray[i])) {
			break;
        }

        void* animatorComp = dataArray[i];
        if (IsValidPtr(animatorComp) && IsValidPtr((void*)((uintptr_t)animatorComp + 0x10)) && *(void**)((uintptr_t)animatorComp + 0x10) != nullptr) {
			void* go = GetGameObjectViaReflection(animatorComp);
            if (!IsValidPtr(go)) continue;

            std::string name = GetNameViaReflection(go);
            std::string tag = GetTagViaReflection(go);
            if (tag == "Player" || name.find("Player") != std::string::npos) {
                g_Players.push_back(go);
            }
            else if (tag == "Enemy" || name.find("Enemy") != std::string::npos || name.find("Monster") != std::string::npos) {
                g_Enemies.push_back(go);
            }
            else {
                g_Others.push_back(go);
            }

            char logMsg[256];
            sprintf_s(logMsg, "[ENTITY] %s | Tag: %s", name.c_str(), tag.c_str());
            Log(logMsg);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms = end - start;

    char buf2[256];
    sprintf_s(buf2, "[DEBUG] Entity scan completed in %.2f ms. Players: %zu | Enemies: %zu | Others: %zu",
        ms.count(), g_Players.size(), g_Enemies.size(), g_Others.size());
    Log(buf2);
}
void DrawLazyHierarchyNode(void* transform) {
    if (!IsValidPtr(transform)) return;
    if (!IsValidPtr((void*)((uintptr_t)transform + 0x10)) || *(void**)((uintptr_t)transform + 0x10) == nullptr) {
        return;
    }
    void* go = GetGameObjectViaReflection(transform);
    if (!IsValidPtr(go)) return;
    std::string name = GetNameViaReflection(go);
    if (name.empty()) name = "Unnamed Entity";

    int childCount = Unity::GetChildCount(transform);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (childCount == 0) flags |= ImGuiTreeNodeFlags_Leaf;
    if (g_SelectedObject == go) flags |= ImGuiTreeNodeFlags_Selected; 
    bool isOpen = ImGui::TreeNodeEx(transform, flags, "%s", name.c_str());
    if (ImGui::IsItemClicked()) {
        g_SelectedObject = go;
    }

    if (isOpen) {
        for (int i = 0; i < childCount; i++) {
            void* childTransform = Unity::GetChild(transform, i);
			std::string name = GetNameViaReflection(childTransform);

            DrawLazyHierarchyNode(childTransform);
        }
        ImGui::TreePop();
    }
}

void SafeDrawMainUI() {
    if (!g_ShowMenu) return;

    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Universal Aimbot/ESP Framework", &g_ShowMenu);

    ImGui::BeginChild("EntityList", ImVec2(350, 0), true);

    ImGui::Checkbox("Enable ESP Mode", &g_EnableESP);
    ImGui::Spacing(); 
    ImGui::InputText("Input Unity Type to scan (Default type: Rigidbody2D)", SearchUnityTypeValue, sizeof(SearchUnityTypeValue));
    ImGui::Spacing();
    if (ImGui::Button("Scan Entities", ImVec2(-1, 30))) {
        RefreshLivingEntities(SearchUnityTypeValue);
    }
    ImGui::Separator();

    auto DrawCategory = [](const char* title, const std::vector<void*>& entityList) {
        if (!entityList.empty()) {
            char headerTitle[128];
            sprintf_s(headerTitle, "%s [%zu]", title, entityList.size());

            if (ImGui::CollapsingHeader(headerTitle, ImGuiTreeNodeFlags_DefaultOpen)) {
                for (void* go : entityList) {
                    if (!IsValidPtr(go)) continue;

                    if (!IsValidPtr((void*)((uintptr_t)go + 0x10)) || *(void**)((uintptr_t)go + 0x10) == nullptr) continue;
                    void* transform = GetTransformViaReflection(go);

                    if (IsValidPtr(transform)) {
                        DrawLazyHierarchyNode(transform);
                    }
                }
            }
        }};

    DrawCategory("Players", g_Players);
    DrawCategory("Enemies", g_Enemies);
    DrawCategory("Others (NPC, Pets...)", g_Others);

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Inspector", ImVec2(0, 0), true);
    if (g_SelectedObject && IsValidPtr(g_SelectedObject) &&
        IsValidPtr((void*)((uintptr_t)g_SelectedObject + 0x10)) && *(void**)((uintptr_t)g_SelectedObject + 0x10) != nullptr)
    {
        std::string objName = GetNameViaReflection(g_SelectedObject);
        std::string objTag = GetTagViaReflection(g_SelectedObject);

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "[ %s ]", objName.c_str());
        ImGui::TextDisabled("GameObject: 0x%p | Tag: %s", g_SelectedObject, objTag.c_str());
        ImGui::Separator();

        void* transform = GetTransformViaReflection(g_SelectedObject); 
        if (IsValidPtr(transform)) {
            Unity::Vector3 pos;
            if (GetPositionFromGameObjectSafe(g_SelectedObject, pos)) {
                ImGui::Text("Position (X, Y, Z)");
                ImGui::InputFloat3("##pos", &pos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
            }
        }
    }
    else {
        ImGui::TextDisabled("Select an entity to inspect.");
    }
    ImGui::EndChild();

    ImGui::End();
}
void DrawESP() {
    if (!g_EnableESP) return; 
    Log("DrawESP");
    if (g_Others.empty()) return;
    Log("g_Others not empty");
    void* mainCamera = GetMainCameraSafe();
    if (!mainCamera) return;
    Log("Get Main Camera success");

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    for (void* enemy : g_Others) {
        if (!IsValidPtr(enemy)) continue;
        Unity::Vector3 worldPos;
        if (GetPositionFromGameObjectSafe(enemy, worldPos)) {
			Log("Got world position");
            Unity::Vector3 screenPos;
            if (WorldToScreenSafe(mainCamera, worldPos, screenPos)) {
				Log("World to screen success");
                if (screenPos.z > 0) {
					Log("Enemy is in front of camera");
                    float realY = screenSize.y - screenPos.y;

                    drawList->AddCircleFilled(ImVec2(screenPos.x, realY), 5.0f, IM_COL32(255, 0, 0, 255));

                    drawList->AddLine(ImVec2(screenSize.x / 2, screenSize.y), ImVec2(screenPos.x, realY), IM_COL32(255, 0, 0, 150), 1.5f);

                    std::string name = GetNameViaReflection(enemy);
                    drawList->AddText(ImVec2(screenPos.x + 8, realY - 8), IM_COL32(255, 255, 255, 255), name.c_str());
                }
            }
        }
    }
}
void DrawMainUI() {
    __try {
        DrawESP();
    }
    __except (1) {}

    __try {
        SafeDrawMainUI();
    }
    __except (1) {}
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && wParam == VK_F1) {
        g_ShowMenu = !g_ShowMenu;
        return 0;
    }

    if (g_ShowMenu) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP ||
            uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP ||
            uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEMOVE) {
            return true;
        }
    }

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
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

            SetupIL2CPP();
            Unity::Init();

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

            ImGui::StyleColorsDark();
            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX11_Init(pDevice, pContext);
            initImgui = true;
        }
        else {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
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

    ImGui::GetIO().MouseDrawCursor = g_ShowMenu;

    DrawMainUI();

    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI InitThread(LPVOID lpParam) {
    CreateDebugConsole();
    Log("[+] Console khoi tao thanh cong!");
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

    MH_Initialize();
    MH_CreateHook(resizeAddr, &hkResizeBuffers, (LPVOID*)&oResizeBuffers);
    MH_CreateHook(presentAddr, &hkPresent, (LPVOID*)&oPresent);
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