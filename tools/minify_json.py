"""Генератор C-структур конфигурации тестера.

Читает config/new_config.json и выпускает C-файл с готовыми const-структурами,
которые линкуются в отдельную секцию Flash (.config). Прошивке разбирать JSON
не нужно вообще: params - это const-структура во Flash, а не построенное в
рантайме дерево.

Зачем: разбор JSON на месте требовал массива токенов jsmn (1696 байт ОЗУ уже
на текущем конфиге в 395 байт и порядка 35 КБ на конфиге в 8 КБ, то есть не
помещался в 20 КБ вовсе) плюс дерево через malloc. Здесь стоимость по ОЗУ - 0.

Формат входного файла (config/new_config.json) - массив, где:
  * элемент 0     - каталог: SeriesAmount/GroupsAmount/DevicesAmount/SequencesAmount/
                    TestParamsAmount (счётчики, необязательны, сверяются с фактом),
                    DevicesNames (имена приборов),
                    Sequences (список тестовых последовательностей {InputPin, OutputPin}),
                    TestParams (список параметров теста {Voltage_V, Current_mA}),
                    Series (для каждой серии - имя и диапазон групп {SeriaName, GroupBeginID, GroupCount}),
                    Groups (для каждой группы - имя и диапазон приборов {GroupName, DeviceBeginID, DeviceCount}).
                    Приборы в элементах 1..N обязаны идти отсортированными по
                    (SeriaID, GroupID), т.к. диапазоны Series/Groups ссылаются на
                    непрерывные позиции в списках групп/приборов;
  * элементы 1..N - приборы: SeriaID/GroupID/DeviceID (индексы серии, группы, имени),
                    TestSequences (список тестовых шагов {ID, TestID}, где ID - индекс
                    в Sequences, TestID - индекс в TestParams), MinimalVoltage/MaximalVoltage
                    (диапазон напряжения), DischargerVoltage (напряжение разрядника),
                    ConnectorMask (маска разъёмов).

Использование:
    python tools/minify_json.py -i config/new_config.json -o build/config_data.c
"""

import argparse
import json
import os
import sys

# --- Границы, продублированные из прошивки -------------------------------- #

MAX_NAME_SIZE = 31          # params.h: MAX_NAME_SIZE
MAX_VOLTAGE = 550           # config_testerUZIP_schema.json
MAX_CURRENT_MA = 2          # code/Inc/hv_feedback.h: HV_SHUNT_MAX_CURRENT_nA = 2 000 000 нА = 2 мА
MAX_PIN = 8                 # ActiveLine_1..ActiveLine_9 в commutator.h
MAX_CONNECTOR_MASK = 0x07   # Probe | RJ45 | K1
CONFIG_REGION_SIZE = 8 * 1024   # секция CONFIG в STM32F103XB_FLASH.ld
MAX_SEQUENCES_PER_DEVICE = 32   # model.h: разрядность маски результатов

# Значения CONFIG_MAGIC и CONFIG_VERSION берутся из макросов params.h прямо
# в сгенерированном файле - дублировать их здесь незачем, разъехались бы.

# --- Имена ключей для разбора JSON ---------------------------------------- #
CATALOG_SERIES_AMOUNT = "SeriesAmount"
CATALOG_GROUPS_AMOUNT = "GroupsAmount"
CATALOG_DEVICES_AMOUNT = "DevicesAmount"
CATALOG_SEQUENCES_AMOUNT = "SequencesAmount"
CATALOG_DEVICES_NAMES = "DevicesNames"
CATALOG_SEQUENCES = "Sequences"
CATALOG_SEQUENCES_INPUT_PIN = "InputPin"
CATALOG_SEQUENCES_OUTPUT_PIN = "OutputPin"

CATALOG_TEST_PARAMS = "TestParams"
CATALOG_TEST_PARAMS_MINVOLTAGE = "MinVoltage"
CATALOG_TEST_PARAMS_MAXVOLTAGE = "MaxVoltage"
CATALOG_TEST_PARAMS_CURRENT = "Current"
CATALOG_TEST_PARAMS_AMOUNT = "TestParamsAmount"

CATALOG_SERIES = "Series"
CATALOG_SERIA_NAME = "SeriaName"
CATALOG_SERIES_GROUP_BEGIN = "GroupBeginID"
CATALOG_SERIES_GROUP_COUNT = "GroupCount"

