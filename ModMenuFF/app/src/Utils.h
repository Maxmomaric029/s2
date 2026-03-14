#ifndef ANDROID_MOD_MENU_UTILS_H
#define ANDROID_MOD_MENU_UTILS_H

#include <cmath>

struct Vector3 {
    float x, y, z;
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    static float Distance(Vector3 a, Vector3 b) {
        return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
    }
    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
};

struct Quaternion {
    float x, y, z, w;
    static Quaternion LookRotation(Vector3 forward) {
        float mag = sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
        if (mag < 0.001f) return {0,0,0,1};
        forward.x /= mag; forward.y /= mag; forward.z /= mag;
        float yaw = atan2(forward.x, forward.z);
        float pitch = asin(-forward.y);
        float cy = cos(yaw * 0.5f);
        float sy = sin(yaw * 0.5f);
        float cp = cos(pitch * 0.5f);
        float sp = sin(pitch * 0.5f);
        return {cy * sp, sy * cp, -sy * sp, cy * cp};
    }
};

#endif
