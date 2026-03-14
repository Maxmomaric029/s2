#ifndef ANDROID_MOD_MENU_GLOBAL_H
#define ANDROID_MOD_MENU_GLOBAL_H

#include <stdint.h>

struct {
    // --- CLASES ---
    uintptr_t MainCameraTransform  = 0x5A8;
    uintptr_t Dictionary           = 0x44;
    uintptr_t HeadTF               = 0x5B8;
    uintptr_t HipTF                = 0x5C0;
    
    // --- MIEMBROS JUGADOR ---
    uintptr_t LocalPlayer          = 0xB0;
    uintptr_t AimRotation          = 0x53C; // Quaternion (16 bytes)
    uintptr_t Weapon               = 0x528;
    uintptr_t WeaponData           = 0x88;
    uintptr_t WeaponRecoil         = 0x18; // float
    uintptr_t Player_IsDead        = 0x74;

    // --- FUNCIONES (DIRECCIONES ABSOLUTAS) ---
    uintptr_t Component_GetTransform          = 0x2729DC4;
    uintptr_t Transform_INTERNAL_GetPosition  = 0x2D3638C;
    uintptr_t WorldToScreenPoint              = 0x2724AA0;
    uintptr_t Camera_main                     = 0x2725090;

    // --- SILENT AIM ---
    uintptr_t sAim1 = 0x750;
} Global;

#endif
