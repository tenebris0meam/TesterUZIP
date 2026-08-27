"""Генератор C-структур конфигурации тестера.

Читает config/new_config.json и выпускает C-файл с готовыми const-структурами,
которые линкуются в отдельную секцию Flash (.config). Прошивке разбирать JSON
не нужно вообще: params - это const-структура во Flash, а не построенное в
рантайме дерево.

Зачем: разбор JSON на месте требовал массива токенов jsmn (1696 байт ОЗУ уже
на текущем конфиге в 395 байт и порядка 35 КБ на конфиге в 8 КБ, то есть не
помещался в 20 КБ вовсе) плюс дерево через malloc. Здесь стоимость по ОЗУ - 0.
Подробности: docs/markdown/ram_audit.md, раздел "Разбор конфигурации".

Формат входного файла (config/new_config.json) - массив, где:
  * элемент 0     - каталог: SeriesAmount/GroupsAmount/DevicesAmount/SeqencesAmount (счётчики), 
                    SeriesNames/GroupsNames/DevicesNames (имена серий,
                    групп, приборов), Sequences (список тестовых последовательностей
                    {InputPin, OutputPin});
  * элементы 1..N - приборы: SeriaID/GroupID/DeviceID (индексы серии, группы, имени),
                    SequenceIDs (индексы в Sequence), MinimalVoltage/MaximalVoltage (диапазон напряжения),
                    DischargerVoltage (напряжение разрядника), ConnectorMask (маска разъёмов).

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
CATALOG_SERIES_NAMES = "SeriesNames"
CATALOG_GROUPS_NAMES = "GroupsNames"
CATALOG_DEVICES_NAMES = "DevicesNames"
CATALOG_SEQUENCES = "Sequences"
CATALOG_SEQUENCES_INPUT_PIN = "InputPin"
CATALOG_SEQUENCES_OUTPUT_PIN = "OutputPin"

DEVICES_SERIA_ID = "SeriaID"
DEVICES_GROUP_ID = "GroupID"
DEVICES_DEVICE_ID = "DeviceID"
DEVICES_SEQUENCE_IDS = "SequenceIDs"
DEVICES_SEQUENCE_AMOUNT = "SequenceIDAmount"
DEVICES_MIN_VOLT = "MinimalVoltage"
DEVICES_MAX_VOLT = "MaximalVoltage"
DEVICES_DISCHARGER = "DischargerVoltage"
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

    series_names = _get_names(catalog, CATALOG_SERIES_NAMES, "каталог")
    groups_names = _get_names(catalog, CATALOG_GROUPS_NAMES, "каталог")
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

    _check_declared_count(catalog, CATALOG_SERIES_AMOUNT, len(series_names), "имён серий")
    _check_declared_count(catalog, CATALOG_GROUPS_AMOUNT, len(groups_names), "имён групп")
    _check_declared_count(catalog, CATALOG_DEVICES_AMOUNT, len(devices_names), "имён приборов")
    _check_declared_count(catalog, CATALOG_SEQUENCES_AMOUNT, len(sequences), "последовательностей")

    for name, count in (("серий", len(series_names)),
                        ("групп", len(groups_names)),
                        ("приборов", len(devices_names)),
                        ("последовательностей", len(sequences))):
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

        require(DEVICES_SEQUENCE_IDS in dev, f"{where}: отсутствует список '" + DEVICES_SEQUENCE_IDS + "'")
        require(isinstance(dev[DEVICES_SEQUENCE_IDS], list), f"{where}: '" + DEVICES_SEQUENCE_IDS + "' должен быть массивом")
        seq_idxs = []
        for k, sidx in enumerate(dev[DEVICES_SEQUENCE_IDS]):
            require(isinstance(sidx, int) and not isinstance(sidx, bool),
                    f"{where}: " + DEVICES_SEQUENCE_IDS + f"[{k}] должен быть целым, получено {sidx!r}")
            require(0 <= sidx < len(sequences),
                    f"{where}: " + DEVICES_SEQUENCE_IDS + f"[{k}] = {sidx}, а последовательностей "
                    f"{len(sequences)} (допустимо 0..{len(sequences) - 1})")
            seq_idxs.append(sidx)

        require(len(seq_idxs) <= MAX_SEQUENCES_PER_DEVICE,
                f"{where}: {len(seq_idxs)} последовательностей, а маска "
                f"результатов в model.h рассчитана на {MAX_SEQUENCES_PER_DEVICE}")

        if DEVICES_SEQUENCE_AMOUNT in dev and dev[DEVICES_SEQUENCE_AMOUNT] != len(seq_idxs):
            warn(f"{where}: " + DEVICES_SEQUENCE_AMOUNT +
                 f" = {dev[DEVICES_SEQUENCE_AMOUNT]}, а в " + DEVICES_SEQUENCE_IDS +
                 f" {len(seq_idxs)} элементов. " +
                 f"Использую фактическое значение {len(seq_idxs)}")

        min_v = _get_int(dev, DEVICES_MIN_VOLT, where, 0, MAX_VOLTAGE)
        max_v = _get_int(dev, DEVICES_MAX_VOLT, where, 0, MAX_VOLTAGE)
        require(min_v <= max_v,
                f"{where}: " + DEVICES_MIN_VOLT + f" = {min_v} больше " + DEVICES_MAX_VOLT + f" = {max_v}")
        discharger = _get_int(dev, DEVICES_DISCHARGER, where, 0, MAX_VOLTAGE)
        connector = _get_int(dev, DEVICES_CONNECTOR_MASK, where, 1, MAX_CONNECTOR_MASK)

        devices.append({
            "seria_idx": seria_idx,
            "group_idx": group_idx,
            "device_idx": device_idx,
            "seq_idxs": seq_idxs,
            "min_v": min_v,
            "max_v": max_v,
            "discharger": discharger,
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

    return {
        "series_names": series_names,
        "groups_names": groups_names,
        "devices_names": devices_names,
        "sequences": sequences,
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

    seq_idx_pool = []
    device_seq_offsets = []
    for dev in cfg["devices"]:
        device_seq_offsets.append(len(seq_idx_pool))
        seq_idx_pool.extend(dev["seq_idxs"])

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

    # --- индексы последовательностей приборов ---
    add("/** @brief Общий пул индексов последовательностей для всех приборов. */")
    add("static const uint8_t cfg_sequence_idxs[] CFG_DATA = {")
    if seq_idx_pool:
        for i in range(0, len(seq_idx_pool), 16):
            chunk = ", ".join(str(v) for v in seq_idx_pool[i:i + 16])
            add(f"    {chunk},")
    else:
        add("    0, /* ни одному прибору не назначено последовательностей */")
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
        add(f"        .sequence_idxs = cfg_sequence_idxs + {off},")
        add(f"        .num_seqs = {len(dev['seq_idxs'])},")
        add(f"        .minimal_voltage_test = {dev['min_v']},")
        add(f"        .maximal_voltage_test = {dev['max_v']},")
        add(f"        .discharger_voltage = {dev['discharger']},")
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
    add("    .series_names = cfg_series_names,")
    add("    .groups_names = cfg_groups_names,")
    add("    .devices_names = cfg_devices_names,")
    add("    .sequences = cfg_sequences,")
    add("    .devices = cfg_devices")
    add("};")
    add("")

    estimate = estimate_size(cfg, pool, seq_idx_pool)
    require(estimate <= CONFIG_REGION_SIZE,
            f"конфигурация занимает примерно {estimate} байт, а регион CONFIG "
            f"в STM32F103XB_FLASH.ld рассчитан на {CONFIG_REGION_SIZE}")

    return "\n".join(out) + "\n", estimate


def estimate_size(cfg, pool, seq_idx_pool):
    """Оценка сверху для занимаемого места во Flash (для ранней диагностики)."""
    n_names = (len(cfg["series_names"]) + len(cfg["groups_names"]) +
               len(cfg["devices_names"]))
    return (
        len(pool)
        + n_names * 8                     # MyString: два указателя
        + len(cfg["sequences"]) * 2       # Params_TestSequence_TypeDef
        + len(seq_idx_pool)
        + len(cfg["devices"]) * 20        # Params_Device_TypeDef с выравниванием
        + 32                              # Params_TypeDef
        + 64                              # запас на выравнивание секций
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
          f"последовательностей {len(cfg['sequences'])}")
    print(f"config: во Flash примерно {estimate} байт из {CONFIG_REGION_SIZE}.")
    if _warnings:
        print(f"config: предупреждений: {len(_warnings)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
