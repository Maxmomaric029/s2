#ifndef ANDROID_MOD_MENU_MENU_H
#define ANDROID_MOD_MENU_MENU_H

class Menu {
public:
    // C++17 inline static — sin errores de definición múltiple
    inline static bool aimbot    = false;
    inline static bool esp       = false;
    inline static bool silentAim = false;
    inline static bool noRecoil  = false;
    inline static bool fovCircle = false;

    // Configuración del aimbot
    inline static float fov       = 180.0f; // Radio de búsqueda en píxeles
    inline static float smoothing = 5.0f;   // 1.0 = instantáneo, mayor = más suave
};

#endif // ANDROID_MOD_MENU_MENU_H
