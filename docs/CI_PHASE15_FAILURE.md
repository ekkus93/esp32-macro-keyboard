# FIX1 Phase 15 automation failure

Apply outcome: success
Apply status: 0
Validation outcome: failure
Validation status: 1
Evidence outcome: skipped

## Apply log

```text
```

## Validation log

```text
[108/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/cam_periph.c.obj
[109/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/power_supply_periph.c.obj
[110/123] Generating project_elf_src_esp32s3.c
[111/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/rtc_io_periph.c.obj
[112/123] Building C object esp-idf/xtensa/CMakeFiles/__idf_xtensa.dir/eri.c.obj
[113/123] Building C object CMakeFiles/bootloader.elf.dir/project_elf_src_esp32s3.c.obj
[114/123] Building C object esp-idf/xtensa/CMakeFiles/__idf_xtensa.dir/xt_trax.c.obj
[115/123] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/bootloader_start.c.obj
[116/123] Building C object esp-idf/micro-ecc/CMakeFiles/__idf_micro-ecc.dir/uECC_verify_antifault.c.obj
[117/123] Linking C static library esp-idf/micro-ecc/libmicro-ecc.a
[118/123] Linking C static library esp-idf/soc/libsoc.a
[119/123] Linking C static library esp-idf/xtensa/libxtensa.a
[120/123] Linking C static library esp-idf/main/libmain.a
[121/123] Linking C executable bootloader.elf
[122/123] Generating binary image from built executable
esptool.py v4.12.0
Creating esp32s3 image...
Merged 2 ELF sections
Successfully created esp32s3 image.
Generated /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/bootloader/bootloader.bin
[123/123] cd /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/bootloader/esp-idf/esptool_py && /home/runner/.espressif/python_env/idf5.5_py3.12_env/bin/python /home/runner/esp/esp-idf-v5.5.5/components/partition_table/check_sizes.py --offset 0x8000 bootloader 0x0 /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/bootloader/bootloader.bin
Bootloader binary size 0x5160 bytes. 0x2ea0 bytes (36%) free.
[1181/1187] No install step for 'bootloader'
[1182/1187] Completed 'bootloader'
[1183/1187] Generating esp-idf/esp_system/ld/sections.ld
[1184/1187] Building C object CMakeFiles/esp32_macro_keyboard.elf.dir/project_elf_src_esp32s3.c.obj
[1185/1187] Linking CXX executable esp32_macro_keyboard.elf
[1186/1187] Generating binary image from built executable
esptool.py v4.12.0
Creating esp32s3 image...
Merged 2 ELF sections
Successfully created esp32s3 image.
Generated /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/esp32_macro_keyboard.bin
[1187/1187] cd /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/esp-idf/esptool_py && /home/runner/.espressif/python_env/idf5.5_py3.12_env/bin/python /home/runner/esp/esp-idf-v5.5.5/components/partition_table/check_sizes.py --offset 0x8000 partition --type app /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/partition_table/partition-table.bin /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/esp32_macro_keyboard.bin
esp32_macro_keyboard.bin binary size 0xe6c90 bytes. Smallest app partition is 0x280000 bytes. 0x199370 bytes (64%) free.

Project build complete. To flash, run:
 idf.py flash
or
 idf.py -p PORT flash
or
 python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0xf000 build/ota_data_initial.bin 0x20000 build/esp32_macro_keyboard.bin
or from the "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build" directory
 python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash "@flash_args"
CMake Warning at /home/runner/esp/esp-idf-v5.5.5/components/esp_common/project_include.cmake:14 (message):
  Building ESP-IDF with clang is an experimental feature and is not yet
  officially supported.
Call Stack (most recent call first):
  /home/runner/esp/esp-idf-v5.5.5/tools/cmake/build.cmake:471 (include)
  /home/runner/esp/esp-idf-v5.5.5/tools/cmake/build.cmake:722 (__build_process_project_includes)
  /home/runner/esp/esp-idf-v5.5.5/tools/cmake/project.cmake:752 (idf_build_process)
  CMakeLists.txt:9 (project)


Enabled checks:
    bugprone-argument-comment
    bugprone-assert-side-effect
    bugprone-assignment-in-if-condition
    bugprone-bad-signal-to-kill-thread
    bugprone-bool-pointer-implicit-conversion
    bugprone-branch-clone
    bugprone-casting-through-void
    bugprone-chained-comparison
    bugprone-compare-pointer-to-member-virtual-function
    bugprone-copy-constructor-init
    bugprone-crtp-constructor-accessibility
    bugprone-dangling-handle
    bugprone-dynamic-static-initializers
    bugprone-easily-swappable-parameters
    bugprone-empty-catch
    bugprone-exception-escape
    bugprone-fold-init-type
    bugprone-forward-declaration-namespace
    bugprone-forwarding-reference-overload
    bugprone-implicit-widening-of-multiplication-result
    bugprone-inaccurate-erase
    bugprone-inc-dec-in-conditions
    bugprone-incorrect-enable-if
    bugprone-incorrect-roundings
    bugprone-infinite-loop
    bugprone-integer-division
    bugprone-lambda-function-name
    bugprone-macro-parentheses
    bugprone-macro-repeated-side-effects
    bugprone-misplaced-operator-in-strlen-in-alloc
    bugprone-misplaced-pointer-arithmetic-in-alloc
    bugprone-misplaced-widening-cast
    bugprone-move-forwarding-reference
    bugprone-multi-level-implicit-pointer-conversion
    bugprone-multiple-new-in-one-expression
    bugprone-multiple-statement-macro
    bugprone-narrowing-conversions
    bugprone-no-escape
    bugprone-non-zero-enum-to-bool-conversion
    bugprone-not-null-terminated-result
    bugprone-optional-value-conversion
    bugprone-parent-virtual-call
    bugprone-pointer-arithmetic-on-polymorphic-object
    bugprone-posix-return
    bugprone-redundant-branch-condition
    bugprone-reserved-identifier
    bugprone-return-const-ref-from-parameter
    bugprone-shared-ptr-array-mismatch
    bugprone-signal-handler
    bugprone-signed-char-misuse
    bugprone-sizeof-container
    bugprone-sizeof-expression
    bugprone-spuriously-wake-up-functions
    bugprone-standalone-empty
    bugprone-string-constructor
    bugprone-string-integer-assignment
    bugprone-string-literal-with-embedded-nul
    bugprone-stringview-nullptr
    bugprone-suspicious-enum-usage
    bugprone-suspicious-include
    bugprone-suspicious-memory-comparison
    bugprone-suspicious-memset-usage
    bugprone-suspicious-missing-comma
    bugprone-suspicious-realloc-usage
    bugprone-suspicious-semicolon
    bugprone-suspicious-string-compare
    bugprone-suspicious-stringview-data-usage
    bugprone-swapped-arguments
    bugprone-switch-missing-default-case
    bugprone-terminating-continue
    bugprone-throw-keyword-missing
    bugprone-too-small-loop-variable
    bugprone-unchecked-optional-access
    bugprone-undefined-memory-manipulation
    bugprone-undelegated-constructor
    bugprone-unhandled-exception-at-new
    bugprone-unhandled-self-assignment
    bugprone-unique-ptr-array-mismatch
    bugprone-unsafe-functions
    bugprone-unused-local-non-trivial-variable
    bugprone-unused-raii
    bugprone-unused-return-value
    bugprone-use-after-move
    bugprone-virtual-near-miss
    cert-err34-c
    cert-str34-c
    clang-analyzer-apiModeling.Errno
    clang-analyzer-apiModeling.TrustNonnull
    clang-analyzer-apiModeling.TrustReturnsNonnull
    clang-analyzer-apiModeling.google.GTest
    clang-analyzer-apiModeling.llvm.CastValue
    clang-analyzer-apiModeling.llvm.ReturnValue
    clang-analyzer-core.BitwiseShift
    clang-analyzer-core.CallAndMessage
    clang-analyzer-core.CallAndMessageModeling
    clang-analyzer-core.DivideZero
    clang-analyzer-core.DynamicTypePropagation
    clang-analyzer-core.NonNullParamChecker
    clang-analyzer-core.NonnilStringConstants
    clang-analyzer-core.NullDereference
    clang-analyzer-core.StackAddrEscapeBase
    clang-analyzer-core.StackAddressEscape
    clang-analyzer-core.UndefinedBinaryOperatorResult
    clang-analyzer-core.VLASize
    clang-analyzer-core.builtin.BuiltinFunctions
    clang-analyzer-core.builtin.NoReturnFunctions
    clang-analyzer-core.uninitialized.ArraySubscript
    clang-analyzer-core.uninitialized.Assign
    clang-analyzer-core.uninitialized.Branch
    clang-analyzer-core.uninitialized.CapturedBlockVariable
    clang-analyzer-core.uninitialized.NewArraySize
    clang-analyzer-core.uninitialized.UndefReturn
    clang-analyzer-cplusplus.ArrayDelete
    clang-analyzer-cplusplus.InnerPointer
    clang-analyzer-cplusplus.Move
    clang-analyzer-cplusplus.NewDelete
    clang-analyzer-cplusplus.NewDeleteLeaks
    clang-analyzer-cplusplus.PlacementNew
    clang-analyzer-cplusplus.PureVirtualCall
    clang-analyzer-cplusplus.SelfAssignment
    clang-analyzer-cplusplus.SmartPtrModeling
    clang-analyzer-cplusplus.StringChecker
    clang-analyzer-cplusplus.VirtualCallModeling
    clang-analyzer-deadcode.DeadStores
    clang-analyzer-fuchsia.HandleChecker
    clang-analyzer-nullability.NullPassedToNonnull
    clang-analyzer-nullability.NullReturnedFromNonnull
    clang-analyzer-nullability.NullabilityBase
    clang-analyzer-nullability.NullableDereferenced
    clang-analyzer-nullability.NullablePassedToNonnull
    clang-analyzer-nullability.NullableReturnedFromNonnull
    clang-analyzer-optin.core.EnumCastOutOfRange
    clang-analyzer-optin.cplusplus.UninitializedObject
    clang-analyzer-optin.cplusplus.VirtualCall
    clang-analyzer-optin.mpi.MPI-Checker
    clang-analyzer-optin.osx.OSObjectCStyleCast
    clang-analyzer-optin.osx.cocoa.localizability.EmptyLocalizationContextChecker
    clang-analyzer-optin.osx.cocoa.localizability.NonLocalizedStringChecker
    clang-analyzer-optin.performance.GCDAntipattern
    clang-analyzer-optin.performance.Padding
    clang-analyzer-optin.portability.UnixAPI
    clang-analyzer-optin.taint.TaintedAlloc
    clang-analyzer-osx.API
    clang-analyzer-osx.MIG
    clang-analyzer-osx.NSOrCFErrorDerefChecker
    clang-analyzer-osx.NumberObjectConversion
    clang-analyzer-osx.OSObjectRetainCount
    clang-analyzer-osx.ObjCProperty
    clang-analyzer-osx.SecKeychainAPI
    clang-analyzer-osx.cocoa.AtSync
    clang-analyzer-osx.cocoa.AutoreleaseWrite
    clang-analyzer-osx.cocoa.ClassRelease
    clang-analyzer-osx.cocoa.Dealloc
    clang-analyzer-osx.cocoa.IncompatibleMethodTypes
    clang-analyzer-osx.cocoa.Loops
    clang-analyzer-osx.cocoa.MissingSuperCall
    clang-analyzer-osx.cocoa.NSAutoreleasePool
    clang-analyzer-osx.cocoa.NSError
    clang-analyzer-osx.cocoa.NilArg
    clang-analyzer-osx.cocoa.NonNilReturnValue
    clang-analyzer-osx.cocoa.ObjCGenerics
    clang-analyzer-osx.cocoa.RetainCount
    clang-analyzer-osx.cocoa.RetainCountBase
    clang-analyzer-osx.cocoa.RunLoopAutoreleaseLeak
    clang-analyzer-osx.cocoa.SelfInit
    clang-analyzer-osx.cocoa.SuperDealloc
    clang-analyzer-osx.cocoa.UnusedIvars
    clang-analyzer-osx.cocoa.VariadicMethodTypes
    clang-analyzer-osx.coreFoundation.CFError
    clang-analyzer-osx.coreFoundation.CFNumber
    clang-analyzer-osx.coreFoundation.CFRetainRelease
    clang-analyzer-osx.coreFoundation.containers.OutOfBounds
    clang-analyzer-osx.coreFoundation.containers.PointerSizedValues
    clang-analyzer-security.FloatLoopCounter
    clang-analyzer-security.PutenvStackArray
    clang-analyzer-security.SetgidSetuidOrder
    clang-analyzer-security.cert.env.InvalidPtr
    clang-analyzer-security.insecureAPI.SecuritySyntaxChecker
    clang-analyzer-security.insecureAPI.UncheckedReturn
    clang-analyzer-security.insecureAPI.bcmp
    clang-analyzer-security.insecureAPI.bcopy
    clang-analyzer-security.insecureAPI.bzero
    clang-analyzer-security.insecureAPI.decodeValueOfObjCType
    clang-analyzer-security.insecureAPI.getpw
    clang-analyzer-security.insecureAPI.gets
    clang-analyzer-security.insecureAPI.mkstemp
    clang-analyzer-security.insecureAPI.mktemp
    clang-analyzer-security.insecureAPI.rand
    clang-analyzer-security.insecureAPI.strcpy
    clang-analyzer-security.insecureAPI.vfork
    clang-analyzer-unix.API
    clang-analyzer-unix.BlockInCriticalSection
    clang-analyzer-unix.DynamicMemoryModeling
    clang-analyzer-unix.Errno
    clang-analyzer-unix.Malloc
    clang-analyzer-unix.MallocSizeof
    clang-analyzer-unix.MismatchedDeallocator
    clang-analyzer-unix.StdCLibraryFunctions
    clang-analyzer-unix.Stream
    clang-analyzer-unix.Vfork
    clang-analyzer-unix.cstring.BadSizeArg
    clang-analyzer-unix.cstring.CStringModeling
    clang-analyzer-unix.cstring.NullArg
    clang-analyzer-valist.CopyToSelf
    clang-analyzer-valist.Uninitialized
    clang-analyzer-valist.Unterminated
    clang-analyzer-valist.ValistBase
    clang-analyzer-webkit.NoUncountedMemberChecker
    clang-analyzer-webkit.RefCntblBaseVirtualDtor
    clang-analyzer-webkit.UncountedLambdaCapturesChecker
    concurrency-thread-canceltype-asynchronous
    misc-confusable-identifiers
    misc-const-correctness
    misc-coroutine-hostile-raii
    misc-definitions-in-headers
    misc-header-include-cycle
    misc-include-cleaner
    misc-misleading-bidirectional
    misc-misleading-identifier
    misc-misplaced-const
    misc-new-delete-overloads
    misc-no-recursion
    misc-non-copyable-objects
    misc-non-private-member-variables-in-classes
    misc-redundant-expression
    misc-static-assert
    misc-throw-by-value-catch-by-reference
    misc-unconventional-assign-operator
    misc-uniqueptr-reset-release
    misc-unused-alias-decls
    misc-unused-parameters
    misc-unused-using-decls
    misc-use-anonymous-namespace
    misc-use-internal-linkage
    performance-avoid-endl
    performance-enum-size
    performance-faster-string-find
    performance-for-range-copy
    performance-implicit-conversion-in-loop
    performance-inefficient-algorithm
    performance-inefficient-string-concatenation
    performance-inefficient-vector-operation
    performance-move-const-arg
    performance-move-constructor-init
    performance-no-automatic-move
    performance-no-int-to-ptr
    performance-noexcept-destructor
    performance-noexcept-move-constructor
    performance-noexcept-swap
    performance-trivially-destructible
    performance-type-promotion-in-math-fn
    performance-unnecessary-copy-initialization
    performance-unnecessary-value-param
    portability-restrict-system-includes
    portability-simd-intrinsics
    portability-std-allocator-const
    readability-avoid-const-params-in-decls
    readability-avoid-nested-conditional-operator
    readability-avoid-return-with-void-value
    readability-avoid-unconditional-preprocessor-if
    readability-braces-around-statements
    readability-const-return-type
    readability-container-contains
    readability-container-data-pointer
    readability-container-size-empty
    readability-convert-member-functions-to-static
    readability-delete-null-pointer
    readability-duplicate-include
    readability-else-after-return
    readability-enum-initial-value
    readability-function-cognitive-complexity
    readability-function-size
    readability-identifier-length
    readability-identifier-naming
    readability-implicit-bool-conversion
    readability-inconsistent-declaration-parameter-name
    readability-isolate-declaration
    readability-magic-numbers
    readability-make-member-function-const
    readability-math-missing-parentheses
    readability-misleading-indentation
    readability-misplaced-array-index
    readability-named-parameter
    readability-operators-representation
    readability-qualified-auto
    readability-redundant-access-specifiers
    readability-redundant-casting
    readability-redundant-control-flow
    readability-redundant-declaration
    readability-redundant-function-ptr-dereference
    readability-redundant-inline-specifier
    readability-redundant-member-init
    readability-redundant-preprocessor
    readability-redundant-smartptr-get
    readability-redundant-string-cstr
    readability-redundant-string-init
    readability-reference-to-constructed-temporary
    readability-simplify-boolean-expr
    readability-simplify-subscript-expr
    readability-static-accessed-through-instance
    readability-static-definition-in-anonymous-namespace
    readability-string-compare
    readability-suspicious-call-argument
    readability-uniqueptr-delete-release
    readability-uppercase-literal-suffix
    readability-use-anyofallof
    readability-use-std-min-max

Running clang-tidy for 67 files out of 1059 in compilation database ...
[ 1/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard_state.c
134 warnings generated.
Suppressed 134 warnings (134 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[ 2/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c
144 warnings generated.
Suppressed 144 warnings (144 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[ 3/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_common.c
1499 warnings generated.
Suppressed 1499 warnings (1499 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[ 4/67][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c
894 warnings generated.
Suppressed 894 warnings (894 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[ 5/67][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c
667 warnings generated.
Suppressed 667 warnings (667 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[ 6/67][2.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c
1451 warnings generated.
Suppressed 1451 warnings (1451 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[ 7/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_static.c
1447 warnings generated.
Suppressed 1447 warnings (1447 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[ 8/67][2.7s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c
672 warnings generated.
Suppressed 672 warnings (672 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[ 9/67][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor_engine.c
682 warnings generated.
Suppressed 682 warnings (682 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[10/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c
1440 warnings generated.
Suppressed 1440 warnings (1440 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[11/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_logout_execution.c
1442 warnings generated.
Suppressed 1442 warnings (1442 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[12/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c
749 warnings generated.
Suppressed 749 warnings (749 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[13/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap.c
1446 warnings generated.
Suppressed 1446 warnings (1446 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[14/67][0.0s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c
[15/67][4.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c
1132 warnings generated.
Suppressed 1132 warnings (1132 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[16/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount.c
879 warnings generated.
Suppressed 879 warnings (879 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[17/67][0.9s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c
1128 warnings generated.
Suppressed 1128 warnings (1128 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[18/67][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c
692 warnings generated.
Suppressed 692 warnings (692 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[19/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_setup.c
1447 warnings generated.
Suppressed 1447 warnings (1447 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[20/67][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_common.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[21/67][4.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core_sequence.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[22/67][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_cancel.c
1437 warnings generated.
Suppressed 1437 warnings (1437 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[23/67][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c
1440 warnings generated.
Suppressed 1440 warnings (1440 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[24/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_lifecycle.c
1442 warnings generated.
Suppressed 1442 warnings (1442 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[25/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_body_auth.c
154 warnings generated.
Suppressed 154 warnings (154 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[26/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/main/app_main.c
745 warnings generated.
Suppressed 745 warnings (745 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[27/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_login.c
1504 warnings generated.
Suppressed 1504 warnings (1504 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[28/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c
816 warnings generated.
Suppressed 816 warnings (816 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[29/67][1.6s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c
1445 warnings generated.
Suppressed 1445 warnings (1445 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[30/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c
672 warnings generated.
Suppressed 672 warnings (672 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[31/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c
913 warnings generated.
Suppressed 913 warnings (913 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[32/67][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core.c
996 warnings generated.
Suppressed 996 warnings (996 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[33/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c
682 warnings generated.
Suppressed 682 warnings (682 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[34/67][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_json.c
867 warnings generated.
Suppressed 867 warnings (867 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[35/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard.c
1454 warnings generated.
Suppressed 1454 warnings (1454 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[36/67][0.6s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor.c
1308 warnings generated.
Suppressed 1308 warnings (1308 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[37/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_lifecycle.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[38/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c
15 warnings generated.
Suppressed 15 warnings (15 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[39/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_status_limits.c
1437 warnings generated.
Suppressed 1437 warnings (1437 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[40/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/device_controls/device_controls.c
1315 warnings generated.
Suppressed 1315 warnings (1315 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[41/67][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_rate_limit.c
311 warnings generated.
Suppressed 311 warnings (311 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[42/67][3.8s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_common.c
149 warnings generated.
Suppressed 149 warnings (149 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[43/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c
911 warnings generated.
Suppressed 911 warnings (911 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[44/67][5.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c
1042 warnings generated.
Suppressed 1042 warnings (1042 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[45/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c
677 warnings generated.
Suppressed 677 warnings (677 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[46/67][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c
857 warnings generated.
Suppressed 857 warnings (857 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[47/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_password.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[48/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[49/67][0.9s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c
908 warnings generated.
Suppressed 908 warnings (908 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[50/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_descriptors.c
1454 warnings generated.
Suppressed 1454 warnings (1454 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[51/67][8.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c
1186 warnings generated.
Suppressed 1186 warnings (1186 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[52/67][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_static_stream.c
876 warnings generated.
Suppressed 876 warnings (876 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[53/67][0.7s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning.c
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning.c:226:65: error: no header providing "app_uuid_t" is directly included [misc-include-cleaner,-warnings-as-errors]
    8 | app_error_code_t provisioning_clear_active_set_if_matches(const app_uuid_t *set_id,
      |                                                                 ^
1321 warnings generated.
Suppressed 1320 warnings (1320 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
1 warning treated as error

[54/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c
1440 warnings generated.
Suppressed 1440 warnings (1440 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[55/67][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap_state.c
682 warnings generated.
Suppressed 682 warnings (682 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[56/67][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap.c
874 warnings generated.
Suppressed 874 warnings (874 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[57/67][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c
672 warnings generated.
Suppressed 672 warnings (672 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[58/67][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c
902 warnings generated.
Suppressed 902 warnings (902 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[59/67][3.9s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c
692 warnings generated.
Suppressed 692 warnings (692 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[60/67][4.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c
866 warnings generated.
Suppressed 866 warnings (866 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[61/67][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth.c
1479 warnings generated.
Suppressed 1479 warnings (1479 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[62/67][2.7s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c
682 warnings generated.
Suppressed 682 warnings (682 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[63/67][3.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c
762 warnings generated.
Suppressed 762 warnings (762 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[64/67][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c
1285 warnings generated.
Suppressed 1285 warnings (1285 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[65/67][7.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/device_controls/device_controls_logic.c
672 warnings generated.
Suppressed 672 warnings (672 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[66/67][3.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c
759 warnings generated.
Suppressed 759 warnings (759 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[67/67][4.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c
1115 warnings generated.
Suppressed 1115 warnings (1115 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

error: run-clang-tidy failed for /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware with status 1
```

## Evidence log

```text
```
