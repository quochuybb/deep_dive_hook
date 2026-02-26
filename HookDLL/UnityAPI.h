#pragma once
#include "IL2CPP_Resolver.h"
#include <string>

namespace Unity {
    struct Vector3 {
        float x, y, z;
    };

    extern bool Init();

    void* GetGameObject(void* transform);
    void* GetTransform(void* gameObject);
    void* GetParent(void* transform);
    int GetChildCount(void* transform);
    void* GetChild(void* transform, int index);
    std::string GetObjectName(void* obj);
    Il2CppArray* GetSceneRootObjects();
    bool GetPosition(void* transform, Vector3& outPos);
    void SetPosition(void* transform, Vector3 pos);
}