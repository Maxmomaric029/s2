#include "MemoryUtils.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Definición única de la variable estática
uintptr_t MemoryUtils::libBase = 0;

uintptr_t MemoryUtils::getLibraryBase(const char* libName) {
    // Si ya está cacheada, no releer /proc/self/maps
    if (libBase != 0) return libBase;

    FILE* fp = fopen("/proc/self/maps", "rt");
    if (!fp) return 0;

    char line[512];
    uintptr_t baseAddr = 0;

    while (fgets(line, sizeof(line), fp)) {
        // Buscamos el primer segmento ejecutable (r-xp) de la librería
        if (strstr(line, libName) && strstr(line, "r-xp")) {
            baseAddr = static_cast<uintptr_t>(strtoul(line, nullptr, 16));
            break;
        }
    }

    fclose(fp);
    libBase = baseAddr;
    return baseAddr;
}
