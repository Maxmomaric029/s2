#ifndef INLINE_HOOK_H
#define INLINE_HOOK_H

// ============================================================
//  InlineHook — ARM64 inline hook sin dependencias externas
//  Trampoline de 16 bytes: LDR X17, #8 / BR X17 / .quad addr
// ============================================================

#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <android/log.h>

#define IH_LOG(...) __android_log_print(ANDROID_LOG_DEBUG,"InlineHook",__VA_ARGS__)

namespace InlineHook {

static constexpr size_t PATCH_SIZE = 16;

static bool MakeRWX(uintptr_t addr, size_t len) {
    size_t ps = static_cast<size_t>(getpagesize());
    uintptr_t start = addr & ~(ps - 1);
    size_t    size  = ((addr + len - 1) & ~(ps - 1)) - start + ps;
    return mprotect(reinterpret_cast<void*>(start), size,
                    PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
}

static void Flush(uintptr_t addr, size_t len) {
    __builtin___clear_cache(reinterpret_cast<char*>(addr),
                            reinterpret_cast<char*>(addr + len));
}

// Instala hook ARM64. Retorna 0 en éxito, -1 en error.
static int Hook(void* func, void* replace, void** origin) {
    if (!func || !replace || !origin) return -1;

    uintptr_t target = reinterpret_cast<uintptr_t>(func);
    uintptr_t hook   = reinterpret_cast<uintptr_t>(replace);

    // Alocar región RWX para el trampoline (saved bytes + LDR/BR + return addr)
    // Layout: [16 bytes saved] [LDR X17,#8 (4)] [BR X17 (4)] [ret_addr (8)]
    const size_t TR_SIZE = PATCH_SIZE + 4 + 4 + 8;
    uint8_t* tr = reinterpret_cast<uint8_t*>(
        mmap(nullptr, TR_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (tr == MAP_FAILED) { IH_LOG("mmap failed"); return -1; }

    if (!MakeRWX(target, PATCH_SIZE)) {
        IH_LOG("mprotect failed"); munmap(tr, TR_SIZE); return -1;
    }

    // 1. Salvar instrucciones originales
    memcpy(tr, reinterpret_cast<void*>(target), PATCH_SIZE);

    // 2. Trampoline: ejecutar instrucciones salvadas, luego saltar a target+16
    uintptr_t retAddr = target + PATCH_SIZE;
    uint32_t ldr = 0x58000051u;  // LDR X17, #8
    uint32_t br  = 0xD61F0220u;  // BR  X17
    memcpy(tr + PATCH_SIZE + 0, &ldr, 4);
    memcpy(tr + PATCH_SIZE + 4, &br,  4);
    memcpy(tr + PATCH_SIZE + 8, &retAddr, 8);
    Flush(reinterpret_cast<uintptr_t>(tr), TR_SIZE);

    // 3. Parche en función original: LDR X17,#8 / BR X17 / .quad hook
    uint32_t patch[2] = { 0x58000051u, 0xD61F0220u };
    memcpy(reinterpret_cast<void*>(target),     patch, 8);
    memcpy(reinterpret_cast<void*>(target + 8), &hook, 8);
    Flush(target, PATCH_SIZE);

    *origin = reinterpret_cast<void*>(tr);
    IH_LOG("Hook %p -> %p trampoline=%p", func, replace, tr);
    return 0;
}

} // namespace InlineHook

// Drop-in replacement para DobbyHook
static inline int DobbyHook(void* addr, void* replace, void** origin) {
    return InlineHook::Hook(addr, replace, origin);
}

#endif // INLINE_HOOK_H
