#!/usr/bin/env python3
"""Regression tests for the v2 macro-corpus C generator."""

from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import ModuleType
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
GENERATOR_PATH = REPOSITORY_ROOT / "scripts/generate-v2-macro-corpus.py"
CORPUS_PATH = REPOSITORY_ROOT / "contracts/v2/macro-conformance.json"


def load_generator() -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "generate_v2_macro_corpus", GENERATOR_PATH
    )
    if specification is None or specification.loader is None:
        raise RuntimeError("unable to load macro corpus generator")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


class MacroCorpusGeneratorTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.generator = load_generator()
        cls.corpus: Any = json.loads(CORPUS_PATH.read_text(encoding="utf-8"))

    def test_render_is_deterministic_and_contains_all_cases(self) -> None:
        first = self.generator.render(self.corpus)
        second = self.generator.render(self.corpus)
        self.assertEqual(first, second)
        self.assertIn("static const v2_macro_case_t v2_macro_cases[]", first)
        self.assertIn("static const size_t v2_macro_case_count", first)
        self.assertEqual(first.count("        .name = "), len(self.corpus["cases"]))
        self.assertNotIn("cJSON", first)

    def test_non_ascii_source_is_emitted_as_utf8_bytes(self) -> None:
        rendered = self.generator.render(self.corpus)
        self.assertIn("UINT8_C(0xc3), UINT8_C(0xa9)", rendered)
        self.assertNotIn('"é"', rendered)

    def test_rejects_unknown_root_fields(self) -> None:
        invalid = dict(self.corpus)
        invalid["unexpected"] = True
        with self.assertRaisesRegex(ValueError, "root has wrong fields"):
            self.generator.render(invalid)

    def test_rejects_case_with_both_results(self) -> None:
        invalid = json.loads(json.dumps(self.corpus))
        invalid["cases"][0]["invalid"] = {
            "code": "macro_syntax",
            "byteOffset": 0,
            "line": 1,
            "column": 1,
            "messageClass": "unknown_key",
        }
        with self.assertRaisesRegex(ValueError, "exactly one result"):
            self.generator.render(invalid)

    def test_rejects_action_with_unknown_fields(self) -> None:
        invalid = json.loads(json.dumps(self.corpus))
        action = invalid["cases"][1]["valid"]["actions"][0]
        action["unexpected"] = 1
        with self.assertRaisesRegex(ValueError, "action 0 has wrong fields"):
            self.generator.render(invalid)

    def test_command_line_writes_same_output_as_render(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output_path = Path(directory) / "generated.inc"
            subprocess.run(
                [
                    sys.executable,
                    str(GENERATOR_PATH),
                    "--input",
                    str(CORPUS_PATH),
                    "--output",
                    str(output_path),
                ],
                check=True,
                cwd=REPOSITORY_ROOT,
            )
            self.assertEqual(
                output_path.read_text(encoding="utf-8"),
                self.generator.render(self.corpus),
            )


if __name__ == "__main__":
    unittest.main()
