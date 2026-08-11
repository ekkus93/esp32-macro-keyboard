from pathlib import Path

path = Path("scripts/tmp_h9_patch4.py")
text = path.read_text()
old = '''# Host send-route target now compiles the real settings-backed confirmation adapter.\nreplace_once(\n    "tests/host/CMakeLists.txt",\n    "            ../../firmware/components/auth/include\\n            ../../firmware/components/storage/include\\n            ../../firmware/components/wifi_ap/include\\n",\n    "            ../../firmware/components/auth/include\\n            ../../firmware/components/device_settings/include\\n            ../../firmware/components/storage/include\\n            ../../firmware/components/wifi_ap/include\\n",\n)\n'''
new = '''# Host send-route target now compiles the real settings-backed confirmation adapter.\ncmake_path = Path("tests/host/CMakeLists.txt")\ncmake_text = cmake_path.read_text()\nstart = cmake_text.index("add_executable(\\n    web_server_send_route_tests")\nend = cmake_text.index("target_compile_definitions(\\n    web_server_send_route_tests", start)\nblock = cmake_text[start:end]\nneedle = "            ../../firmware/components/auth/include\\n            ../../firmware/components/storage/include\\n"\nif block.count(needle) != 1:\n    raise SystemExit(f"send-route include anchor count: {block.count(needle)}")\nblock = block.replace(needle, "            ../../firmware/components/auth/include\\n            ../../firmware/components/device_settings/include\\n            ../../firmware/components/storage/include\\n", 1)\ncmake_path.write_text(cmake_text[:start] + block + cmake_text[end:])\n'''
if text.count(old) != 1:
    raise SystemExit(f"tmp_h9_patch4 CMake block count: {text.count(old)}")
path.write_text(text.replace(old, new, 1))
