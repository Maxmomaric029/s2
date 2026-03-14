#ifndef ANDROID_MOD_MENU_MEMORYUTILS_H
#define ANDROID_MOD_MENU_MEMORYUTILS_H

#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

class MemoryUtils {
public:
    static uintptr_t libBase;

    static uintptr_t getLibraryBase(const char *libName) {
        FILE *fp = fopen("/proc/self/maps", "rt");
        if (!fp) return 0;
        char line[512];
        uintptr_t baseAddr = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, libName) && strstr(line, "r-xp")) {
                baseAddr = (uintptr_t)strtoul(line, NULL, 16);
                break;
            }
        }
        fclose(fp);
        return baseAddr;
    }

    template <typename T>
    static void Write(uintptr_t address, T value) {
        if (address < 0x1000) return;
        uintptr_t page_start = address & ~(getpagesize() - 1);
        mprotect((void *)page_start, getpagesize() * 2, PROT_READ | PROT_WRITE | PROT_EXEC);
        memcpy((void *)address, &value, sizeof(T));
    }

    template <typename T>
    static T Read(uintptr_t address) {
        if (address < 0x1000) return T();
        T res;
        memcpy(&res, (void *)address, sizeof(T));
        return res;
    }
};

#endif
