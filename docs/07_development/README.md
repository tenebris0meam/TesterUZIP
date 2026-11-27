
## Сборка

Требуется:

- ARM GNU Toolchain (`arm-none-eabi-gcc`), файл `arm-none-eabi.cmake` в корне;
- CMake ≥ 3.20, MinGW Make;
- Python 3 (генерация конфигурации);
- библиотека **STM32CubeF1** (пути заданы в начале `CMakeLists.txt`: `WORKSPACE_PATH`, `STM32CUBE_DRIVERS`).

```bash
cmake --preset debug      # или release
cmake --build build
```

Результат: `tester-uzip.elf`, `.bin`, `.hex`, `.map` в каталоге `build/`.

Флаги компиляции: `USE_FULL_LL_DRIVER`, `STM32F103xB`, `DEBUG` (только для Debug; в нём используется другая калибровка `SHUNT_CAL_RAW`). Release собирается с `-Os`.

Опциональные макросы (закомментированы в `CMakeLists.txt`):

| Макрос | Эффект |
|--------|--------|
| `TESTER_MODES` | режимы вольтметра, омметра, генератора и выбор режима |
| `TESTER_MULTIPOLE` | сценарий проверки многополюсного устройства (`multi_pole_test.c`) |

## Прошивка и отладка

Подключение через ST-Link и OpenOCD (`my_f103.cfg`).

```powershell
# прошивка (.elf, .hex или .bin; для .bin адрес 0x08000000 подставляется сам)
.\firmware_stm32f103c8t6.ps1 -FirmwareFile build\tester-uzip.elf

# полное стирание памяти МК
.\erase_stm32f103c8t6.ps1
```

В `PATH` должны быть `openocd` и `arm-none-eabi-gdb`. Скрипты поднимают OpenOCD в фоне, выполняют команды через GDB и завершают OpenOCD.