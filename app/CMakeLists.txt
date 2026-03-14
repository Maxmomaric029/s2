cmake_minimum_required(VERSION 3.22.1)
project("modmenu")

# ── Dobby desde fuente (NDK lo compila correctamente para arm64-v8a) ──────────
include(FetchContent)
FetchContent_Declare(
    Dobby
    GIT_REPOSITORY https://github.com/jmpews/Dobby.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   FALSE
)
set(DOBBY_BUILD_SHARED       OFF CACHE BOOL "" FORCE)
set(Plugin.Darwin.Dyld       OFF CACHE BOOL "" FORCE)
set(Plugin.Darwin.ObjectiveC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Dobby)

# ── Módulo principal ──────────────────────────────────────────────────────────
add_library(modmenu SHARED
    src/Main.cpp
    src/MemoryUtils.cpp
)

target_include_directories(modmenu PRIVATE
    src/
    ${dobby_SOURCE_DIR}/include   # provee dobby.h real
)

target_compile_features(modmenu PRIVATE cxx_std_17)

target_compile_options(modmenu PRIVATE
    -fvisibility=hidden
    -fexceptions
    -frtti
    -Wno-error=format-security
    -O2
)

target_link_libraries(modmenu
    dobby
    android
    log
    atomic
)
