#include "InlineHook.h"
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <cmath>
#include <vector>
#include <android/log.h>

#include "Global.h"
#include "Utils.h"
#include "MemoryUtils.h"
#include "Menu.h"
#include "ESP.h"

#define TAG "ModMenu"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// =============================================================================
//  VARIABLES GLOBALES
// =============================================================================
static uintptr_t gBase    = 0;
static float     gScreenW = 1280.0f;
static float     gScreenH = 720.0f;

// =============================================================================
//  CADENA DE PUNTEROS FF MAX (PLAY)
//
//  gBase + InitBase  →  [+CurrentMatch]  →  Match
//  Match             →  [+LocalPlayer]   →  Player
//  Match             →  [+DictionaryEntities] → Dict → values[]
//  Player            →  [+AvatarManager] →  AvatarMgr → [+Avatar] → Avatar
//  Avatar            →  [+Head/Hip/Foot] →  Transform* → pos en [+0x40]
//  Player            →  [+FollowCamera]  →  CamObj → [+Camera] → [+ViewMatrix]
// =============================================================================

static uintptr_t GetMatch() {
    uintptr_t sc = MemoryUtils::Read<uintptr_t>(gBase + Global.InitBase);
    if (!MemoryUtils::IsValidPtr(sc)) return 0;
    uintptr_t m  = MemoryUtils::Read<uintptr_t>(sc + Global.CurrentMatch);
    return MemoryUtils::IsValidPtr(m) ? m : 0;
}

static uintptr_t GetLocalPlayer() {
    uintptr_t match = GetMatch();
    if (!match) return 0;
    uintptr_t p = MemoryUtils::Read<uintptr_t>(match + Global.LocalPlayer);
    return MemoryUtils::IsValidPtr(p) ? p : 0;
}

static uintptr_t GetAvatar(uintptr_t player) {
    if (!MemoryUtils::IsValidPtr(player)) return 0;
    uintptr_t mgr = MemoryUtils::Read<uintptr_t>(player + Global.AvatarManager);
    if (!MemoryUtils::IsValidPtr(mgr)) return 0;
    uintptr_t av  = MemoryUtils::Read<uintptr_t>(mgr + Global.Avatar);
    return MemoryUtils::IsValidPtr(av) ? av : 0;
}

// Transform* guardado en avatar+boneOffset → posición world en [+0x40]
static Vector3 GetBonePos(uintptr_t avatar, uintptr_t boneOffset) {
    if (!MemoryUtils::IsValidPtr(avatar)) return {};
    uintptr_t tf = MemoryUtils::Read<uintptr_t>(avatar + boneOffset);
    if (!MemoryUtils::IsValidPtr(tf)) return {};
    return MemoryUtils::Read<Vector3>(tf + 0x40);
}

static Vector3 GetPlayerPos(uintptr_t player) {
    uintptr_t av = GetAvatar(player);
    if (!av) return {};
    return GetBonePos(av, Global.Hip);
}

// WorldToScreen via ViewMatrix (más estable que hookear función en FF Max)
struct Matrix4x4 { float m[4][4]; };

static Vector3 WorldToScreen(Vector3 world) {
    uintptr_t lp = GetLocalPlayer();
    if (!lp) return {};
    uintptr_t fc = MemoryUtils::Read<uintptr_t>(lp + Global.FollowCamera);
    if (!MemoryUtils::IsValidPtr(fc)) return {};
    uintptr_t cam = MemoryUtils::Read<uintptr_t>(fc + Global.Camera);
    if (!MemoryUtils::IsValidPtr(cam)) return {};
    Matrix4x4 vp = MemoryUtils::Read<Matrix4x4>(cam + Global.ViewMatrix);

    float x = vp.m[0][0]*world.x + vp.m[0][1]*world.y + vp.m[0][2]*world.z + vp.m[0][3];
    float y = vp.m[1][0]*world.x + vp.m[1][1]*world.y + vp.m[1][2]*world.z + vp.m[1][3];
    float w = vp.m[3][0]*world.x + vp.m[3][1]*world.y + vp.m[3][2]*world.z + vp.m[3][3];
    if (w < 0.0001f) return {};

    return {
        (1.0f + x / w) * 0.5f * gScreenW,
        (1.0f - y / w) * 0.5f * gScreenH,
        w
    };
}

