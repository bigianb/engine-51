@echo off
mkdir build_vs2026
pushd build_vs2026
cmake -G "Visual Studio 18 2026" -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\msvc2022_64 ..
popd
