#include "pch.h"
#include "UnityAPI.h"
#include <windows.h>
#include <vector>
#include <chrono>

extern void Log(const char* msg);

namespace Unity {
    typedef void (*t_get_go_inj)(void* comp, void** outGO);
    typedef void* (*t_get_go_std)(void* comp);

    typedef void (*t_get_transform_inj)(void* go, void** outTransform);
    typedef void* (*t_get_transform_std)(void* go);

    typedef void (*t_get_parent_inj)(void* t, void** out);
    typedef void* (*t_get_parent_std)(void* t);

    typedef int (*t_get_childCount)(void* t);

    typedef void (*t_get_child_inj)(void* t, int index, void** out);
    typedef void* (*t_get_child_std)(void* t, int index);

    typedef void (*t_GetName_inj)(void* obj, Il2CppString** outName);
    typedef Il2CppString* (*t_GetName_std)(void* obj);

    typedef void (*t_get_position)(void* t, Vector3* outPos);
    typedef void (*t_set_position)(void* t, Vector3* pos);

    void* p_get_go = nullptr; bool is_inj_go = false;
    void* p_get_transform = nullptr; bool is_inj_transform = false;
    void* p_get_parent = nullptr; bool is_inj_parent = false;
    void* p_get_childCount = nullptr;
    void* p_get_child = nullptr; bool is_inj_child = false;
    void* p_GetName = nullptr; bool is_inj_name = false;
    void* p_get_position = nullptr;
    void* p_set_position = nullptr;

    void* ResolveSmart(const std::vector<const char*>& sigs, bool& isInjected) {
        for (const char* sig : sigs) {
            void* ptr = il2cpp_resolve_icall(sig);
            if (ptr) {
                isInjected = (std::string(sig).find("_Injected") != std::string::npos);
                return ptr;
            }
        }
        return nullptr;
    }

    bool Init() {
        if (!il2cpp_resolve_icall) return false;

        p_get_go = ResolveSmart({ "UnityEngine.Component::get_gameObject_Injected", "UnityEngine.Component::get_gameObject" }, is_inj_go);
        p_get_transform = ResolveSmart({ "UnityEngine.GameObject::get_transform_Injected", "UnityEngine.GameObject::get_transform" }, is_inj_transform);
        p_get_parent = ResolveSmart({ "UnityEngine.Transform::get_parent_Injected", "UnityEngine.Transform::get_parent" }, is_inj_parent);
        p_get_childCount = ResolveSmart({ "UnityEngine.Transform::get_childCount" }, is_inj_name);
        p_get_child = ResolveSmart({ "UnityEngine.Transform::GetChild_Injected", "UnityEngine.Transform::GetChild" }, is_inj_child);
        p_GetName = ResolveSmart({ "UnityEngine.Object::GetName_Injected", "UnityEngine.Object::get_name_Injected", "UnityEngine.Object::GetName", "UnityEngine.Object::get_name" }, is_inj_name);

        bool dummy;
        p_get_position = ResolveSmart({ "UnityEngine.Transform::get_position_Injected", "UnityEngine.Transform::get_position" }, dummy);
        p_set_position = ResolveSmart({ "UnityEngine.Transform::set_position_Injected", "UnityEngine.Transform::set_position" }, dummy);

        return p_get_transform && p_GetName;
    }

    void* GetGameObject(void* transform) {
        if (!transform || !p_get_go) return nullptr;
        if (is_inj_go) {
            void* go = nullptr;
            __try { ((t_get_go_inj)p_get_go)(transform, &go); return go; }
            __except (1) { return nullptr; }
        }
        __try { return ((t_get_go_std)p_get_go)(transform); }
        __except (1) { return nullptr; }
    }

    void* GetTransform(void* gameObject) {
        if (!gameObject || !p_get_transform) return nullptr;
        if (is_inj_transform) {
            void* t = nullptr;
            __try { ((t_get_transform_inj)p_get_transform)(gameObject, &t); return t; }
            __except (1) { return nullptr; }
        }
        __try { return ((t_get_transform_std)p_get_transform)(gameObject); }
        __except (1) { return nullptr; }
    }

    void* GetParent(void* transform) {
        if (!transform || !p_get_parent) return nullptr;
        if (is_inj_parent) {
            void* p = nullptr;
            __try { ((t_get_parent_inj)p_get_parent)(transform, &p); return p; }
            __except (1) { return nullptr; }
        }
        __try { return ((t_get_parent_std)p_get_parent)(transform); }
        __except (1) { return nullptr; }
    }

    int GetChildCount(void* transform) {
        if (!transform || !p_get_childCount) return 0;
        __try { return ((t_get_childCount)p_get_childCount)(transform); }
        __except (1) { return 0; }
    }

    void* GetChild(void* transform, int index) {
        if (!transform || !p_get_child) return nullptr;
        if (is_inj_child) {
            void* c = nullptr;
            __try { ((t_get_child_inj)p_get_child)(transform, index, &c); return c; }
            __except (1) { return nullptr; }
        }
        __try { return ((t_get_child_std)p_get_child)(transform, index); }
        __except (1) { return nullptr; }
    }

    void GetObjectNameSafe(void* obj, char* outBuf, size_t bufSize) {
        outBuf[0] = '\0';
        if (!obj || !p_GetName) return;
        __try {
            Il2CppString* strObj = nullptr;
            if (is_inj_name) {
                ((t_GetName_inj)p_GetName)(obj, &strObj);
            }
            else {
                strObj = ((t_GetName_std)p_GetName)(obj);
            }

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

    std::string GetObjectName(void* obj) {
        char buf[256];
        GetObjectNameSafe(obj, buf, sizeof(buf));
        return std::string(buf);
    }

    bool GetPosition(void* transform, Vector3& outPos) {
        if (!transform || !p_get_position) return false;
        __try { ((t_get_position)p_get_position)(transform, &outPos); return true; }
        __except (1) { return false; }
    }

    void SetPosition(void* transform, Vector3 pos) {
        if (!transform || !p_set_position) return;
        __try { ((t_set_position)p_set_position)(transform, &pos); }
        __except (1) {}
    }

    Il2CppObject* SafeInvokeActiveScene(const MethodInfo* method) {
        __try {
            return il2cpp_runtime_invoke(method, nullptr, nullptr, nullptr);
        }
        __except (1) {
            Log("[CRASH] Phat hien Crash tai Invoke GetActiveScene!");
            return nullptr;
        }
    }

    Il2CppObject* SafeInvokeRootObjects(const MethodInfo* method, void* unboxedScene) {
        __try {
            return il2cpp_runtime_invoke(method, unboxedScene, nullptr, nullptr);
        }
        __except (1) {
            Log("[CRASH] Phat hien Crash tai Invoke GetRootGameObjects!");
            return nullptr;
        }
    }

    
}