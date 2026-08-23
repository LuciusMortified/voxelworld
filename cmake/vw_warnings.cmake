# Общий набор предупреждений для кода проекта.
#
# Применяется только к нашим таргетам: у зависимостей свои представления о
# чистоте сборки, и чинить их предупреждения мы не можем. PRIVATE — чтобы набор
# не протекал в потребителей библиотеки.

option(VW_WARNINGS_AS_ERRORS "Turn warnings into errors" OFF)

function(vw_set_warnings target)
    # Развилка по синтаксису драйвера, а не по ABI: clang++ на Windows целится
    # в MSVC, но ключи принимает только в GNU-написании.
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # /permissive- нужен не меньше самих предупреждений: без него MSVC
        # принимает код, который другие компиляторы отвергают.
        set(flags /W4 /permissive-)
        if(VW_WARNINGS_AS_ERRORS)
            list(APPEND flags /WX)
        endif()
    else()
        # -Wdouble-promotion в набор не входит: генератор шума считает в float64
        # намеренно, и предупреждение отмечало бы этот замысел десятками строк.
        set(flags
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow              # затенение переменных — источник тихих правок не того объекта
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Woverloaded-virtual
            -Wformat=2
            -Wimplicit-fallthrough
        )
        if(VW_WARNINGS_AS_ERRORS)
            list(APPEND flags -Werror)
        endif()
    endif()

    target_compile_options(${target} PRIVATE ${flags})
endfunction()
