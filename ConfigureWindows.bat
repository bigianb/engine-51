@echo off
mkdir build_vs2022
pushd build_vs2022
cmake -G "Visual Studio 17 2022" -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DCMAKE_PREFIX_PATH=C:\Qt\6.8.0\msvc2022_64 ..
popd
