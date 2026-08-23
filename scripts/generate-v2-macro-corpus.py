#!/usr/bin/env python3
"""Generate a C include from the shared v2 macro-conformance JSON corpus."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

ERROR_CODES = {
    "invalid_argument": "APP_ERROR_INVALID_ARGUMENT",
    "macro_syntax": "APP_ERROR_MACRO_SYNTAX",
    "macro_limit": "APP_ERROR_MACRO_LIMIT",
}
ACTION_KINDS = {
    "key": "MACRO_ACTION_KEY",
    "chord": "MACRO_ACTION_CHORD",
    "delay": "MACRO_ACTION_DELAY",
}


def require_int(value: Any, context: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise ValueError(f"{context} must be a nonnegative integer")
    return value


def require_string(value: Any, context: str) -> str:
    if not isinstance(value, str):
        raise ValueError(f"{context} must be a string")
    return value


def c_string(value: str) -> str:
    if any(ord(character) > 0x7F for character in value):
        raise ValueError("generated C metadata strings must remain ASCII")
    return json.dumps(value)


def source_array(index: int, source: str) -> list[str]:
    encoded = source.encode("utf-8")
    values = ", ".join(f"UINT8_C(0x{byte:02x})" for byte in encoded)
    if values:
        values += ", "
    values += "UINT8_C(0x00)"
    return [f"static const uint8_t v2_source_{index}[] = {{{values}}};"]


MACRO_ACTION_USAGES_MAX = 6


def action_array(index: int, actions: list[Any]) -> list[str]:
    if not actions:
        return []
    lines = [f"static const macro_action_t v2_actions_{index}[] = {{"]
    for action_index, raw_action in enumerate(actions):
        if not isinstance(raw_action, dict):
            raise ValueError(f"case {index} action {action_index} must be an object")
        kind = require_string(raw_action.get("kind"), "action kind")
        if kind not in ACTION_KINDS:
            raise ValueError(f"case {index} has unsupported action kind {kind}")
        # "key" (a literal character, named directive, or standalone modifier
        # tap) carries at most one usage, kept as a scalar for readability;
        # "chord" (a [...] simultaneous-key group) carries a usages array,
        # since that is the whole point of the group.
        expected_keys = {
            "delay": {"kind", "durationMs"},
            "key": {"kind", "usage", "modifiers"},
            "chord": {"kind", "usages", "modifiers"},
        }[kind]
        if set(raw_action) != expected_keys:
            raise ValueError(f"case {index} action {action_index} has wrong fields")
        if kind == "delay":
            delay = require_int(raw_action["durationMs"], "delay duration")
            usages: list[int] = []
            modifiers = 0
        elif kind == "key":
            delay = 0
            usage = require_int(raw_action["usage"], "HID usage")
            usages = [] if usage == 0 else [usage]
            modifiers = require_int(raw_action["modifiers"], "HID modifiers")
        else:
            delay = 0
            raw_usages = raw_action["usages"]
            if not isinstance(raw_usages, list):
                raise ValueError(f"case {index} action {action_index} usages must be an array")
            usages = [require_int(value, "HID usage") for value in raw_usages]
            if len(usages) != len(set(usages)):
                raise ValueError(f"case {index} action {action_index} has a duplicate usage")
            modifiers = require_int(raw_action["modifiers"], "HID modifiers")
        if len(usages) > MACRO_ACTION_USAGES_MAX:
            raise ValueError(
                f"case {index} action {action_index} exceeds "
                f"MACRO_ACTION_USAGES_MAX ({MACRO_ACTION_USAGES_MAX})"
            )
        if any(usage > 0xFF for usage in usages) or modifiers > 0xFF:
            raise ValueError("HID usage and modifiers must fit uint8_t")
        usages_initializer = ", ".join(f"UINT8_C({usage})" for usage in usages) or "0"
        lines.extend(
            [
                "    {",
                f"        .type = {ACTION_KINDS[kind]},",
                f"        .modifiers = UINT8_C({modifiers}),",
                f"        .usages = {{{usages_initializer}}},",
                f"        .usage_count = UINT8_C({len(usages)}),",
                f"        .delay_ms = UINT32_C({delay}),",
                "    },",
            ]
        )
    lines.append("};")
    return lines


def render(document: Any) -> str:
    if not isinstance(document, dict):
        raise ValueError("macro corpus root must be an object")
    if set(document) != {"format", "version", "cases"}:
        raise ValueError("macro corpus root has wrong fields")
    if document["format"] != "esp32-macro-keyboard-macro-conformance":
        raise ValueError("macro corpus format is invalid")
    if document["version"] != 1:
        raise ValueError("macro corpus version is unsupported")
    cases = document["cases"]
    if not isinstance(cases, list) or not cases:
        raise ValueError("macro corpus cases must be a nonempty array")

    declarations: list[str] = [
        "/* Generated by scripts/generate-v2-macro-corpus.py. */",
        "",
    ]
    initializers: list[str] = [
        "static const v2_macro_case_t v2_macro_cases[] = {",
    ]

    for index, raw_case in enumerate(cases):
        if not isinstance(raw_case, dict):
            raise ValueError(f"case {index} must be an object")
        required = {"name", "source", "keyPressMs", "interKeyMs"}
        result_fields = {"valid", "invalid"} & set(raw_case)
        if not required.issubset(raw_case) or len(result_fields) != 1:
            raise ValueError(f"case {index} must contain exactly one result")
        if set(raw_case) != required | result_fields:
            raise ValueError(f"case {index} has unknown fields")

        name = require_string(raw_case["name"], "case name")
        source = require_string(raw_case["source"], "case source")
        key_press = require_int(raw_case["keyPressMs"], "keyPressMs")
        inter_key = require_int(raw_case["interKeyMs"], "interKeyMs")
        declarations.extend(source_array(index, source))

        valid = raw_case.get("valid")
        invalid = raw_case.get("invalid")
        if valid is not None:
            if not isinstance(valid, dict) or set(valid) != {
                "estimatedDurationMs",
                "actions",
            }:
                raise ValueError(f"case {index} valid result has wrong fields")
            actions = valid["actions"]
            if not isinstance(actions, list):
                raise ValueError(f"case {index} actions must be an array")
            declarations.extend(action_array(index, actions))
            action_pointer = f"v2_actions_{index}" if actions else "NULL"
            error_code = "APP_ERROR_NONE"
            error_offset = 0
            error_line = 0
            error_column = 0
            message_class = ""
            duration = require_int(
                valid["estimatedDurationMs"], "estimatedDurationMs"
            )
        else:
            if not isinstance(invalid, dict) or set(invalid) != {
                "code",
                "byteOffset",
                "line",
                "column",
                "messageClass",
            }:
                raise ValueError(f"case {index} invalid result has wrong fields")
            code = require_string(invalid["code"], "error code")
            if code not in ERROR_CODES:
                raise ValueError(f"case {index} has unsupported error code {code}")
            actions = []
            action_pointer = "NULL"
            error_code = ERROR_CODES[code]
            error_offset = require_int(invalid["byteOffset"], "byteOffset")
            error_line = require_int(invalid["line"], "line")
            error_column = require_int(invalid["column"], "column")
            message_class = require_string(
                invalid["messageClass"], "messageClass"
            )
            duration = 0

        declarations.append("")
        initializers.extend(
            [
                "    {",
                f"        .name = {c_string(name)},",
                f"        .source = v2_source_{index},",
                f"        .source_length = {len(source.encode('utf-8'))}U,",
                f"        .key_press_ms = UINT32_C({key_press}),",
                f"        .inter_key_ms = UINT32_C({inter_key}),",
                f"        .valid = {'true' if valid is not None else 'false'},",
                f"        .expected_duration_ms = UINT32_C({duration}),",
                f"        .expected_actions = {action_pointer},",
                f"        .expected_action_count = {len(actions)}U,",
                f"        .expected_error_code = {error_code},",
                f"        .expected_byte_offset = {error_offset}U,",
                f"        .expected_line = {error_line}U,",
                f"        .expected_column = {error_column}U,",
                f"        .expected_message_class = {c_string(message_class)},",
                "    },",
            ]
        )

    initializers.extend(
        [
            "};",
            "",
            "static const size_t v2_macro_case_count =",
            "    sizeof(v2_macro_cases) / sizeof(v2_macro_cases[0]);",
            "",
        ]
    )
    return "\n".join(declarations + initializers)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    arguments = parser.parse_args()

    document = json.loads(arguments.input.read_text(encoding="utf-8"))
    output = render(document)
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(output, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
