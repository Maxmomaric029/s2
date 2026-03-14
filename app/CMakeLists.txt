cmake_minimum_required(VERSION 3.22.1)
project("modmenu")

add_library(modmenu SHARED
    src/Main.cpp
    src/MemoryUtils.cpp
)

target_include_directories(modmenu PRIVATE src/)

target_compile_features(modmenu PRIVATE cxx_std_17)

target_compile_options(modmenu PRIVATE
    -fvisibility=hidden
    -fexceptions
    -frtti
    -Wno-error=format-security
    -O2
)

target_link_libraries(modmenu
    android
    log
    atomic
)