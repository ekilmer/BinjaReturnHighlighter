# This triplet file likely won't work for non-CMake packages
# See https://github.com/microsoft/vcpkg/discussions/19454
# Also a script https://gitlab.com/kelteseth/ScreenPlay/-/blob/4c9ab644ecc712a524ce27070a83b4b54294e9b7/Tools/macos_make_universal.py

set(VCPKG_TARGET_ARCHITECTURE universal)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES "arm64;x86_64")

set(VCPKG_OSX_DEPLOYMENT_TARGET 11.0)

set(VCPKG_BUILD_TYPE release)
