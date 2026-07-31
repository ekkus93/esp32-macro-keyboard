from pathlib import Path

helper = Path("tools/phase18_3_tree_impl.py")
text = helper.read_text()

replacements = {
    '''    join_path(path, sizeof(path), out_set_path "/macros", LOCAL_MACRO_ID ".json");
    write_text(path, LOCAL_MACRO_JSON);
    join_path(path, sizeof(path), out_set_path "/procedures", PROCEDURE_ID ".json");
    write_text(path, PROCEDURE_JSON);
    join_path(path, sizeof(path), out_set_path "/progress", PROCEDURE_ID ".json");
    write_text(path, PROGRESS_JSON);
''': '''    char directory_path[APP_PATH_MAX_BYTES];
    join_path(directory_path, sizeof(directory_path), out_set_path, "macros");
    join_path(path, sizeof(path), directory_path, LOCAL_MACRO_ID ".json");
    write_text(path, LOCAL_MACRO_JSON);
    join_path(directory_path, sizeof(directory_path), out_set_path, "procedures");
    join_path(path, sizeof(path), directory_path, PROCEDURE_ID ".json");
    write_text(path, PROCEDURE_JSON);
    join_path(directory_path, sizeof(directory_path), out_set_path, "progress");
    join_path(path, sizeof(path), directory_path, PROCEDURE_ID ".json");
    write_text(path, PROGRESS_JSON);
''',
    '''    join_path(path, sizeof(path), set_path "/macros", "not-a-uuid.json");
''': '''    char directory_path[APP_PATH_MAX_BYTES];
    join_path(directory_path, sizeof(directory_path), set_path, "macros");
    join_path(path, sizeof(path), directory_path, "not-a-uuid.json");
''',
    '''    join_path(path, sizeof(path), set_path "/progress", PROCEDURE_ID ".json");
''': '''    char progress_path[APP_PATH_MAX_BYTES];
    join_path(progress_path, sizeof(progress_path), set_path, "progress");
    join_path(path, sizeof(path), progress_path, PROCEDURE_ID ".json");
''',
}

for old, new in replacements.items():
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"expected one set-tree fixture replacement, found {count}")
    text = text.replace(old, new, 1)

text += r'''

extra_tests = Path("tests/host/cmake/extra_tests.cmake")
extra_text = extra_tests.read_text()
extra_needle = '    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_transaction.c"\n'
if extra_text.count(extra_needle) != 2:
    raise SystemExit(
        f"expected two modular transaction source entries, found {extra_text.count(extra_needle)}"
    )
extra_replacement = (
    extra_needle
    + '    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_transaction_replace.c"\n'
)
extra_tests.write_text(extra_text.replace(extra_needle, extra_replacement))
'''

exec(compile(text, str(helper), "exec"), {"__name__": "__main__"})
