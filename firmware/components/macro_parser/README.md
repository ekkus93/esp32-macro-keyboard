# Macro Parser Component

This component implements the version 0.1 US-English macro language:

- printable US-ASCII mapping
- named keys
- modifier chords
- bounded delay directives
- escaped literal braces
- exact byte, line, and column diagnostics
- compiled-action and duration limits
- no partial plan on failure

The public compiler returns an immutable action plan for the executor. Parsing and
validation always finish before any USB report can be emitted.

Host coverage is in `tests/host/test_macro_parser.c`. Physical-device coverage is
in `firmware/test_app/main/test_macro_parser.c`. The authoritative syntax reference
is `docs/MACRO_LANGUAGE.md`.
