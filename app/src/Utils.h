#ifndef ANDROID_MOD_MENU_UTILS_H
#define ANDROID_MOD_MENU_UTILS_H

#include <cmath>

struct Vector3 {
    float x, y, z;
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }

    float magnitude() const { return sqrtf(x*x + y*y + z*z); }

    Vector3 normalized() const {
        float m = magnitude();
        if (m < 0.0001f) return {0, 0, 0};
        return {x/m, y/m, z/m};
    }

    static float Distance(const Vector3& a, const Vector3& b) {
        return (a - b).magnitude();
    }

    static Vector3 Cross(const Vector3& a, const Vector3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    static float Dot(const Vector3& a, const Vector3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }
};

struct Quaternion {
    float x, y, z, w;
    Quaternion(float x = 0, float y = 0, float z = 0, float w = 1)
        : x(x), y(y), z(z), w(w) {}

    // Construcción correcta desde base ortonormal (forward, up)
    // Equivalente a Unity's Quaternion.LookRotation
    static Quaternion LookRotation(Vector3 forward, Vector3 up = {0, 1, 0}) {
        forward = forward.normalized();
        if (forward.magnitude() < 0.0001f) return {0, 0, 0, 1};

        Vector3 right = Vector3::Cross(up, forward).normalized();
        // Recalcular up para que sea ortogonal
        Vector3 newUp = Vector3::Cross(forward, right);

        // Construir quaternion desde matriz de rotación 3x3
        // Columnas: right, newUp, forward
        float m00 = right.x,   m01 = right.y,   m02 = right.z;
        float m10 = newUp.x,   m11 = newUp.y,   m12 = newUp.z;
        float m20 = forward.x, m21 = forward.y, m22 = forward.z;

        float trace = m00 + m11 + m22;
        Quaternion q;
        if (trace > 0.0f) {
            float s = 0.5f / sqrtf(trace + 1.0f);
            q.w = 0.25f / s;
            q.x = (m12 - m21) * s;
            q.y = (m20 - m02) * s;
            q.z = (m01 - m10) * s;
        } else if (m00 > m11 && m00 > m22) {
            float s = 2.0f * sqrtf(1.0f + m00 - m11 - m22);
            q.w = (m12 - m21) / s;
            q.x = 0.25f * s;
            q.y = (m01 + m10) / s;
            q.z = (m20 + m02) / s;
        } else if (m11 > m22) {
            float s = 2.0f * sqrtf(1.0f + m11 - m00 - m22);
            q.w = (m20 - m02) / s;
            q.x = (m01 + m10) / s;
            q.y = 0.25f * s;
            q.z = (m12 + m21) / s;
        } else {
            float s = 2.0f * sqrtf(1.0f + m22 - m00 - m11);
            q.w = (m01 - m10) / s;
            q.x = (m20 + m02) / s;
            q.y = (m12 + m21) / s;
            q.z = 0.25f * s;
        }
        return q;
    }
};

#endif // ANDROID_MOD_MENU_UTILS_H
