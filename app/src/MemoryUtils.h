#ifndef ANDROID_MOD_MENU_MEMORYUTILS_H
#define ANDROID_MOD_MENU_MEMORYUTILS_H

#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/uio.h>   // process_vm_readv / process_vm_writev
#include <unistd.h>
#include <android/log.h>

#define MU_LOG(...) __android_log_print(ANDROID_LOG_DEBUG, "MemoryUtils", __VA_ARGS__)

class MemoryUtils {
public:
    // PID del proceso de Free Fire (se resuelve en hack_thread)
    static pid_t targetPid;
    // Base de libil2cpp.so dentro del proceso de Free Fire
    static uintptr_t libBase;

    // Busca el PID de Free Fire en /proc leyendo cmdline de cada proceso
    static pid_t findProcessPid(const char* packageName) {
        FILE* fp = popen("ps -A -o pid,name 2>/dev/null", "r");
        if (!fp) return 0;
        char line[256];
        pid_t result = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, packageName)) {
                result = static_cast<pid_t>(strtol(line, nullptr, 10));
                break;
            }
        }
        pclose(fp);
        return result;
    }

    // Lee la base de libil2cpp.so del proceso remoto via /proc/[pid]/maps
    static uintptr_t getRemoteLibBase(pid_t pid, const char* libName) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/maps", pid);
        FILE* fp = fopen(path, "rt");
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
        return base;
    }

    // Comprobación de puntero válido
    static bool IsValidPtr(uintptr_t addr) {
        return addr > 0x10000 && addr < (uintptr_t)0x7FFFFFFFFFFF;
    }

    // Lee sizeof(T) bytes desde el proceso remoto usando process_vm_readv
    template <typename T>
    static T Read(uintptr_t address) {
        T result{};
        if (!IsValidPtr(address) || targetPid <= 0) return result;

        struct iovec local  = { &result,                    sizeof(T) };
        struct iovec remote = { reinterpret_cast<void*>(address), sizeof(T) };
        process_vm_readv(targetPid, &local, 1, &remote, 1, 0);
        return result;
    }

    // Escribe sizeof(T) bytes en el proceso remoto usando process_vm_writev
    // NOTA: process_vm_writev solo funciona con root o si la app tiene
    // PTRACE_MODE_ATTACH (requiere VirtualXposed / GameGuardian / root).
    // Sin root, el write silenciosamente falla — el read siempre funciona
    // en Android 10+ si el proceso target es debuggable o en VirtualSpace.
    template <typename T>
    static void Write(uintptr_t address, T value) {
        if (!IsValidPtr(address) || targetPid <= 0) return;

        struct iovec local  = { &value,                         sizeof(T) };
        struct iovec remote = { reinterpret_cast<void*>(address), sizeof(T) };
        process_vm_writev(targetPid, &local, 1, &remote, 1, 0);
    }

    // Mantener compatibilidad con getLibraryBase (ahora busca en proceso remoto)
    static uintptr_t getLibraryBase(const char* libName) {
        if (libBase != 0) return libBase;
        if (targetPid <= 0) return 0;
        libBase = getRemoteLibBase(targetPid, libName);
        return libBase;
    }
};

#endif // ANDROID_MOD_MENU_MEMORYUTILS_H
