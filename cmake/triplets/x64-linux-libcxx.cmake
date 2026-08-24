# Штатный x64-linux, но зависимости собираются тем же Clang с libc++, каким
# собирается движок.
#
# Иначе glfw3, imgui и Catch2 приедут собранными системным gcc поверх libstdc++,
# и линковка разъедется на всём, что несёт std-типы через границу библиотеки.
# Это не догадка: ровно это уже случилось с предсобранным Catch2 под ASan.
# Чистый C (glfw3, libvulkan) пережил бы и смешение, Catch2 — нет.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../toolchains/linux-libcxx.cmake")
