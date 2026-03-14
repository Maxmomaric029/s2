#ifndef ANDROID_MOD_MENU_ESP_H
#define ANDROID_MOD_MENU_ESP_H

#include <jni.h>
#include <vector>
#include "Utils.h"

struct EspEntry {
    Vector3 screenHead;
    Vector3 screenFoot;
    Vector3 screenHip;
    Vector3 screenChest;
    float   distance;
    char    name[64];
    float   hp;
    bool    visible;
};

namespace EspBridge {
    static JavaVM*    gJvm         = nullptr;
    static jclass     gEspClass    = nullptr;
    static jmethodID  gUpdateMethod = nullptr;

    inline void Init(JNIEnv* env, JavaVM* vm) {
        gJvm = vm;
        jclass local = env->FindClass("com/dts/freefiremax/EspRenderer");
        if (!local) return;
        gEspClass    = reinterpret_cast<jclass>(env->NewGlobalRef(local));
        gUpdateMethod = env->GetStaticMethodID(gEspClass, "updateEspData", "([F)V");
    }

    inline void PushEspData(const std::vector<EspEntry>& entries) {
        if (!gJvm || !gEspClass || !gUpdateMethod || entries.empty()) return;

        JNIEnv* env = nullptr;
        bool attached = false;
        int status = gJvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            gJvm->AttachCurrentThread(&env, nullptr);
            attached = true;
        }
        if (!env) return;

        const int FLOATS_PER_ENTRY = 9;
        jsize count = static_cast<jsize>(entries.size() * FLOATS_PER_ENTRY);
        jfloatArray arr = env->NewFloatArray(count);
        if (!arr) { if (attached) gJvm->DetachCurrentThread(); return; }

        std::vector<float> buf;
        buf.reserve(count);
        for (const auto& e : entries) {
            buf.push_back(e.screenHead.x);
            buf.push_back(e.screenHead.y);
            buf.push_back(e.screenFoot.x);
            buf.push_back(e.screenFoot.y);
            buf.push_back(e.screenHip.x);
            buf.push_back(e.screenHip.y);
            buf.push_back(e.distance);
            buf.push_back(e.hp);
            buf.push_back(e.visible ? 1.0f : 0.0f);
        }

        env->SetFloatArrayRegion(arr, 0, count, buf.data());
        env->CallStaticVoidMethod(gEspClass, gUpdateMethod, arr);
        env->DeleteLocalRef(arr);

        if (attached) gJvm->DetachCurrentThread();
    }
}

#endif
