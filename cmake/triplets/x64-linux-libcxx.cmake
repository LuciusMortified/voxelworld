# Штатный x64-linux, но зависимости собираются той же стандартной библиотекой,
# что и движок.
#
# Иначе glfw3, imgui и Catch2 приедут собранными поверх libstdc++, тогда как
# движок идёт на libc++. Чистый C пережил бы и смешение, Catch2 — нет: он несёт
# std::string через границу библиотеки.
#
# Компилятор здесь не назначается. Он приходит из CC и CXX, и это не лень, а
# единственный способ не потерять всё, что делает штатный
# scripts/toolchains/linux.cmake: VCPKG_CHAINLOAD_TOOLCHAIN_FILE замещает его
# целиком, вместе с -fPIC, CMAKE_SYSTEM_NAME, снятым CMAKE_CROSSCOMPILING и
# четырьмя policy. Свой файл всё это отнимает у портов, а флаги ниже штатный
# тулчейн подхватывает сам.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_CXX_FLAGS "-stdlib=libc++")
set(VCPKG_LINKER_FLAGS "-stdlib=libc++")
