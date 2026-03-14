#ifndef ANDROID_MOD_MENU_GLOBAL_H
#define ANDROID_MOD_MENU_GLOBAL_H

#include <stdint.h>

// ============================================================
//  OFFSETS — Free Fire MAX  (ARM64 / libil2cpp.so)
//  Versión : PLAY (Google Play)
//  Arch    : ARM64
//  NOTA    : Para cambiar de versión, reemplaza los valores
//            usando la columna correspondiente del dump.
// ============================================================

struct GlobalOffsets {

    // ── InitBase (base del singleton principal) ───────────────
    // Dirección absoluta en libil2cpp.so, NO es un offset de campo
    uintptr_t InitBase = 0xA3F438C;

    // ── Singleton / Match ─────────────────────────────────────
    uintptr_t StaticClass    = 0x5C;
    uintptr_t CurrentMatch   = 0x50;
    uintptr_t MatchStatus    = 0x74;

    // ── Jugador ───────────────────────────────────────────────
    uintptr_t LocalPlayer        = 0x7C;
    uintptr_t DictionaryEntities = 0x68;   // lista de todos los jugadores
    uintptr_t Player_IsDead      = 0x4C;   // bool
    uintptr_t Player_Name        = 0x28C;  // Il2CppString*
    uintptr_t Player_Data        = 0x44;   // ptr a datos secundarios del jugador
    uintptr_t IsBot              = 0x294;  // bool (relativo a Player_Data)

    // ── Avatar (modelo 3D / huesos) ───────────────────────────
    uintptr_t AvatarManager     = 0x460;
    uintptr_t AvatarPropManager = 0x468;
    uintptr_t Avatar            = 0x94;
    uintptr_t AvatarData        = 0x10;
    uintptr_t Avatar_IsVisible  = 0x7C;   // bool
    uintptr_t Avatar_IsTeam     = 0x51;   // bool

    // ── Arma ──────────────────────────────────────────────────
    uintptr_t Weapon       = 0x39C;
    uintptr_t WeaponData   = 0x58;
    uintptr_t WeaponRecoil = 0x0C;   // float
    uintptr_t WeaponName   = 0x38;   // Il2CppString*
    uintptr_t NoReload     = 0x91;   // bool

    // ── Rotación / Apuntado ───────────────────────────────────
    uintptr_t AimRotation = 0x3A8;   // Quaternion (16 bytes)

    // ── Cámara ────────────────────────────────────────────────
    uintptr_t MainCameraTransform = 0x1FC;
    uintptr_t FollowCamera        = 0x3F0;
    uintptr_t Camera              = 0x14;
    uintptr_t ViewMatrix          = 0xBC;   // Matrix4x4 (64 bytes)

    // ── Misc ──────────────────────────────────────────────────
    uintptr_t Player_ShadowBase  = 0x15E8;
    uintptr_t XPose              = 0x78;
    uintptr_t PlayerAttributes   = 0x45C;

    // ── Silent Aim ────────────────────────────────────────────
    uintptr_t sAim1 = 0x4E0;   // relativo al jugador → ptr a ShootManager
    uintptr_t sAim2 = 0x8F0;   // relativo a ShootManager → ptr a BulletData
    uintptr_t sAim3 = 0x38;    // relativo a BulletData → dirección X
    uintptr_t sAim4 = 0x2C;    // relativo a BulletData → dirección Y

    // ── Huesos (bone transforms, relativos al Avatar) ─────────
    uintptr_t Head          = 0x3F8;
    uintptr_t Spinex        = 0x400;
    uintptr_t Hip           = 0x3FC;
    uintptr_t Root          = 0x40C;
    uintptr_t LeftWrist     = 0x3F4;
    uintptr_t RightCalf     = 0x410;
    uintptr_t LeftCalf      = 0x414;
    uintptr_t RightFoot     = 0x418;
    uintptr_t LeftFoot      = 0x41C;
    uintptr_t RightWrist    = 0x420;
    uintptr_t LeftHand      = 0x424;
    uintptr_t LeftShoulder  = 0x42C;
    uintptr_t RightShoulder = 0x430;
    uintptr_t RightWristJoint = 0x434;
    uintptr_t LeftWristJoint  = 0x438;
    uintptr_t LeftElbow       = 0x43C;
    uintptr_t RightElbow      = 0x440;
    uintptr_t HeadCollider    = 0x444;

    // ── Hook principal ────────────────────────────────────────
    // [PENDIENTE] Offset de Player.Update() o LateUpdate()
    // Buscá en el dump: "CharacterUpdate" o "PlayerController$$Update"
    // Dejalo en 0x0 hasta tenerlo — el hook se omite de forma segura
    uintptr_t Player_Update_Func = 0x0;

} Global;

#endif // ANDROID_MOD_MENU_GLOBAL_H
