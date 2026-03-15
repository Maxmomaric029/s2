#ifndef ANDROID_MOD_MENU_MEMORYUTILS_H
#define ANDROID_MOD_MENU_MEMORYUTILS_H

#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>

class MemoryUtils {
public:
    static uintptr_t libBase;

    // Lectura directa — el .so está inyectado en FF, mismo proceso
    static bool IsValidPtr(uintptr_t addr) {
        return addr > 0x10000 && addr < (uintptr_t)0x7FFFFFFFFFFF;
    }

    template <typename T>
    static T Read(uintptr_t address) {
        T result{};
        if (!IsValidPtr(address)) return result;
        memcpy(&result, reinterpret_cast<void*>(address), sizeof(T));
        return result;
    }

    template <typename T>
    static void Write(uintptr_t address, T value) {
        if (!IsValidPtr(address)) return;
        size_t ps = static_cast<size_t>(getpagesize());
        uintptr_t start = address & ~(ps - 1);
        size_t len = ((address + sizeof(T) - 1) & ~(ps-1)) - start + ps;
        mprotect(reinterpret_cast<void*>(start), len,
                 PROT_READ | PROT_WRITE | PROT_EXEC);
        memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
        __builtin___clear_cache(
            reinterpret_cast<char*>(address),
            reinterpret_cast<char*>(address + sizeof(T)));
    }

    static uintptr_t getLibraryBase(const char* libName) {
        if (libBase != 0) return libBase;
        FILE* fp = fopen("/proc/self/maps", "rt");
        if (!fp) return 0;
        char line[512];
        uintptr_t base = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, libName) && strstr(line, "r-xp")) {
                base = static_cast<uintptr_t>(strtoul(line, nullptr, 16));
                break;
            }
        }
        fclose(fp);
        libBase = base;
        return base;
    }
};

#endif#ifndef ANDROID_MOD_MENU_MEMORYUTILS_H
#define ANDROID_MOD_MENU_MEMORYUTILS_H

#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>

class MemoryUtils {
public:
    static uintptr_t libBase;

    // Lectura directa — el .so está inyectado en FF, mismo proceso
    static bool IsValidPtr(uintptr_t addr) {
        return addr > 0x10000 && addr < (uintptr_t)0x7FFFFFFFFFFF;
    }

    template <typename T>
    static T Read(uintptr_t address) {
        T result{};
        if (!IsValidPtr(address)) return result;
        memcpy(&result, reinterpret_cast<void*>(address), sizeof(T));
        return result;
    }

    template <typename T>
    static void Write(uintptr_t address, T value) {
        if (!IsValidPtr(address)) return;
        size_t ps = static_cast<size_t>(getpagesize());
        uintptr_t start = address & ~(ps - 1);
        size_t len = ((address + sizeof(T) - 1) & ~(ps-1)) - start + ps;
        mprotect(reinterpret_cast<void*>(start), len,
                 PROT_READ | PROT_WRITE | PROT_EXEC);
        memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
        __builtin___clear_cache(
            reinterpret_cast<char*>(address),
            reinterpret_cast<char*>(address + sizeof(T)));
    }

    static uintptr_t getLibraryBase(const char* libName) {
        if (libBase != 0) return libBase;
        FILE* fp = fopen("/proc/self/maps", "rt");
        if (!fp) return 0;
        char line[512];
        uintptr_t base = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, libName) && strstr(line, "r-xp")) {
                base = static_cast<uintptr_t>(strtoul(line, nullptr, 16));
                break;
            }
        }
        fclose(fp);
        libBase = base;
        return base;
    }
};

#endif