CATALOG_GROUPS = "Groups"
CATALOG_GROUP_NAME = "GroupName"
CATALOG_GROUPS_DEVICE_BEGIN = "DeviceBeginID"
CATALOG_GROUPS_DEVICE_COUNT = "DeviceCount"

DEVICES_SERIA_ID = "SeriaID"
DEVICES_GROUP_ID = "GroupID"
DEVICES_DEVICE_ID = "DeviceID"
DEVICES_TEST_SEQUENCES = "TestSequences"
DEVICES_TEST_SEQUENCES_SEQ_ID = "SequenceID"
DEVICES_TEST_SEQUENCES_TEST_ID = "TestID"
DEVICES_TEST_SEQUENCES_AMOUNT = "TestSequencesAmount"
DEVICES_CONNECTOR_MASK = "ConnectorMask"

class ConfigError(Exception):
    """Ошибка, из-за которой сборку нужно остановить."""


_warnings = []


def warn(message: str):
    _warnings.append(message)
    print(f"config: ПРЕДУПРЕЖДЕНИЕ: {message}", file=sys.stderr)


def require(condition: bool, message: str):
    if not condition:
        raise ConfigError(message)


# --- Разбор и проверка ----------------------------------------------------- #

def _get_int(obj, key, where, lo, hi):
    require(key in obj, f"{where}: отсутствует поле '{key}'")
    val = obj[key]
    require(isinstance(val, int) and not isinstance(val, bool),
            f"{where}: поле '{key}' должно быть целым, получено {val!r}")
    require(lo <= val <= hi,
            f"{where}: поле '{key}' = {val}, допустимо {lo}..{hi}")
    return val

def _get_str(obj, key, where):
    require(key in obj, f"{where}: отсутствует поле '{key}'")
    val = obj[key]
    require(isinstance(val, str), 
            f"{where}: '{key}' должно быть строкой, получено {val!r}")
    raw = val.encode("utf-8")
    require(len(raw) < MAX_NAME_SIZE, 
                f"{where}: '{key}' = {val!r} длиной {len(raw)} байт, "
                f"максимум {MAX_NAME_SIZE}")
    for byte in raw:
            if not (0x20 <= byte <= 0x7F):
                warn(f"{where}: '{key}' = {val!r} содержит байт 0x{byte:02X} "
                     f"вне диапазона шрифта 0x20..0x7F - на экране будет точка. "
                     f"Кириллицу задавать по docs/markdown/character_table.md")
    return val

def _get_names(catalog, key, where):
    require(key in catalog, f"{where}: отсутствует список имён '{key}'")
    names = catalog[key]
    require(isinstance(names, list), f"{where}: '{key}' должен быть массивом")
    for idx, name in enumerate(names):
        require(isinstance(name, str),
                f"{where}: {key}[{idx}] должно быть строкой, получено {name!r}")
        raw = name.encode("utf-8")
        require(len(raw) <= MAX_NAME_SIZE,
                f"{where}: {key}[{idx}] = {name!r} длиной {len(raw)} байт, "
                f"максимум {MAX_NAME_SIZE}")
        for byte in raw:
            if not (0x20 <= byte <= 0x7F):
                warn(f"{where}: {key}[{idx}] = {name!r} содержит байт 0x{byte:02X} "
                     f"вне диапазона шрифта 0x20..0x7F - на экране будет точка. "
                     f"Кириллицу задавать по docs/markdown/character_table.md")
    return names


def _check_declared_count(catalog, key, actual, what):
    if key in catalog and catalog[key] != actual:
        warn(f"каталог: {key} = {catalog[key]}, а в конфигурации {actual} {what}. "
             f"Использую фактическое значение {actual}")


