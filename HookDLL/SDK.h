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