// Leer string Il2Cpp (UTF-16 LE → ASCII)
static void ReadIl2CppString(uintptr_t ptr, char* out, size_t maxLen) {
    if (!MemoryUtils::IsValidPtr(ptr) || !out) { if (out) out[0]='\0'; return; }
    int len = MemoryUtils::Read<int>(ptr + 0x10);
    if (len <= 0 || len > 48) { out[0]='\0'; return; }
    len = len < (int)(maxLen-1) ? len : (int)(maxLen-1);
    for (int i = 0; i < len; i++) {
        uint16_t ch = MemoryUtils::Read<uint16_t>(ptr + 0x14 + i*2);
        out[i] = (char)(ch < 128 ? ch : '?');
    }
    out[len] = '\0';
}

// =============================================================================
//  AIMBOT CON SMOOTHING
// =============================================================================
static void ApplyAimbot(uintptr_t localPlayer, Vector3 targetPos) {
    if (!localPlayer) return;
    Vector3 diff = targetPos - GetPlayerPos(localPlayer);
    if (diff.magnitude() < 0.001f) return;

    Quaternion target = Quaternion::LookRotation(diff);

    if (Menu::smoothing <= 1.0f) {
        MemoryUtils::Write<Quaternion>(localPlayer + Global.AimRotation, target);
    } else {
        Quaternion cur = MemoryUtils::Read<Quaternion>(localPlayer + Global.AimRotation);
        float t = 1.0f / Menu::smoothing;
        Quaternion lerped = {
            cur.x + (target.x - cur.x) * t,
            cur.y + (target.y - cur.y) * t,
            cur.z + (target.z - cur.z) * t,
            cur.w + (target.w - cur.w) * t
        };
        float mag = sqrtf(lerped.x*lerped.x + lerped.y*lerped.y +
                          lerped.z*lerped.z + lerped.w*lerped.w);
        if (mag > 0.0001f) {
            lerped.x/=mag; lerped.y/=mag; lerped.z/=mag; lerped.w/=mag;
        }
        MemoryUtils::Write<Quaternion>(localPlayer + Global.AimRotation, lerped);
    }
}

// =============================================================================
//  SILENT AIM
//  Player → [+sAim1] → ShootMgr → [+sAim2] → BulletData
//  BulletData → [+sAim3] = dir.x, [+sAim4] = dir.y
// =============================================================================
static void ApplySilentAim(uintptr_t player, Vector3 targetPos) {
    if (!MemoryUtils::IsValidPtr(player)) return;
    uintptr_t shootMgr = MemoryUtils::Read<uintptr_t>(player + Global.sAim1);
    if (!MemoryUtils::IsValidPtr(shootMgr)) return;
    uintptr_t bulletData = MemoryUtils::Read<uintptr_t>(shootMgr + Global.sAim2);
    if (!MemoryUtils::IsValidPtr(bulletData)) return;
    Vector3 dir = (targetPos - GetPlayerPos(player)).normalized();
    MemoryUtils::Write<float>(bulletData + Global.sAim3, dir.x);
    MemoryUtils::Write<float>(bulletData + Global.sAim4, dir.y);
}

// =============================================================================
//  ITERACIÓN DE ENTIDADES + RECOLECCIÓN ESP
// =============================================================================
struct BestTarget { uintptr_t addr=0; float dist=99999.f; Vector3 headPos; };

static BestTarget ProcessEntities(uintptr_t localPlayer, std::vector<EspEntry>& esp) {
    BestTarget best;
    esp.clear();
    if (!localPlayer) return best;

    uintptr_t match = GetMatch();
    if (!match) return best;

    uintptr_t dictPtr = MemoryUtils::Read<uintptr_t>(match + Global.DictionaryEntities);
    if (!MemoryUtils::IsValidPtr(dictPtr)) return best;

    uintptr_t values = MemoryUtils::Read<uintptr_t>(dictPtr + 0x18);
    int       count  = MemoryUtils::Read<int>(dictPtr + 0x10);
    if (count <= 0 || count > 100) return best;

    Vector3 myPos = GetPlayerPos(localPlayer);

    for (int i = 0; i < count; i++) {
        uintptr_t entity = MemoryUtils::Read<uintptr_t>(values + 0x10 + i * 8);
        if (!MemoryUtils::IsValidPtr(entity) || entity == localPlayer) continue;
        if (MemoryUtils::Read<bool>(entity + Global.Player_IsDead)) continue;

        // Filtrar bots
        uintptr_t pData = MemoryUtils::Read<uintptr_t>(entity + Global.Player_Data);
        if (MemoryUtils::IsValidPtr(pData) && MemoryUtils::Read<bool>(pData + Global.IsBot)) continue;

        // Filtrar equipo
        uintptr_t avatar = GetAvatar(entity);
        if (avatar && MemoryUtils::Read<bool>(avatar + Global.Avatar_IsTeam)) continue;

        Vector3 headPos = GetBonePos(avatar ? avatar : entity, Global.Head);
        Vector3 hipPos  = GetBonePos(avatar ? avatar : entity, Global.Hip);
        Vector3 footPos = GetBonePos(avatar ? avatar : entity, Global.LeftFoot);
        float   dist    = Vector3::Distance(myPos, hipPos);

        if (Menu::esp) {
            EspEntry e{};
            e.screenHead = WorldToScreen(headPos);
            e.screenFoot = WorldToScreen(footPos);
            e.screenHip  = WorldToScreen(hipPos);
            e.distance   = dist;
            e.visible    = avatar ? MemoryUtils::Read<bool>(avatar + Global.Avatar_IsVisible) : false;
            uintptr_t namePtr = MemoryUtils::Read<uintptr_t>(entity + Global.Player_Name);
            ReadIl2CppString(namePtr, e.name, sizeof(e.name));
            if (e.screenHead.z > 0) esp.push_back(e);
        }

        if (dist < best.dist && dist < Menu::fov) {
            best.dist    = dist;
            best.addr    = entity;
            best.headPos = headPos;
        }
    }
    return best;
}