def parse_config(raw):
    require(isinstance(raw, list) and raw,
            "конфигурация должна быть непустым массивом")

    catalog = raw[0]
    require(isinstance(catalog, dict), "элемент 0 (каталог) должен быть объектом")

    devices_names = _get_names(catalog, CATALOG_DEVICES_NAMES, "каталог")

    require(CATALOG_SEQUENCES in catalog, "каталог: отсутствует список последовательностей '" + CATALOG_SEQUENCES + "'")
    require(isinstance(catalog[CATALOG_SEQUENCES], list), "каталог: '" + CATALOG_SEQUENCES + "' должен быть массивом")
    sequences = []
    for idx, seq in enumerate(catalog[CATALOG_SEQUENCES]):
        where = f"каталог: '" + CATALOG_SEQUENCES + f"'[{idx}]"
        require(isinstance(seq, dict), f"{where}: должен быть объектом")
        sequences.append((
            _get_int(seq, CATALOG_SEQUENCES_INPUT_PIN, where, 0, MAX_PIN),
            _get_int(seq, CATALOG_SEQUENCES_OUTPUT_PIN, where, 0, MAX_PIN),
        ))

    require(CATALOG_TEST_PARAMS in catalog, "каталог: отсутствует список параметров теста '" + CATALOG_TEST_PARAMS + "'")
    require(isinstance(catalog[CATALOG_TEST_PARAMS], list), "каталог: '" + CATALOG_TEST_PARAMS + "' должен быть массивом")
    test_params = []
    for idx, tp in enumerate(catalog[CATALOG_TEST_PARAMS]):
        where = f"каталог: '" + CATALOG_TEST_PARAMS + f"'[{idx}]"
        require(isinstance(tp, dict), f"{where}: должен быть объектом")
        test_params.append((
            _get_int(tp, CATALOG_TEST_PARAMS_MINVOLTAGE, where, 0, MAX_VOLTAGE),
            _get_int(tp, CATALOG_TEST_PARAMS_MAXVOLTAGE, where, 0, MAX_VOLTAGE),
            _get_int(tp, CATALOG_TEST_PARAMS_CURRENT, where, 0, MAX_CURRENT_MA)
        ))
    require(test_params, "каталог: '" + CATALOG_TEST_PARAMS + "' пуст - " +
            "каждой тестовой последовательности прибора нужен параметр теста")

    def _parse_ranges(list_key, name_key, begin_key, count_key, owner_count_key, owner_name):
        require(list_key in catalog, f"каталог: отсутствует список диапазонов '{list_key}'")
        require(isinstance(catalog[list_key], list), f"каталог: '{list_key}' должен быть массивом")
        ranges = []
        names = []
        for idx, item in enumerate(catalog[list_key]):
            where = f"каталог: '{list_key}'[{idx}]"
            require(isinstance(item, dict), f"{where}: должен быть объектом")
            name = _get_str(item, name_key, where)
            begin = _get_int(item, begin_key, where, 0, 254)
            count = _get_int(item, count_key, where, 0, 255)
            ranges.append((begin, count))
            names.append(name)
        return ranges, names

    series_ranges, series_names = _parse_ranges(CATALOG_SERIES, 
                                                CATALOG_SERIA_NAME, 
                                                CATALOG_SERIES_GROUP_BEGIN,
                                                CATALOG_SERIES_GROUP_COUNT, 
                                                CATALOG_GROUPS_AMOUNT, 
                                                "групп")
    group_ranges, groups_names = _parse_ranges(CATALOG_GROUPS, 
                                               CATALOG_GROUP_NAME, 
                                               CATALOG_GROUPS_DEVICE_BEGIN,
                                               CATALOG_GROUPS_DEVICE_COUNT, 
                                               CATALOG_DEVICES_AMOUNT, 
                                               "приборов")

    require(len(series_ranges) == len(series_names),
            f"каталог: '" + CATALOG_SERIES + f"' содержит {len(series_ranges)} диапазонов, "
            f"а имён серий {len(series_names)} - должно быть по одному диапазону на серию")
    require(len(group_ranges) == len(groups_names),
            f"каталог: '" + CATALOG_GROUPS + f"' содержит {len(group_ranges)} диапазонов, "
            f"а имён групп {len(groups_names)} - должно быть по одному диапазону на группу")

    _check_declared_count(catalog, CATALOG_SERIES_AMOUNT, len(series_names), "имён серий")
    _check_declared_count(catalog, CATALOG_GROUPS_AMOUNT, len(groups_names), "имён групп")
    _check_declared_count(catalog, CATALOG_DEVICES_AMOUNT, len(devices_names), "имён приборов")
    _check_declared_count(catalog, CATALOG_SEQUENCES_AMOUNT, len(sequences), "последовательностей")
    _check_declared_count(catalog, CATALOG_TEST_PARAMS_AMOUNT, len(test_params), "параметров теста")

    for name, count in (("серий", len(series_names)),
                        ("групп", len(groups_names)),
                        ("приборов", len(devices_names)),
                        ("последовательностей", len(sequences)),
                        ("параметров теста", len(test_params))):
        require(count <= 255, f"{name} {count}, а счётчики в params.h - uint8_t "
                              f"(максимум 255)")

    devices = []
    for pos, dev in enumerate(raw[1:], start=1):
        where = f"прибор (элемент {pos})"
        require(isinstance(dev, dict), f"{where}: должен быть объектом")

        seria_idx = _get_int(dev, DEVICES_SERIA_ID, where, 0, max(len(series_names) - 1, 0))
        group_idx = _get_int(dev, DEVICES_GROUP_ID, where, 0, max(len(groups_names) - 1, 0))
        device_idx = _get_int(dev, DEVICES_DEVICE_ID, where, 0, max(len(devices_names) - 1, 0))
        require(len(series_names) > 0, f"{where}: список серий пуст")
        require(len(groups_names) > 0, f"{where}: список групп пуст")
        require(len(devices_names) > 0, f"{where}: список имён приборов пуст")

        require(DEVICES_TEST_SEQUENCES in dev, f"{where}: отсутствует список '" + DEVICES_TEST_SEQUENCES + "'")
        require(isinstance(dev[DEVICES_TEST_SEQUENCES], list), f"{where}: '" + DEVICES_TEST_SEQUENCES + "' должен быть массивом")
        test_seqs = []
        for k, ts in enumerate(dev[DEVICES_TEST_SEQUENCES]):
            sub_where = f"{where}: " + DEVICES_TEST_SEQUENCES + f"[{k}]"
            require(isinstance(ts, dict), f"{sub_where}: должен быть объектом")
            sidx = _get_int(ts, DEVICES_TEST_SEQUENCES_SEQ_ID, sub_where, 0, max(len(sequences) - 1, 0))
            require(0 <= sidx < len(sequences),
                    f"{sub_where}: " + DEVICES_TEST_SEQUENCES_SEQ_ID + f" = {sidx}, а последовательностей "
                    f"{len(sequences)} (допустимо 0..{len(sequences) - 1})")
            tidx = _get_int(ts, DEVICES_TEST_SEQUENCES_TEST_ID, sub_where, 0, max(len(test_params) - 1, 0))
            require(0 <= tidx < len(test_params),
                    f"{sub_where}: " + DEVICES_TEST_SEQUENCES_TEST_ID + f" = {tidx}, а параметров теста "
                    f"{len(test_params)} (допустимо 0..{len(test_params) - 1})")
            test_seqs.append((sidx, tidx))

        require(len(test_seqs) <= MAX_SEQUENCES_PER_DEVICE,
                f"{where}: {len(test_seqs)} последовательностей, а маска "
                f"результатов в model.h рассчитана на {MAX_SEQUENCES_PER_DEVICE}")

        if DEVICES_TEST_SEQUENCES_AMOUNT in dev and dev[DEVICES_TEST_SEQUENCES_AMOUNT] != len(test_seqs):
            warn(f"{where}: " + DEVICES_TEST_SEQUENCES_AMOUNT +
                 f" = {dev[DEVICES_TEST_SEQUENCES_AMOUNT]}, а в " + DEVICES_TEST_SEQUENCES +
                 f" {len(test_seqs)} элементов. " +
                 f"Использую фактическое значение {len(test_seqs)}")

        connector = _get_int(dev, DEVICES_CONNECTOR_MASK, where, 1, MAX_CONNECTOR_MASK)

        devices.append({
            "seria_idx": seria_idx,
            "group_idx": group_idx,
            "device_idx": device_idx,
            "test_seqs": test_seqs,
            "connector": connector,
        })

    require(len(devices) <= 255,
            f"приборов {len(devices)}, а счётчик в params.h - uint8_t")
    require(devices, "в конфигурации нет ни одного прибора")

    # В params.h devices_count - ОДИН счётчик и для массива имён, и для массива
    # приборов: params_get_device_name() индексирует им cfg_devices_names,
    # params_get_device() - cfg_devices. Если длины разойдутся, экран выбора
    # прибора прочитает за границей cfg_devices. Плюс экран берёт номер строки
    # списка как индекс прибора, поэтому DI обязан совпадать с позицией.
    require(len(devices) == len(devices_names),
            f"приборов {len(devices)}, а имён в " + CATALOG_DEVICES_NAMES + f" {len(devices_names)}. "
            f"В params.h devices_count один на оба массива, длины обязаны "
            f"совпадать")
    for pos, dev in enumerate(devices):
        require(dev["device_idx"] == pos,
                f"прибор на позиции {pos} имеет " + CATALOG_DEVICES_NAMES + f" = {dev['device_idx']}. "
                f"Экран выбора индексирует и список имён, и список приборов "
                f"одним и тем же номером строки, поэтому DI обязан совпадать "
                f"с позицией прибора в файле")

    seen = set()
    for dev in devices:
        key = (dev["seria_idx"], dev["group_idx"], dev["device_idx"])
        if key in seen:
            warn(f"прибор с ключом " + DEVICES_SERIA_ID + 
                 f"={key[0]} " + DEVICES_GROUP_ID + 
                 f"={key[1]} " + DEVICES_DEVICE_ID + f"={key[2]} встречается " +
                 f"повторно - экран выбора покажет только первый")
        seen.add(key)

    # Иерархия Series -> Groups -> Devices опирается на то, что приборы во
    # входном JSON идут отсортированными по (seria_idx, group_idx): тогда
    # каждой серии/группе соответствует непрерывный диапазон позиций в
    # плоских массивах cfg_groups_names / cfg_devices. Экран меню использует
    # диапазоны из Series/Groups напрямую, поэтому расхождение с фактическим
    # порядком приборов молча сломает навигацию - проверяем строго.

    require(len(groups_names) > 0, "каталог: список групп пуст")
    require(len(devices_names) > 0, "каталог: список имён приборов пуст")

    prev_seria, prev_group = -1, -1
    for pos, dev in enumerate(devices):
        pair = (dev["seria_idx"], dev["group_idx"])
        prev_pair = (prev_seria, prev_group)
        require(pair >= prev_pair,
                f"прибор (элемент {pos + 1}): " + DEVICES_SERIA_ID + f"={dev['seria_idx']} " +
                DEVICES_GROUP_ID + f"={dev['group_idx']} нарушает сортировку - "
                f"приборы должны идти по возрастанию (SeriaID, GroupID), "
                f"иерархия Series/Groups опирается на непрерывные диапазоны позиций")
        prev_seria, prev_group = pair

    def _check_ranges(ranges, owner_names, owner_label, child_count, child_label):
        covered = [False] * child_count
        for idx, (begin, count) in enumerate(ranges):
            where = f"каталог: '" + (CATALOG_SERIES if owner_label == "серии" else CATALOG_GROUPS) + f"'[{idx}]"
            end = begin + count
            require(end <= child_count,
                    f"{where}: диапазон [{begin}, {end}) выходит за пределы "
                    f"{child_label} ({child_count} шт.)")
            for i in range(begin, end):
                require(not covered[i],
                        f"{where}: позиция {i} среди {child_label} уже покрыта "
                        f"другим диапазоном {owner_label} - диапазоны не должны пересекаться")
                covered[i] = True
        uncovered = [i for i, c in enumerate(covered) if not c]
        require(not uncovered,
                f"каталог: {child_label} на позициях {uncovered} не входят ни в один "
                f"диапазон {owner_label} - в '" +
                (CATALOG_SERIES if owner_label == "серии" else CATALOG_GROUPS) +
                "' пропущены записи")

    _check_ranges(series_ranges, series_names, "серии", len(groups_names), "группы")
    _check_ranges(group_ranges, groups_names, "группы", len(devices_names), "приборы")

    # Диапазоны должны не просто покрывать все группы/приборы без дыр и
    # пересечений (проверено выше), но и совпадать с тем, что реально следует
    # из seria_idx/group_idx приборов - иначе экран меню, идущий по
    # cfg_series_groups/cfg_group_devices, покажет не тех приборов не в той
    # серии.
    actual_group_owner = {}   # group_idx -> seria_idx, по факту встреченный у приборов
    actual_device_owner = {}  # device_idx (= позиция) -> group_idx
    for dev in devices:
        actual_device_owner[dev["device_idx"]] = dev["group_idx"]
        if dev["group_idx"] not in actual_group_owner:
            actual_group_owner[dev["group_idx"]] = dev["seria_idx"]
        else:
            require(actual_group_owner[dev["group_idx"]] == dev["seria_idx"],
                    f"прибор: GroupID={dev['group_idx']} встречается под разными "
                    f"SeriaID ({actual_group_owner[dev['group_idx']]} и {dev['seria_idx']}) - "
                    f"группа не может принадлежать двум сериям")

    for seria_idx, (begin, count) in enumerate(series_ranges):
        for group_idx in range(begin, begin + count):
            require(actual_group_owner.get(group_idx) == seria_idx,
                    f"каталог: '" + CATALOG_SERIES + f"'[{seria_idx}] относит группу "
                    f"{group_idx} к серии {seria_idx}, но по SeriaID/GroupID приборов "
                    f"эта группа принадлежит серии {actual_group_owner.get(group_idx)!r}")

    for group_idx, (begin, count) in enumerate(group_ranges):
        for device_idx in range(begin, begin + count):
            require(actual_device_owner.get(device_idx) == group_idx,
                    f"каталог: '" + CATALOG_GROUPS + f"'[{group_idx}] относит прибор "
                    f"{device_idx} к группе {group_idx}, но по GroupID прибора "
                    f"он принадлежит группе {actual_device_owner.get(device_idx)!r}")

    return {
        "series_names": series_names,
        "groups_names": groups_names,
        "devices_names": devices_names,
        "sequences": sequences,
        "test_params": test_params,
        "series_ranges": series_ranges,
        "group_ranges": group_ranges,
        "devices": devices,
    }


