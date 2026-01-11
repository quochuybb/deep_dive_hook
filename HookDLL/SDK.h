#pragma once
struct Vector2 { float x, y; };

struct Damageable {
    char pad_0000[0x58];
    float m_Health; // 0x58
};

struct PlayerDamageable : Damageable {
    char pad_005C[0x1C];
    float m_Oxygen; // 0x78
};