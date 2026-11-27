
## Конфигурация приборов

Каталог приборов хранится в `config/new_config.json` и **не разбирается в рантайме**. При сборке `tools/minify_json.py` генерирует `build/config/config_data.c` с готовыми `const`-структурами, которые линкуются в секцию `.config` (регион `CONFIG`, 8 КБ Flash). Расход ОЗУ — ноль. Прошивка лишь проверяет сигнатуру и версию (`configurator_read_config()`).

Формат файла — массив:

- **элемент 0 — каталог**: `Series`, `Groups`, `DevicesNames`, `Sequences` (пары `InputPin`/`OutputPin`, 0…8), `TestParams` (3 типа: TVS, варистор, GDT; в каждом — `MinVoltage`/`MaxVoltage`);
- **элементы 1…N — приборы**: `SeriaID`, `GroupID`, `DeviceID`, `TestSequences` (три «ведра» по типам с парами `SequenceID`/`TestID`), `ConnectorID` (0 — щупы, 1 — RJ45, 2 — K1).

Генератор строго валидирует конфигурацию: диапазоны, сортировку приборов по (серия, группа), совпадение индексов с позициями, лимит 32 проверки на прибор, размер блока. Имена пишутся в закодированной кириллице (см. выше). Текущий каталог: 2 серии, 13 групп, 41 прибор, 72 последовательности.

Запуск вручную:

```bash
python tools/minify_json.py -i config/new_config.json -o build/config/config_data.c
```