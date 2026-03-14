cmake_minimum_required(VERSION 3.10.2)

project("modmenu")

# --- FLAGS DE COMPILACIÓN ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fexceptions -frtti")

# --- DIRECTORIOS DE CABECERA ---
include_directories(
    src/
    dobby/include/
)

# --- FUENTES DEL MOD ---
add_library(modmenu SHARED
    src/Main.cpp
    src/MemoryUtils.cpp
)

# --- DEFINICIÓN DE RUTA DE DOBBY ---
set(DOBBY_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dobby/${ANDROID_ABI}/libdobby.a)

# --- LIBRERÍAS DE ANDROID ---
find_library(log-lib log)
find_library(android-lib android)

# --- LINKER (Forzado) ---
target_link_libraries(modmenu
    ${log-lib}
    ${android-lib}
    ${DOBBY_LIB_PATH} # Pasamos el archivo .a directamente
    atomic
    m
)