// =============================================================================
//  HOOK DE UPDATE
// =============================================================================
static void (*old_Update)(void* instance) = nullptr;

static void hook_Update(void* instance) {
    uintptr_t lp = GetLocalPlayer();

    if (lp) {
        std::vector<EspEntry> espEntries;
        BestTarget best = ProcessEntities(lp, espEntries);

        // No Recoil
        if (Menu::noRecoil) {
            uintptr_t weapon = MemoryUtils::Read<uintptr_t>(lp + Global.Weapon);
            if (MemoryUtils::IsValidPtr(weapon)) {
                uintptr_t data = MemoryUtils::Read<uintptr_t>(weapon + Global.WeaponData);
                if (MemoryUtils::IsValidPtr(data))
                    MemoryUtils::Write<float>(data + Global.WeaponRecoil, 0.0f);
            }
        }

        // Silent Aim
        if (Menu::silentAim && best.addr)
            ApplySilentAim(lp, best.headPos);

        // Aimbot
        if (Menu::aimbot && best.addr)
            ApplyAimbot(lp, best.headPos);

        // ESP → Java
        if (Menu::esp && !espEntries.empty())
            EspBridge::PushEspData(espEntries);
    }

    if (old_Update) old_Update(instance);
}

// =============================================================================
//  HILO DE INICIALIZACIÓN
// =============================================================================
static void* hack_thread(void*) {
    LOGD("Esperando libil2cpp.so...");
    while ((gBase = MemoryUtils::getLibraryBase("libil2cpp.so")) == 0) sleep(1);
    LOGD("Base: %p", reinterpret_cast<void*>(gBase));

    if (Global.Player_Update_Func == 0x0) {
        LOGE("Player_Update_Func = 0x0 en Global.h — hook omitido. Pon el offset real.");
        return nullptr;
    }

    void* addr = reinterpret_cast<void*>(gBase + Global.Player_Update_Func);
    int   res  = DobbyHook(addr,
                           reinterpret_cast<void*>(hook_Update),
                           reinterpret_cast<void**>(&old_Update));
    if (res == 0) LOGD("Hook instalado en %p", addr);
    else { LOGE("DobbyHook fallo ret=%d", res); old_Update = nullptr; }

    return nullptr;
}

// =============================================================================
//  JNI EXPORTS
// =============================================================================
extern "C" JNIEXPORT void JNICALL
Java_com_dts_freefiremax_NativeLib_toggleFeature(JNIEnv*, jclass, jint id, jboolean val) {
    switch (id) {
        case 0: Menu::aimbot    = val; break;
        case 1: Menu::esp       = val; break;
        case 2: Menu::silentAim = val; break;
        case 3: Menu::noRecoil  = val; break;
        default: break;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_dts_freefiremax_NativeLib_setAimbotParam(JNIEnv*, jclass, jint id, jfloat val) {
    if (id == 0) Menu::fov       = val;
    if (id == 1) Menu::smoothing = val;
}

extern "C" JNIEXPORT void JNICALL
Java_com_dts_freefiremax_NativeLib_setScreenSize(JNIEnv*, jclass, jint w, jint h) {
    gScreenW = static_cast<float>(w);
    gScreenH = static_cast<float>(h);
}

// =============================================================================
//  ENTRY POINT
// =============================================================================
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;
    EspBridge::Init(env, vm);
    pthread_t pt;
    pthread_create(&pt, nullptr, hack_thread, nullptr);
    return JNI_VERSION_1_6;
}
