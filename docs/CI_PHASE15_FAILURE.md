# FIX1 Phase 15 automation failure

Apply outcome: success
Apply status: 0
Validation outcome: failure
Validation status: 1
Evidence outcome: skipped
Evidence status: 

## Apply log

```text
```

## Validation log

```text
-- The C compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "1.8.1")
-- Checking for module 'libcjson'
--   Found libcjson, version 1.7.17
CMake Error at cmake/extra_tests.cmake:77 (add_executable):
  add_executable cannot create target "storage_macro_repository_tests"
  because another target with the same name already exists.  The existing
  target is an executable created in source directory
  "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host".
  See documentation for policy CMP0002 for more details.
Call Stack (most recent call first):
  cmake/host_test_project.cmake:5 (include)
  CMakeLists.txt:DEFERRED


CMake Error at cmake/extra_tests.cmake:113 (add_executable):
  add_executable cannot create target "storage_procedure_repository_tests"
  because another target with the same name already exists.  The existing
  target is an executable created in source directory
  "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host".
  See documentation for policy CMP0002 for more details.
Call Stack (most recent call first):
  cmake/host_test_project.cmake:5 (include)
  CMakeLists.txt:DEFERRED


-- Configuring incomplete, errors occurred!
```

## Evidence log

```text
```
