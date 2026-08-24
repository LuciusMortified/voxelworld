# Clang с libc++ для портов vcpkg — см. triplets/x64-linux-libcxx.cmake.
#
# Суффикс версии приходит из VW_LLVM_SUFFIX (в CI это `-20`, потому что llvm.sh
# ставит именно `clang++-20` и не заводит имени без суффикса). Пустое значение
# оставляет `clang++` из PATH, что нужно для сборки с локальной машины.
#
# CMAKE_SYSTEM_NAME здесь намеренно не задаётся: сборка нативная, а объявление
# системы включило бы CMAKE_CROSSCOMPILING, и порты начали бы искать эмулятор
# для собственных вспомогательных программ.

set(_vw_llvm_suffix "$ENV{VW_LLVM_SUFFIX}")

set(CMAKE_C_COMPILER "clang${_vw_llvm_suffix}")
set(CMAKE_CXX_COMPILER "clang++${_vw_llvm_suffix}")

set(CMAKE_CXX_FLAGS_INIT "-stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-stdlib=libc++")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-stdlib=libc++")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-stdlib=libc++")
