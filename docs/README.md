
## Структура проекта

Исходники разложены по модулям `code/<модуль>/{Inc,Src}`:

| Модуль | Файлы | Назначение |
|--------|-------|------------|
| `core` | `main.c`, `gpio.c`, `spi.c`, `dma.c`, `tim.c`, `delay.c`, `flash.c`, `stm32f1xx_it.c`, `critical_section.c` | инициализация, периферия, прерывания |
| `proccessor` | `proccessor.c`, `resolver.c`, `model.c`, `states/*.c` | конечный автомат проверки и модель данных |
| `calc` | `calc.c`, `hv_feedback.h` | регулятор напряжения, классификация измерений |
| `ina239` | `ina239.c`, `ina239_user.c` | драйвер INA239 и обмен кадрами по DMA |
| `commutator` | `commutator.c`, `mux.c`, `relay.c` | коммутация линий |
| `lcd` | `lcd.c`, `framebuffer.c`, `display.c`, `screens.c`, `cursor.c`, `fonts.c`, `screens/*.c` | драйвер NT75451, построчная отрисовка, экраны |
| `keyboard` | `keyboard.c`, `input_events.c` | опрос кнопок, события ввода |
| `buzzer` | `buzzer.c` | звуковой сигнал |
| `configurator` | `configurator.c`, `params.h` | проверка блока конфигурации во Flash |
| `utils` | `utils.c`, `strbuf.c`, `window.c` | вспомогательные функции |

Прочее:

- `config/new_config.json` — каталог приборов и параметров тестов;
- `tools/minify_json.py` — генератор C-структур конфигурации;
- `*.ld` — скрипт линковщика (регион `CONFIG` под конфигурацию);
- `firmware_stm32f103c8t6.ps1`, `erase_stm32f103c8t6.ps1`, `my_f103.cfg` — прошивка/стирание через OpenOCD + ST-Link;
- `CMakeLists.txt`, `CMakePresets.json` — сборка.

## Содержание

1. [Краткий обзор](./01_overview/README.md)
2. [Архитектура](./02_architecture/README.md)
3. [Модули](./03_modules/README.md)
4. [Алгоритмы](./04_algorithms/README.md)
5. [Аппаратное обеспечение](./05_hardware/README.md)
6. [Конфигурация](./06_configuration/README.md)
7. [Разработка](./07_development/README.md)
8. [Справка](./08_reference/README.md)