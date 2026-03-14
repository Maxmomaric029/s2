#ifndef ANDROID_MOD_MENU_MEMORYUTILS_H
#define ANDROID_MOD_MENU_MEMORYUTILS_H

#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

class MemoryUtils {
public:
    // Base de libil2cpp.so, inicializada en hack_thread
    static uintptr_t libBase;

    // Lee la base de la librería desde /proc/self/maps
    // Solo debe llamarse una vez; el resultado se guarda en libBase
    static uintptr_t getLibraryBase(const char* libName);

    // Comprobación mínima de rango para evitar deref de null/kernel
    static bool IsValidPtr(uintptr_t addr) {
        return addr > 0x10000 && addr < (uintptr_t)0x7FFFFFFFFFFF;
    }

    // Lee sizeof(T) bytes desde address usando memcpy (seguro ante desalineación)
    template <typename T>
    static T Read(uintptr_t address) {
        T result{};
        if (!IsValidPtr(address)) return result;
        memcpy(&result, reinterpret_cast<void*>(address), sizeof(T));
        return result;
    }

    // Escribe sizeof(T) bytes en address.
    // Desprotege exactamente las páginas que cubre el write y nada más.
    template <typename T>
    static void Write(uintptr_t address, T value) {
        if (!IsValidPtr(address)) return;

        const size_t  len        = sizeof(T);
        const size_t  page_size  = static_cast<size_t>(getpagesize());
        uintptr_t     page_start = address & ~(page_size - 1);
        // Calcular cuántas páginas cubre [address, address+len)
        size_t        page_len   = ((address + len - 1) & ~(page_size - 1)) - page_start + page_size;

        mprotect(reinterpret_cast<void*>(page_start), page_len,
                 PROT_READ | PROT_WRITE | PROT_EXEC);

        memcpy(reinterpret_cast<void*>(address), &value, len);

        // Limpiar caché de instrucciones (necesario en ARM para hooks/patches)
        __builtin___clear_cache(
            reinterpret_cast<char*>(address),
            reinterpret_cast<char*>(address + len));
    }
};

#endif // ANDROID_MOD_MENU_MEMORYUTILS_H