# --- Генерация C ----------------------------------------------------------- #

def _c_char(byte):
    """Байт как символьная константа C, безопасная в любом окружении."""
    if byte == ord("\\"):
        return "'\\\\'"
    if byte == ord("'"):
        return "'\\''"
    if 0x20 <= byte <= 0x7E:
        return f"'{chr(byte)}'"
    return f"0x{byte:02X}"


def build_string_pool(cfg):
    """Общий пул байтов имён. Одинаковые имена переиспользуются."""
    pool = bytearray()
    offsets = {}

    def intern(name):
        raw = name.encode("utf-8")
        key = bytes(raw)
        if key not in offsets:
            offsets[key] = len(pool)
            pool.extend(raw)
        return offsets[key], len(raw)

    spans = {}
    for kind in ("series_names", "groups_names", "devices_names"):
        spans[kind] = [intern(name) for name in cfg[kind]]
    return bytes(pool), spans


def generate_c(cfg, source_name):
    pool, spans = build_string_pool(cfg)

    test_seq_pool = []   # плоский пул пар (seq_idx, test_param_idx) для всех приборов
    device_seq_offsets = []
    for dev in cfg["devices"]:
        device_seq_offsets.append(len(test_seq_pool))
        test_seq_pool.extend(dev["test_seqs"])

    out = []
    add = out.append

    add("/*")
    add(" * СГЕНЕРИРОВАНО АВТОМАТИЧЕСКИ - НЕ РЕДАКТИРОВАТЬ.")
    add(f" * Источник:  {source_name}")
    add(" * Генератор: tools/minify_json.py")
    add(" *")
    add(" * Правки вносить в исходный JSON и пересобирать проект.")
    add(" * Все объекты этого файла лежат во Flash в секции .config")
    add(" * (регион CONFIG в STM32F103XB_FLASH.ld). ОЗУ не расходуется.")
    add(" */")
    add("")
    add('#include "params.h"')
    add("")
    add("#define CFG_HEADER __attribute__((section(\".config.header\"), used))")
    add("#define CFG_DATA   __attribute__((section(\".config.data\"), used))")
    add("")

    # --- пул имён ---
    add(f"/** @brief Пул байтов имён, {len(pool)} байт. Строки не завершены нулём. */")
    add("static const char cfg_names[] CFG_DATA = {")
    for i in range(0, len(pool), 12):
        chunk = ", ".join(_c_char(b) for b in pool[i:i + 12])
        add(f"    {chunk},")
    add("};")
    add("")

    def emit_names(c_name, kind, comment):
        entries = spans[kind]
        add(f"/** @brief {comment} */")
        add(f"static const MyString {c_name}[] CFG_DATA = {{")
        for (off, length), text in zip(entries, cfg[kind]):
            add(f"    {{ cfg_names + {off:3d}, cfg_names + {off + length:3d} }}, "
                f"/* {text!r} */".replace("'", '"'))
        add("};")
        add("")

    emit_names("cfg_series_names", "series_names", "Имена серий.")
    emit_names("cfg_groups_names", "groups_names", "Имена групп.")
    emit_names("cfg_devices_names", "devices_names", "Имена приборов.")

    # --- последовательности ---
    add("/** @brief Тестовые последовательности: пары коммутируемых линий. */")
    add("static const Params_TestSequence_TypeDef cfg_sequences[] CFG_DATA = {")
    for idx, (in_pin, out_pin) in enumerate(cfg["sequences"]):
        add(f"    {{ .input_pin = {in_pin}, .output_pin = {out_pin} }}, /* [{idx}] */")
    add("};")
    add("")

    # --- параметры теста (напряжение/ток) ---
    add("/** @brief Параметры теста: напряжение и ток, на которые ссылаются приборы. */")
    add("static const Params_TestParam_TypeDef cfg_test_params[] CFG_DATA = {")
    for idx, (voltage_min, voltage_max, current) in enumerate(cfg["test_params"]):
        add(f"    {{ .min_voltage = {voltage_min}, .max_voltage = {voltage_max}, .current = {current} }}, /* [{idx}] */")
    add("};")
    add("")

    # --- тестовые шаги приборов (последовательность + параметр теста) ---
    add("/** @brief Общий пул тестовых шагов (seq_idx, test_param_idx) для всех приборов. */")
    add("static const Params_TestSeq_TypeDef cfg_test_sequences[] CFG_DATA = {")
    if test_seq_pool:
        for seq_idx, test_idx in test_seq_pool:
            add(f"    {{ .seq_idx = {seq_idx}, .test_param_idx = {test_idx} }},")
    else:
        add("    { .seq_idx = 0, .test_param_idx = 0 }, /* ни одному прибору не назначено последовательностей */")
    add("};")
    add("")

    # --- диапазоны: серия -> группы, группа -> приборы ---
    add("/** @brief Для каждой серии - диапазон принадлежащих ей групп в cfg_groups_names/cfg_group_devices. */")
    add("static const Params_Range_TypeDef cfg_series_groups[] CFG_DATA = {")
    for idx, (begin, count) in enumerate(cfg["series_ranges"]):
        name = cfg["series_names"][idx]
        add(f"    {{ .begin_idx = {begin}, .count = {count} }}, /* {name!r} */".replace("'", '"'))
    add("};")
    add("")

    add("/** @brief Для каждой группы - диапазон принадлежащих ей приборов в cfg_devices_names/cfg_devices. */")
    add("static const Params_Range_TypeDef cfg_group_devices[] CFG_DATA = {")
    for idx, (begin, count) in enumerate(cfg["group_ranges"]):
        name = cfg["groups_names"][idx]
        add(f"    {{ .begin_idx = {begin}, .count = {count} }}, /* {name!r} */".replace("'", '"'))
    add("};")
    add("")

    # --- приборы ---
    add("/** @brief Приборы. */")
    add("static const Params_Device_TypeDef cfg_devices[] CFG_DATA = {")
    for dev, off in zip(cfg["devices"], device_seq_offsets):
        name = cfg["devices_names"][dev["device_idx"]]
        add(f"    {{ /* {name} */")
        add(f"        .seria_idx = {dev['seria_idx']},")
        add(f"        .group_idx = {dev['group_idx']},")
        add(f"        .device_idx = {dev['device_idx']},")
        add(f"        .test_sequences = cfg_test_sequences + {off},")
        add(f"        .num_seqs = {len(dev['test_seqs'])},")
        add(f"        .connector_types = {dev['connector']}")
        add("    },")
    add("};")
    add("")

    # --- корень ---
    add("/** @brief Корень конфигурации. Лежит в начале региона CONFIG. */")
    add("const Params_TypeDef params CFG_HEADER = {")
    add("    .magic = CONFIG_MAGIC,")
    add("    .version = CONFIG_VERSION,")
    add(f"    .series_count = {len(cfg['series_names'])},")
    add(f"    .groups_count = {len(cfg['groups_names'])},")
    add(f"    .devices_count = {len(cfg['devices_names'])},")
    add(f"    .sequences_count = {len(cfg['sequences'])},")
    add(f"    .test_params_count = {len(cfg['test_params'])},")
    add("    .series_names = cfg_series_names,")
    add("    .groups_names = cfg_groups_names,")
    add("    .devices_names = cfg_devices_names,")
    add("    .sequences = cfg_sequences,")
    add("    .test_params = cfg_test_params,")
    add("    .series_groups = cfg_series_groups,")
    add("    .group_devices = cfg_group_devices,")
    add("    .devices = cfg_devices")
    add("};")
    add("")

    estimate = estimate_size(cfg, pool, test_seq_pool)
    require(estimate <= CONFIG_REGION_SIZE,
            f"конфигурация занимает примерно {estimate} байт, а регион CONFIG "
            f"в STM32F103XB_FLASH.ld рассчитан на {CONFIG_REGION_SIZE}")

    return "\n".join(out) + "\n", estimate


