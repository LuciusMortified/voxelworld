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
        #
        # C4324 — «структура дополнена из-за описателя выравнивания» — сообщает
        # ровно о том, чего от alignas и добивались: раскладка UBO обязана
        # совпасть с std140, и паддинг там не побочный эффект, а цель. Проверяют
        # её static_assert на offsetof в render_uniforms.cppm, а не компилятор.
        # Гасить приходится флагом: предупреждение всплывает не в точке
        # определения, а у каждого потребителя, импортирующего структуру, и
        # прагма внутри модуля до них не доходит.
        set(flags /W4 /permissive- /wd4324)
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

            # Из -Wextra, начиная с Clang 19: требует перечислять все поля там,
            # где инициализация идёт по именам. Но назначенные инициализаторы
            # затем и берут, чтобы назвать отличия от умолчаний, — описания
            # состояний в arena перечисляют по три поля из десятка сознательно.
            # Позиционная форма под -Wmissing-field-initializers остаётся: там
            # умолчание не видно по месту, и молчаливый пропуск поля — ошибка.
            -Wno-missing-designated-field-initializers
        )
        if(VW_WARNINGS_AS_ERRORS)
            list(APPEND flags -Werror)
        endif()
    endif()

    target_compile_options(${target} PRIVATE ${flags})
endfunction()
