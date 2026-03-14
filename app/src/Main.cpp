#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <android/log.h>

#include "Global.h"
#include "Utils.h"
#include "MemoryUtils.h"
#include "Menu.h"
#include "ESP.h"

// --- DOBBY HOOK EXTERNAL (Solo incluir el header ya que tiene extern "C") ---
#include "dobby.h"

// Inicialización de la base estática
uintptr_t MemoryUtils::libBase = 0;

// Punteros de función del juego (Resolución dinámica)
typedef void* (*t_GetTransform)(void* obj);
typedef Vector3 (*t_GetPosition)(void* transform);
typedef Vector3 (*t_WorldToScreen)(void* camera, Vector3 worldPos);
typedef void* (*t_GetMainCamera)();

t_GetTransform Component_GetTransform;
t_GetPosition Transform_INTERNAL_GetPosition;
t_WorldToScreen WorldToScreenPoint;
t_GetMainCamera Camera_main;

// --- LÓGICA DE AIMBOT ---
void ApplyAimbot(void* localPlayer, Vector3 targetPos) {
    if (!localPlayer || !Component_GetTransform || !Transform_INTERNAL_GetPosition) return;
    
    void* tf = Component_GetTransform(localPlayer);
    Vector3 myPos = Transform_INTERNAL_GetPosition(tf);
    Vector3 diff = { targetPos.x - myPos.x, targetPos.y - myPos.y, targetPos.z - myPos.z };
    
    // Escribimos los 16 bytes del Quaternion completo para evitar corrupción
    Quaternion rotation = Quaternion::LookRotation(diff);
    MemoryUtils::Write<Quaternion>((uintptr_t)localPlayer + Global.AimRotation, rotation);
}

// --- HOOK DE PLAYER UPDATE ---
void (*old_Player_Update)(void* instance);
void hook_Player_Update(void* instance) {
    if (instance) {
        // 1. NO RECOIL (Data Patch cada frame)
        if (Menu::noRecoil) {
            uintptr_t weapon = MemoryUtils::Read<uintptr_t>((uintptr_t)instance + Global.Weapon);
            if (weapon) {
                uintptr_t data = MemoryUtils::Read<uintptr_t>(weapon + Global.WeaponData);
                if (data) MemoryUtils::Write<float>(data + Global.WeaponRecoil, 0.0f);
            }
        }

        // 2. SILENT AIM (Weapon Offsets)
        if (Menu::silentAim) {
            uintptr_t weapon = MemoryUtils::Read<uintptr_t>((uintptr_t)instance + Global.Weapon);
            if (weapon) {
                MemoryUtils::Write<float>(weapon + Global.sAim1, 0.0f); 
            }
        }

        // 3. ESP & AIMBOT LOOP
        if (Menu::aimbot || Menu::esp) {
             // Aquí iría la lógica de iteración de enemigos (Dictionary 0x44)
             // Y la llamada a EspBridge::PushEspData(vector_entities);
        }
    }
    // Llamada original obligatoria
    if (old_Player_Update) old_Player_Update(instance);
}

// --- HILO DE INYECCIÓN ---
void *hack_thread(void *arg) {
    JavaVM* vm = (JavaVM*)arg;

    // Esperar a que libil2cpp esté cargada
    while (!(MemoryUtils::libBase = MemoryUtils::getLibraryBase("libil2cpp.so"))) {
        usleep(500000); // 500ms
    }

    uintptr_t base = MemoryUtils::libBase;

    // Resolver direcciones de funciones de Unity
    Component_GetTransform = (t_GetTransform)(base + Global.Component_GetTransform);
    Transform_INTERNAL_GetPosition = (t_GetPosition)(base + Global.Transform_INTERNAL_GetPosition);
    WorldToScreenPoint = (t_WorldToScreen)(base + Global.WorldToScreenPoint);
    Camera_main = (t_GetMainCamera)(base + Global.Camera_main);

    // INSTALAR HOOKS REALES
    // DEBES REEMPLAZAR ESTE OFFSET CON EL REAL DE TU DUMP (ej: Player.Update)
    uintptr_t Player_Update_Offset = 0x1A2B3C; 

    DobbyHook((void*)(base + Player_Update_Offset), (void*)hook_Player_Update, (void**)&old_Player_Update);

    return nullptr;
}

// --- JNI INTERFACE ---
extern "C" JNIEXPORT void JNICALL
Java_com_dts_freefiremax_NativeLib_toggleFeature(JNIEnv *env, jclass type, jint id, jboolean val) {
    switch(id) {
        case 0: Menu::aimbot = val; break;
        case 1: Menu::esp = val; break;
        case 2: Menu::silentAim = val; break;
        case 3: Menu::noRecoil = val; break;
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;

    // Inicializar Bridge de ESP
    EspBridge::Init(env, vm);

    // Lanzar hilo de hacks
    pthread_t pt;
    pthread_create(&pt, nullptr, hack_thread, (void*)vm);

    return JNI_VERSION_1_6;
}