def estimate_size(cfg, pool, test_seq_pool):
    """Оценка сверху для занимаемого места во Flash (для ранней диагностики)."""
    n_names = (len(cfg["series_names"]) + len(cfg["groups_names"]) +
               len(cfg["devices_names"]))
    return (
        len(pool)
        + n_names * 8                          # MyString: два указателя
        + len(cfg["sequences"]) * 2            # Params_TestSequence_TypeDef
        + len(cfg["test_params"]) * 3          # Params_TestParam_TypeDef (voltage_v:2 + current_ma:1)
        + len(test_seq_pool) * 2               # Params_TestSeq_TypeDef: seq_idx + test_param_idx
        + len(cfg["series_ranges"]) * 2        # Params_Range_TypeDef
        + len(cfg["group_ranges"]) * 2         # Params_Range_TypeDef
        + len(cfg["devices"]) * 20             # Params_Device_TypeDef с выравниванием
        + 40                                   # Params_TypeDef (расширился новыми полями)
        + 64                                   # запас на выравнивание секций
    )


# --- Точка входа ----------------------------------------------------------- #

def main():
    parser = argparse.ArgumentParser(
        description="Генерация C-структур конфигурации из JSON")
    parser.add_argument("-i", "--input", required=True,
                        help="Входной JSON (config/new_config.json)")
    parser.add_argument("-o", "--output", required=True,
                        help="Выходной C-файл (build/config/config_data.c)")
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"config: ОШИБКА: файл {args.input} не найден", file=sys.stderr)
        return 1

    try:
        with open(args.input, "r", encoding="utf-8-sig") as handle:
            raw = json.load(handle)
    except json.JSONDecodeError as exc:
        print(f"config: ОШИБКА: некорректный JSON в {args.input}: {exc}",
              file=sys.stderr)
        return 1

    try:
        cfg = parse_config(raw)
        text, estimate = generate_c(cfg, os.path.basename(args.input))
    except ConfigError as exc:
        print(f"config: ОШИБКА: {exc}", file=sys.stderr)
        return 1

    out_dir = os.path.dirname(os.path.abspath(args.output))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    # Перезаписываем только при изменении, чтобы не дёргать пересборку впустую.
    new_bytes = text.encode("utf-8")
    if os.path.exists(args.output):
        with open(args.output, "rb") as handle:
            if handle.read() == new_bytes:
                print(f"config: {args.output} уже актуален")
                return 0
    with open(args.output, "wb") as handle:
        handle.write(new_bytes)

    print(f"config: {args.input} -> {args.output}")
    print(f"config: серий {len(cfg['series_names'])}, "
          f"групп {len(cfg['groups_names'])}, "
          f"приборов {len(cfg['devices'])}, "
          f"последовательностей {len(cfg['sequences'])}, "
          f"параметров теста {len(cfg['test_params'])}")
    print(f"config: во Flash примерно {estimate} байт из {CONFIG_REGION_SIZE}.")
    if _warnings:
        print(f"config: предупреждений: {len(_warnings)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
