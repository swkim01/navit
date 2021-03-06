set(CMAKE_SYSTEM_NAME GNU)

set(ANDROID TRUE)
set(ANDROID_API_VERSION 9 CACHE STRING "Andriod API Version")
set(ANDROID_NDK_API_VERSION ${ANDROID_API_VERSION} CACHE STRING "Andriod NDK API Version")
set(ANDROID_ARCH "x86" CACHE STRING "Android architecture")

find_program(CMAKE_C_COMPILER NAMES i686-android-linux-gcc i686-android-linux-gcc.exe)
find_program(CMAKE_CXX_COMPILER NAMES i686-android-linux-g++ i686-android-linux-g++.exe)
set(PKG_CONFIG_EXECUTABLE "i686-android-pkg-config")

get_filename_component(COMPILER_PATH ${CMAKE_C_COMPILER} PATH)

set(ANDROID_NDK "${COMPILER_PATH}/../../../../.." CACHE STRING "PATH to Andriod NDK")

set(NDK_ARCH_DIR "${ANDROID_NDK}/platforms/android-${ANDROID_NDK_API_VERSION}/arch-x86")
set(CMAKE_FIND_ROOT_PATH ${NDK_ARCH_DIR})

set(CMAKE_REQUIRED_FLAGS "-nostdlib -lc -ldl -lgcc -L${NDK_ARCH_DIR}/usr/lib")
set(CMAKE_REQUIRED_INCLUDES "${NDK_ARCH_DIR}/usr/include")

set(NAVIT_COMPILE_FLAGS "-I${NDK_ARCH_DIR}/usr/include -g -D_GNU_SOURCE -DANDROID -fno-short-enums ${CMAKE_REQUIRED_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_REQUIRED_FLAGS} -Wl,--no-undefined -Wl,-rpath,/system/lib")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_REQUIRED_FLAGS} -Wl,--no-undefined")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${CMAKE_REQUIRED_FLAGS} -Wl,-rpath,/data/data/org.navitproject.navit/lib")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
