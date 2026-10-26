cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

project(tester-uzip C CXX ASM)

# ===========================
# Пути к проекту и библиотеке
# ===========================
set(WORKSPACE_PATH     "C:/Users/A.Nitkin/MyWorkspace")
set(PROJ_ROOT          "${WORKSPACE_PATH}/tester-uzip")
set(STM32CUBE_DRIVERS  "${WORKSPACE_PATH}/STM32CubeF1")

# ===========================
# ОБЩИЕ НАСТРОЙКИ ДЛЯ ВСЕХ ЦЕЛЕЙ
# ===========================

add_compile_definitions(
    USE_FULL_LL_DRIVER
    USE_HAL_DRIVER
    STM32F1
    STM32F103xB
    # TESTER_MODES
    # TESTER_MULTIPOLE
    $<$<CONFIG:Debug>:DEBUG>
)

add_compile_options(
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
    -fdata-sections
    -ffunction-sections
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Debug>:-gdwarf-2>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-Os>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:ASM>:-xassembler-with-cpp>
)

# ------------------------
# Подключаем Cube
# ------------------------
add_subdirectory(${STM32CUBE_DRIVERS} stm32f1xx_drv)

# ------------------------
# Поиск скрипта линкера
# ------------------------
file(GLOB LD_SCRIPTS "${PROJ_ROOT}/*.ld")
list(LENGTH LD_SCRIPTS LD_COUNT)
if(LD_COUNT EQUAL 0)
    message(FATAL_ERROR "Не найден скрипт линкера (*.ld) в ${PROJ_ROOT}")
elseif(LD_COUNT GREATER 1)
    list(GET LD_SCRIPTS 0 LD_SCRIPTS)
    message(WARNING "Найдено несколько скриптов, используется первый: ${LD_SCRIPTS}")
endif()
set(LD_SCRIPT ${LD_SCRIPTS})

# ------------------------
# Исходники проекта
# ------------------------
file(GLOB_RECURSE PROJECT_C_SOURCES CONFIGURE_DEPENDS
    "${PROJ_ROOT}/code/buzzer/Src/*.c"
    "${PROJ_ROOT}/code/calc/Src/*.c"
    "${PROJ_ROOT}/code/commutator/Src/*.c"
    "${PROJ_ROOT}/code/configurator/Src/*.c"
    "${PROJ_ROOT}/code/core/Src/*.c"
    "${PROJ_ROOT}/code/ina239/Src/*.c"
    "${PROJ_ROOT}/code/keyboard/Src/*.c"
    "${PROJ_ROOT}/code/lcd/Src/*.c"
    "${PROJ_ROOT}/code/lcd/Src/screens/*.c"
    "${PROJ_ROOT}/code/proccessor/Src/*.c"
    "${PROJ_ROOT}/code/proccessor/Src/states/*.c"
    "${PROJ_ROOT}/code/utils/Src/*.c"
)
# lcd.c раньше собирался в составе цели glcd. От glcd отказались,
# теперь это обычный исходник проекта.
list(REMOVE_ITEM PROJECT_C_SOURCES 
    "${PROJ_ROOT}/code/core/Src/example_usage.c"
)
file(GLOB_RECURSE PROJECT_ASM_SOURCES "${PROJ_ROOT}/code/core/Src/*.s")

# === Конфигурация тестера ===
#
# tools/minify_json.py превращает config/new_config.json в C-файл с готовыми
# const-структурами. Прошивка ничего не разбирает в рантайме: params лежит
# во Flash в секции .config. Разбор JSON на месте требовал бы массива токенов
# jsmn (порядка 1.7 КБ ОЗУ уже на текущем конфиге и не помещался бы вовсе на
# конфиге в 8 КБ) - см. docs/markdown/ram_audit.md.

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CONFIG_SOURCE    ${CMAKE_SOURCE_DIR}/config/new_config.json)
set(CONFIG_GENERATOR ${CMAKE_SOURCE_DIR}/tools/minify_json.py)
set(CONFIG_GENERATED ${CMAKE_BINARY_DIR}/config/config_data.c)

if(NOT EXISTS ${CONFIG_GENERATOR})
    message(FATAL_ERROR "Не найден генератор конфигурации: ${CONFIG_GENERATOR}")
endif()
if(NOT EXISTS ${CONFIG_SOURCE})
    message(FATAL_ERROR "Не найден файл конфигурации: ${CONFIG_SOURCE}")
endif()

add_custom_command(
    OUTPUT ${CONFIG_GENERATED}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/config
    COMMAND ${Python3_EXECUTABLE} ${CONFIG_GENERATOR}
            -i ${CONFIG_SOURCE}
            -o ${CONFIG_GENERATED}
    DEPENDS ${CONFIG_SOURCE} ${CONFIG_GENERATOR}
    COMMENT "Генерация C-структур конфигурации из new_config.json"
    VERBATIM
)

set_source_files_properties(${CONFIG_GENERATED} PROPERTIES GENERATED TRUE)

# Исходники проекта плюс сгенерированная конфигурация
add_executable(${PROJECT_NAME}.elf ${PROJECT_C_SOURCES} ${PROJECT_ASM_SOURCES})
target_sources(${PROJECT_NAME}.elf PRIVATE ${CONFIG_GENERATED})

# ------------------------
# Подключаем конфиг, заголовки и библиотеки
# ------------------------
# glcd убран: отрисовка своя, построчная (code/lcd/Src/framebuffer.c),
# контроллер NT75451 - в code/lcd/Src/lcd.c.
# jsmn убран: JSON в прошивке больше не разбирается.
target_link_libraries(${PROJECT_NAME}.elf PRIVATE 
    stm32_drivers
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${PROJ_ROOT}/code/buzzer/Inc
    ${PROJ_ROOT}/code/calc/Inc
    ${PROJ_ROOT}/code/commutator/Inc
    ${PROJ_ROOT}/code/configurator/Inc
    ${PROJ_ROOT}/code/core/Inc
    ${PROJ_ROOT}/code/ina239/Inc
    ${PROJ_ROOT}/code/keyboard/Inc
    ${PROJ_ROOT}/code/lcd/Inc
    ${PROJ_ROOT}/code/lcd/Inc/screens
    ${PROJ_ROOT}/code/proccessor/Inc
    ${PROJ_ROOT}/code/proccessor/Inc/states
    ${PROJ_ROOT}/code/utils/Inc
)

target_link_options( ${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
    -specs=nano.specs
    -specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map,${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
    -T${LD_SCRIPT}
    # -Wl,--verbose
)

# ===========================
# Генерация bin и hex
# ===========================
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE}              ${PROJECT_NAME}.elf
    COMMENT "Making firmware: ${PROJECT_NAME}.bin, ${PROJECT_NAME}.hex"
)

if(EXISTS ${CMAKE_SOURCE_DIR}/compile_commands.json)
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Remove old compile_commands.json from project workspace"
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}
    COMMENT "Copy new compile_commands.json for clangd LSP"
)

# === Отладочная информация ===
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Toolchain: ${CMAKE_C_COMPILER}")