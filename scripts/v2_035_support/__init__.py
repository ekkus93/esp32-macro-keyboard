"""Support modules for the V2-035 hardware evidence collector.

The collector entry point stays at ``scripts/run-v2-035-hardware.py``: roughly
ten evidence documents under ``docs/implementation-v2/`` cite that exact path,
``scripts/run-h5-055-hardware.py`` loads it, and
``tests/scripts/test-v2-035-hardware.py`` loads it by file path with
``importlib.util.spec_from_file_location``. A module loaded that way has no
package context, so the entry point puts its own directory on ``sys.path``
before importing this package, and re-exports every name those two dependents
reach for.
"""
