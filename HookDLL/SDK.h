#pragma once
#include <string>
#include <Windows.h>
struct Vector2 { float x, y; };

struct Damageable {
    char pad_0000[0x58];
    float m_Health; // 0x58
};

struct PlayerDamageable : Damageable {
    char pad_005C[0x1C];
    float m_Oxygen; // 0x78
};
struct Il2CppString {
    void* klass;        
    void* monitor;      
    int length;         
    wchar_t chars[1];   
};
inline std::string UseString(Il2CppString* str) {
    if (!str) return "[NULL]";

    wchar_t* wstr = str->chars;
    int len = str->length;

    if (len <= 0) return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, len, NULL, 0, NULL, NULL);

    std::string result(size_needed, 0);

    WideCharToMultiByte(CP_UTF8, 0, wstr, len, &result[0], size_needed, NULL, NULL);

    return result;
}
template <typename T>
struct UnityArray {
    void* klass;          // 0x00

    void* monitor;        // 0x08

    void* bounds;         // 0x10

    uint32_t max_length;  // 0x18 

    uint32_t pad_001C;

    T m_Items[65535];     // 0x20

    T Get(int index) {
        if (index < 0 || index >= max_length) return nullptr;
        return m_Items[index];
    }

    int GetSize() {
        return max_length;
    }
};

struct EnemySDK {
    char pad[0x100];
};

struct RoomEventStateDataSDK {
    char pad_0000[0x88];

    UnityArray<EnemySDK*>* Enemies;
};

struct RoomSDK {
    char pad_0000[0x50];

    RoomEventStateDataSDK* m_CurrentRoomData;
};
struct Vector3 { float x, y, z; };

struct TransformInternal {
    char pad_0000[0x38]; 
};
struct Bounds {
    Vector3 center;  
    Vector3 extents; 
};
