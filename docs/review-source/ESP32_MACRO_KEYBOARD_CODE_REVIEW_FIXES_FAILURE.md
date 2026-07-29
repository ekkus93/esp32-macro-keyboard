# Code-review fixes validation failure

**Stage:** `authoritative repository gate`

**Exit status:** `1`

The production changes were not published. The one-shot workflow, runner, and verified payload remain on `master` for deterministic correction.

## Log tail

```text
[15/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_spiflash.c.obj
[16/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_uart.c.obj
[17/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_systimer.c.obj
[18/123] Building ASM object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_cache_writeback_esp32s3.S.obj
[19/123] Linking C static library esp-idf/log/liblog.a
[20/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_wdt.c.obj
[21/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_cache_esp32s2_esp32s3.c.obj
[22/123] Building C object esp-idf/esp_common/CMakeFiles/__idf_esp_common.dir/src/esp_err_to_name.c.obj
[23/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_print.c.obj
[24/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/esp_cpu_intr.c.obj
[25/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/cpu_region_protect.c.obj
[26/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/cpu.c.obj
[27/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/esp_memory_utils.c.obj
[28/123] Linking C static library esp-idf/esp_rom/libesp_rom.a
[29/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/chip_info.c.obj
[30/123] Linking C static library esp-idf/esp_common/libesp_common.a
[31/123] Building C object esp-idf/esp_system/CMakeFiles/__idf_esp_system.dir/esp_err.c.obj
[32/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/esp32s3/esp_efuse_table.c.obj
[33/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_clk_init.c.obj
[34/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/esp32s3/esp_efuse_fields.c.obj
[35/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_sleep.c.obj
[36/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/esp32s3/esp_efuse_rtc_calib.c.obj
[37/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_time.c.obj
[38/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_init.c.obj
[39/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_clk.c.obj
[40/123] Linking C static library esp-idf/esp_hw_support/libesp_hw_support.a
[41/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/esp32s3/esp_efuse_utility.c.obj
[42/123] Linking C static library esp-idf/esp_system/libesp_system.a
[43/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/src/esp_efuse_fields.c.obj
[44/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/src/esp_efuse_api.c.obj
[45/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_mem.c.obj
[46/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/src/esp_efuse_utility.c.obj
[47/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_clock_init.c.obj
[48/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_random.c.obj
[49/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_efuse.c.obj
[50/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_common.c.obj
[51/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_common_loader.c.obj
[52/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/src/efuse_controller/keys/with_key_purposes/esp_efuse_api_key.c.obj
[53/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/secure_boot.c.obj
[54/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/flash_encrypt.c.obj
[55/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_random_esp32s3.c.obj
[56/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/bootloader_flash/src/flash_qio_mode.c.obj
[57/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_sha.c.obj
[58/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/flash_partitions.c.obj
[59/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/bootloader_flash/src/bootloader_flash_config_esp32s3.c.obj
[60/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_clock_loader.c.obj
[61/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_console_loader.c.obj
[62/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_utility.c.obj
[63/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/esp32s3/bootloader_soc.c.obj
[64/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/bootloader_flash/src/bootloader_flash.c.obj
[65/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_init.c.obj
[66/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_console.c.obj
[67/123] Linking C static library esp-idf/efuse/libefuse.a
[68/123] Building C object esp-idf/esp_bootloader_format/CMakeFiles/__idf_esp_bootloader_format.dir/esp_bootloader_desc.c.obj
[69/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_panic.c.obj
[70/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/mpu_hal.c.obj
[71/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/esp32s3/bootloader_esp32s3.c.obj
[72/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/hal_utils.c.obj
[73/123] Building C object esp-idf/spi_flash/CMakeFiles/__idf_spi_flash.dir/spi_flash_wrap.c.obj
[74/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/esp32s3/efuse_hal.c.obj
[75/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/efuse_hal.c.obj
[76/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/dport_access_common.c.obj
[77/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/interrupts.c.obj
[78/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/lldesc.c.obj
[79/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/gpio_periph.c.obj
[80/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/uart_periph.c.obj
[81/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/dedic_gpio_periph.c.obj
[82/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/adc_periph.c.obj
[83/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/gdma_periph.c.obj
[84/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/mmu_hal.c.obj
[85/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/spi_periph.c.obj
[86/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/esp_image_format.c.obj
[87/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/cache_hal.c.obj
[88/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/ledc_periph.c.obj
[89/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/pcnt_periph.c.obj
[90/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/sdm_periph.c.obj
[91/123] Linking C static library esp-idf/bootloader_support/libbootloader_support.a
[92/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/rmt_periph.c.obj
[93/123] Linking C static library esp-idf/esp_bootloader_format/libesp_bootloader_format.a
[94/123] Linking C static library esp-idf/spi_flash/libspi_flash.a
[95/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/i2c_periph.c.obj
[96/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/i2s_periph.c.obj
[97/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/temperature_sensor_periph.c.obj
[98/123] Linking C static library esp-idf/hal/libhal.a
[99/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/timer_periph.c.obj
[100/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/mcpwm_periph.c.obj
[101/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/touch_sensor_periph.c.obj
[102/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/lcd_periph.c.obj
[103/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/sdmmc_periph.c.obj
[104/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/mpi_periph.c.obj
[105/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/twai_periph.c.obj
[106/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/wdt_periph.c.obj
[107/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/usb_dwc_periph.c.obj
[108/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/cam_periph.c.obj
[109/123] Generating project_elf_src_esp32s3.c
[110/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/rtc_io_periph.c.obj
[111/123] Building C object esp-idf/xtensa/CMakeFiles/__idf_xtensa.dir/eri.c.obj
[112/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/power_supply_periph.c.obj
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
Generated /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/test_app/build/bootloader/bootloader.bin
[123/123] cd /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/test_app/build/bootloader/esp-idf/esptool_py && /home/runner/.espressif/python_env/idf5.5_py3.12_env/bin/python /home/runner/esp/esp-idf-v5.5.5/components/partition_table/check_sizes.py --offset 0x8000 bootloader 0x0 /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/test_app/build/bootloader/bootloader.bin
Bootloader binary size 0x5160 bytes. 0x2ea0 bytes (36%) free.
[1122/1128] No install step for 'bootloader'
[1123/1128] Completed 'bootloader'
[1124/1128] Generating esp-idf/esp_system/ld/sections.ld
[1125/1128] Building C object CMakeFiles/esp32_macro_keyboard_device_tests.elf.dir/project_elf_src_esp32s3.c.obj
[1126/1128] Linking CXX executable esp32_macro_keyboard_device_tests.elf
[1127/1128] Generating binary image from built executable
esptool.py v4.12.0
Creating esp32s3 image...
Merged 2 ELF sections
Successfully created esp32s3 image.
Generated /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/test_app/build/esp32_macro_keyboard_device_tests.bin
[1128/1128] cd /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/test_app/build/esp-idf/esptool_py && /home/runner/.espressif/python_env/idf5.5_py3.12_env/bin/python /home/runner/esp/esp-idf-v5.5.5/components/partition_table/check_sizes.py --offset 0x8000 partition --type app /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/test_app/build/partition_table/partition-table.bin /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/test_app/build/esp32_macro_keyboard_device_tests.bin
esp32_macro_keyboard_device_tests.bin binary size 0x2f200 bytes. Smallest app partition is 0x100000 bytes. 0xd0e00 bytes (82%) free.

Project build complete. To flash, run:
 idf.py flash
or
 idf.py -p PORT flash
or
 python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/esp32_macro_keyboard_device_tests.bin
or from the "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/test_app/build" directory
 python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash "@flash_args"
CMake Warning at /home/runner/esp/esp-idf-v5.5.5/components/esp_common/project_include.cmake:14 (message):
  Building ESP-IDF with clang is an experimental feature and is not yet
  officially supported.
Call Stack (most recent call first):
  /home/runner/esp/esp-idf-v5.5.5/tools/cmake/build.cmake:471 (include)
  /home/runner/esp/esp-idf-v5.5.5/tools/cmake/build.cmake:722 (__build_process_project_includes)
  /home/runner/esp/esp-idf-v5.5.5/tools/cmake/project.cmake:752 (idf_build_process)
  CMakeLists.txt:16 (project)


npm warn deprecated whatwg-encoding@3.1.1: Use @exodus/bytes instead for a more spec-conformant and faster implementation

added 425 packages, and audited 426 packages in 4s

found 0 vulnerabilities
npm warn allow-scripts 2 packages have install scripts not yet covered by allowScripts:
npm warn allow-scripts   @tailwindcss/oxide@4.1.11 (postinstall: node ./scripts/install.js)
npm warn allow-scripts   esbuild@0.28.1 (postinstall: node install.js)
npm warn allow-scripts
npm warn allow-scripts Run `npm approve-scripts --allow-scripts-pending` to review, or `npm approve-scripts <pkg>` to allow.

> esp32-macro-keyboard-webapp@0.1.0 typecheck
> tsc -b --pretty false


> esp32-macro-keyboard-webapp@0.1.0 lint
> eslint . --max-warnings=0


/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/eslint.config.js
  7:25  error  `config` is deprecated. ESLint core now provides this functionality via `defineConfig()`,
which we now recommend instead. See {@link https://typescript-eslint.io/packages/typescript-eslint/#config-deprecated}  @typescript-eslint/no-deprecated

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/execution/ExecutionPage.tsx
  77:11  error  Prefer using an optional chain expression instead, as it's more concise and easier to read  @typescript-eslint/prefer-optional-chain

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/macros/MacroEditorPage.tsx
   83:7  error  Error: Calling setState synchronously within an effect can trigger cascading renders

Effects are intended to synchronize state between React and external systems such as manually updating the DOM, state management libraries, or other platform APIs. In general, the body of an effect should do one or both of the following:
* Update external systems with the latest state from React.
* Subscribe for updates from some external system, calling setState in a callback function when external state changes.

Calling setState synchronously within an effect body causes cascading renders that can hurt performance, and is not recommended. (https://react.dev/learn/you-might-not-need-an-effect).

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/macros/MacroEditorPage.tsx:83:7
  81 |   useEffect(() => {
  82 |     if (activeSet === null || target.kind === "invalid") {
> 83 |       setDraft(null);
     |       ^^^^^^^^ Avoid calling setState() directly within an effect
  84 |       setPersisted(false);
  85 |       setLoading(false);
  86 |       setLoadError(null);  react-hooks/set-state-in-effect
  148:7  error  Error: Calling setState synchronously within an effect can trigger cascading renders

Effects are intended to synchronize state between React and external systems such as manually updating the DOM, state management libraries, or other platform APIs. In general, the body of an effect should do one or both of the following:
* Update external systems with the latest state from React.
* Subscribe for updates from some external system, calling setState in a callback function when external state changes.

Calling setState synchronously within an effect body causes cascading renders that can hurt performance, and is not recommended. (https://react.dev/learn/you-might-not-need-an-effect).

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/macros/MacroEditorPage.tsx:148:7
  146 |       conflict
  147 |     ) {
> 148 |       setValidation({ kind: "idle" });
      |       ^^^^^^^^^^^^^ Avoid calling setState() directly within an effect
  149 |       return;
  150 |     }
  151 |     let active = true;                                                               react-hooks/set-state-in-effect

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/macros/MacroLibraryPage.tsx
  26:7  error  Error: Calling setState synchronously within an effect can trigger cascading renders

Effects are intended to synchronize state between React and external systems such as manually updating the DOM, state management libraries, or other platform APIs. In general, the body of an effect should do one or both of the following:
* Update external systems with the latest state from React.
* Subscribe for updates from some external system, calling setState in a callback function when external state changes.

Calling setState synchronously within an effect body causes cascading renders that can hurt performance, and is not recommended. (https://react.dev/learn/you-might-not-need-an-effect).

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/macros/MacroLibraryPage.tsx:26:7
  24 |   useEffect(() => {
  25 |     if (activeSet === null) {
> 26 |       setMacros(null);
     |       ^^^^^^^^^ Avoid calling setState() directly within an effect
  27 |       setLoadError(null);
  28 |       return;
  29 |     }  react-hooks/set-state-in-effect

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/procedures/ProcedureLibraryPage.tsx
  38:7  error  Error: Calling setState synchronously within an effect can trigger cascading renders

Effects are intended to synchronize state between React and external systems such as manually updating the DOM, state management libraries, or other platform APIs. In general, the body of an effect should do one or both of the following:
* Update external systems with the latest state from React.
* Subscribe for updates from some external system, calling setState in a callback function when external state changes.

Calling setState synchronously within an effect body causes cascading renders that can hurt performance, and is not recommended. (https://react.dev/learn/you-might-not-need-an-effect).

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/procedures/ProcedureLibraryPage.tsx:38:7
  36 |   useEffect(() => {
  37 |     if (activeSet === null) {
> 38 |       setCards(null);
     |       ^^^^^^^^ Avoid calling setState() directly within an effect
  39 |       setLoadError(null);
  40 |       return;
  41 |     }  react-hooks/set-state-in-effect

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/procedures/ProcedureWorkflowPage.tsx
  108:7  error  Error: Calling setState synchronously within an effect can trigger cascading renders

Effects are intended to synchronize state between React and external systems such as manually updating the DOM, state management libraries, or other platform APIs. In general, the body of an effect should do one or both of the following:
* Update external systems with the latest state from React.
* Subscribe for updates from some external system, calling setState in a callback function when external state changes.

Calling setState synchronously within an effect body causes cascading renders that can hurt performance, and is not recommended. (https://react.dev/learn/you-might-not-need-an-effect).

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/procedures/ProcedureWorkflowPage.tsx:108:7
  106 |   useEffect(() => {
  107 |     if (activeSet === null || targetProcedureId === null) {
> 108 |       setProcedure(null);
      |       ^^^^^^^^^^^^ Avoid calling setState() directly within an effect
  109 |       setProgressState(null);
  110 |       setLoadError(null);
  111 |       return;  react-hooks/set-state-in-effect

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/settings/SettingsPage.tsx
  25:5  error  Error: Calling setState synchronously within an effect can trigger cascading renders

Effects are intended to synchronize state between React and external systems such as manually updating the DOM, state management libraries, or other platform APIs. In general, the body of an effect should do one or both of the following:
* Update external systems with the latest state from React.
* Subscribe for updates from some external system, calling setState in a callback function when external state changes.

Calling setState synchronously within an effect body causes cascading renders that can hurt performance, and is not recommended. (https://react.dev/learn/you-might-not-need-an-effect).

/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/settings/SettingsPage.tsx:25:5
  23 |
  24 |   useEffect(() => {
> 25 |     setRequirePhysicalConfirmation(settings.requirePhysicalConfirmation);
     |     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Avoid calling setState() directly within an effect
  26 |     setAlwaysSelectSet(settings.alwaysSelectSet);
  27 |   }, [settings]);
  28 |  react-hooks/set-state-in-effect

✖ 8 problems (8 errors, 0 warnings)

From https://github.com/ekkus93/esp32-macro-keyboard
 * branch            master     -> FETCH_HEAD
HEAD is now at 1cfa263 ci(code-review-fixes): regenerate lockfile from reviewed manifest
Removing firmware/build-clang/
Removing firmware/build/
Removing firmware/managed_components/
Removing firmware/sdkconfig
Removing firmware/sdkconfig.old
Removing firmware/test_app/build-clang/
Removing firmware/test_app/build/
Removing firmware/test_app/managed_components/
Removing firmware/test_app/sdkconfig
Removing firmware/test_app/sdkconfig.old
Removing scripts/check-schema-byte-limits.sh
Removing tests/scripts/test-check-schema-byte-limits.sh
Removing webapp/node_modules/
Removing webapp/tests/api-execution-submit.test.ts
Removing webapp/tsconfig.app.tsbuildinfo
Removing webapp/tsconfig.node.tsbuildinfo

```
