# Phase 14 integration validation

- Result: **PASS**
- Source commit: `76753d74509065133ae56bcee8a6711e6d7b0f72`
- Host tests: normal, ASan/UBSan, and strict coverage
- Firmware: ESP-IDF `v5.5.5`, target `esp32s3`

## Validation log tail

```text
[1085/1180] Building C object esp-idf/protocomm/CMakeFiles/__idf_protocomm.dir/src/crypto/srp6a/esp_srp.c.obj
[1086/1180] Building C object esp-idf/unity/CMakeFiles/__idf_unity.dir/unity/src/unity.c.obj
[1087/1180] Linking C static library esp-idf/unity/libunity.a
[1088/1180] Building C object esp-idf/app_trace/CMakeFiles/__idf_app_trace.dir/app_trace_util.c.obj
[1089/1180] Building C object esp-idf/app_trace/CMakeFiles/__idf_app_trace.dir/host_file_io.c.obj
[1090/1180] Building C object esp-idf/app_trace/CMakeFiles/__idf_app_trace.dir/port/port_uart.c.obj
[1091/1180] Building C object esp-idf/app_core/CMakeFiles/__idf_app_core.dir/app_core_sequence.c.obj
[1092/1180] Building C object esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.obj
[1093/1180] Building C object esp-idf/app_trace/CMakeFiles/__idf_app_trace.dir/app_trace.c.obj
[1094/1180] Building C object esp-idf/esp_driver_cam/CMakeFiles/__idf_esp_driver_cam.dir/dvp_share_ctrl.c.obj
[1095/1180] Building C object esp-idf/esp_driver_cam/CMakeFiles/__idf_esp_driver_cam.dir/dvp/src/esp_cam_ctlr_dvp_gdma.c.obj
[1096/1180] Building C object esp-idf/esp_driver_cam/CMakeFiles/__idf_esp_driver_cam.dir/esp_cam_ctlr.c.obj
[1097/1180] Building C object esp-idf/esp_eth/CMakeFiles/__idf_esp_eth.dir/src/esp_eth_netif_glue.c.obj
[1098/1180] Building C object esp-idf/esp_hid/CMakeFiles/__idf_esp_hid.dir/src/esp_hidd.c.obj
[1099/1180] Building C object esp-idf/esp_eth/CMakeFiles/__idf_esp_eth.dir/src/esp_eth.c.obj
[1100/1180] Building C object esp-idf/esp_driver_touch_sens/CMakeFiles/__idf_esp_driver_touch_sens.dir/common/touch_sens_common.c.obj
[1101/1180] Building C object esp-idf/esp_driver_cam/CMakeFiles/__idf_esp_driver_cam.dir/dvp/src/esp_cam_ctlr_dvp_cam.c.obj
[1102/1180] Linking C static library esp-idf/esp_https_server/libesp_https_server.a
[1103/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/src/esp_lcd_common.c.obj
[1104/1180] Building C object esp-idf/esp_driver_touch_sens/CMakeFiles/__idf_esp_driver_touch_sens.dir/hw_ver2/touch_version_specific.c.obj
[1105/1180] Building C object esp-idf/esp_hid/CMakeFiles/__idf_esp_hid.dir/src/esp_hid_common.c.obj
[1106/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/src/esp_lcd_panel_io.c.obj
[1107/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/src/esp_lcd_panel_nt35510.c.obj
[1108/1180] Building C object esp-idf/esp_eth/CMakeFiles/__idf_esp_eth.dir/src/phy/esp_eth_phy_802_3.c.obj
[1109/1180] Building C object esp-idf/esp_hid/CMakeFiles/__idf_esp_hid.dir/src/esp_hidh.c.obj
[1110/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/src/esp_lcd_panel_ops.c.obj
[1111/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/src/esp_lcd_panel_ssd1306.c.obj
[1112/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/i2c/esp_lcd_panel_io_i2c_v1.c.obj
[1113/1180] Linking C static library esp-idf/protocomm/libprotocomm.a
[1114/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/src/esp_lcd_panel_st7789.c.obj
[1115/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/i2c/esp_lcd_panel_io_i2c_v2.c.obj
[1116/1180] Building C object esp-idf/esp_local_ctrl/CMakeFiles/__idf_esp_local_ctrl.dir/src/esp_local_ctrl.c.obj
[1117/1180] Building C object esp-idf/esp_local_ctrl/CMakeFiles/__idf_esp_local_ctrl.dir/src/esp_local_ctrl_handler.c.obj
[1118/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/spi/esp_lcd_panel_io_spi.c.obj
[1119/1180] Building C object esp-idf/esp_local_ctrl/CMakeFiles/__idf_esp_local_ctrl.dir/proto-c/esp_local_ctrl.pb-c.c.obj
[1120/1180] Building C object esp-idf/esp_local_ctrl/CMakeFiles/__idf_esp_local_ctrl.dir/src/esp_local_ctrl_transport_httpd.c.obj
[1121/1180] Building C object esp-idf/mqtt/CMakeFiles/__idf_mqtt.dir/esp-mqtt/lib/platform_esp32_idf.c.obj
[1122/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/i80/esp_lcd_panel_io_i80.c.obj
[1123/1180] Building C object esp-idf/nvs_sec_provider/CMakeFiles/__idf_nvs_sec_provider.dir/nvs_sec_provider.c.obj
[1124/1180] Building C object esp-idf/perfmon/CMakeFiles/__idf_perfmon.dir/xtensa_perfmon_access.c.obj
[1125/1180] Building C object esp-idf/mqtt/CMakeFiles/__idf_mqtt.dir/esp-mqtt/lib/mqtt_outbox.c.obj
[1126/1180] Building C object esp-idf/perfmon/CMakeFiles/__idf_perfmon.dir/xtensa_perfmon_masks.c.obj
[1127/1180] Building C object esp-idf/perfmon/CMakeFiles/__idf_perfmon.dir/xtensa_perfmon_apis.c.obj
[1128/1180] Building C object esp-idf/mqtt/CMakeFiles/__idf_mqtt.dir/esp-mqtt/lib/mqtt_msg.c.obj
[1129/1180] Building C object esp-idf/rt/CMakeFiles/__idf_rt.dir/FreeRTOS_POSIX_utils.c.obj
[1130/1180] Building C object esp-idf/spiffs/CMakeFiles/__idf_spiffs.dir/spiffs_api.c.obj
[1131/1180] Building C object esp-idf/spiffs/CMakeFiles/__idf_spiffs.dir/spiffs/src/spiffs_cache.c.obj
[1132/1180] Building C object esp-idf/rt/CMakeFiles/__idf_rt.dir/FreeRTOS_POSIX_mqueue.c.obj
[1133/1180] Building C object esp-idf/esp_lcd/CMakeFiles/__idf_esp_lcd.dir/rgb/esp_lcd_panel_rgb.c.obj
[1134/1180] Building C object esp-idf/spiffs/CMakeFiles/__idf_spiffs.dir/spiffs/src/spiffs_gc.c.obj
[1135/1180] Building C object esp-idf/mqtt/CMakeFiles/__idf_mqtt.dir/esp-mqtt/mqtt_client.c.obj
[1136/1180] Building C object esp-idf/spiffs/CMakeFiles/__idf_spiffs.dir/spiffs/src/spiffs_check.c.obj
[1137/1180] Building C object esp-idf/spiffs/CMakeFiles/__idf_spiffs.dir/spiffs/src/spiffs_hydrogen.c.obj
[1138/1180] Building C object esp-idf/touch_element/CMakeFiles/__idf_touch_element.dir/touch_button.c.obj
[1139/1180] Building C object esp-idf/touch_element/CMakeFiles/__idf_touch_element.dir/touch_element.c.obj
[1140/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/src/wifi_config.c.obj
[1141/1180] Building C object esp-idf/spiffs/CMakeFiles/__idf_spiffs.dir/esp_spiffs.c.obj
[1142/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/src/wifi_ctrl.c.obj
[1143/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/src/wifi_scan.c.obj
[1144/1180] Building C object esp-idf/touch_element/CMakeFiles/__idf_touch_element.dir/touch_matrix.c.obj
[1145/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/src/handlers.c.obj
[1146/1180] Building C object esp-idf/touch_element/CMakeFiles/__idf_touch_element.dir/touch_slider.c.obj
[1147/1180] Building C object esp-idf/spiffs/CMakeFiles/__idf_spiffs.dir/spiffs/src/spiffs_nucleus.c.obj
[1148/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/src/scheme_console.c.obj
[1149/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/proto-c/wifi_scan.pb-c.c.obj
[1150/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/proto-c/wifi_constants.pb-c.c.obj
[1151/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/proto-c/wifi_config.pb-c.c.obj
[1152/1180] Linking C static library esp-idf/app_core/libapp_core.a
[1153/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/proto-c/wifi_ctrl.pb-c.c.obj
[1154/1180] Linking C static library esp-idf/app_trace/libapp_trace.a
[1155/1180] Linking C static library esp-idf/cmock/libcmock.a
[1156/1180] Linking C static library esp-idf/esp_driver_cam/libesp_driver_cam.a
[1157/1180] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/app_main.c.obj
[1158/1180] Linking C static library esp-idf/esp_driver_touch_sens/libesp_driver_touch_sens.a
[1159/1180] Linking C static library esp-idf/esp_eth/libesp_eth.a
[1160/1180] Linking C static library esp-idf/esp_hid/libesp_hid.a
[1161/1180] Linking C static library esp-idf/mqtt/libmqtt.a
[1162/1180] Linking C static library esp-idf/esp_lcd/libesp_lcd.a
[1163/1180] Linking C static library esp-idf/esp_local_ctrl/libesp_local_ctrl.a
[1164/1180] Linking C static library esp-idf/perfmon/libperfmon.a
[1165/1180] Linking C static library esp-idf/nvs_sec_provider/libnvs_sec_provider.a
[1166/1180] Linking C static library esp-idf/rt/librt.a
[1167/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/src/scheme_softap.c.obj
[1168/1180] Linking C static library esp-idf/spiffs/libspiffs.a
[1169/1180] Linking C static library esp-idf/main/libmain.a
[1170/1180] Linking C static library esp-idf/touch_element/libtouch_element.a
[1171/1180] Building C object esp-idf/wifi_provisioning/CMakeFiles/__idf_wifi_provisioning.dir/src/manager.c.obj
[1172/1180] Linking C static library esp-idf/wifi_provisioning/libwifi_provisioning.a
[1173/1180] Performing build step for 'bootloader'
[1/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/noos/util.c.obj
[2/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/noos/log_lock.c.obj
[3/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/log_timestamp_common.c.obj
[4/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/util.c.obj
[5/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/noos/log_timestamp.c.obj
[6/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/log_print.c.obj
[7/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/log_format_text.c.obj
[8/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj
[9/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/buffer/log_buffers.c.obj
[10/123] Building C object esp-idf/log/CMakeFiles/__idf_log.dir/src/log.c.obj
[11/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_sys.c.obj
[12/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_gpio.c.obj
[13/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_efuse.c.obj
[14/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_uart.c.obj
[15/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_spiflash.c.obj
[16/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_systimer.c.obj
[17/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_wdt.c.obj
[18/123] Building ASM object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_cache_writeback_esp32s3.S.obj
[19/123] Linking C static library esp-idf/log/liblog.a
[20/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_print.c.obj
[21/123] Building C object esp-idf/esp_common/CMakeFiles/__idf_esp_common.dir/src/esp_err_to_name.c.obj
[22/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/esp_memory_utils.c.obj
[23/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/cpu_region_protect.c.obj
[24/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/cpu.c.obj
[25/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_init.c.obj
[26/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_clk.c.obj
[27/123] Building ASM object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_longjmp.S.obj
[28/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/esp_cpu_intr.c.obj
[29/123] Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_cache_esp32s2_esp32s3.c.obj
[30/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_clk_init.c.obj
[31/123] Linking C static library esp-idf/esp_rom/libesp_rom.a
[32/123] Linking C static library esp-idf/esp_common/libesp_common.a
[33/123] Building C object esp-idf/esp_system/CMakeFiles/__idf_esp_system.dir/esp_err.c.obj
[34/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/chip_info.c.obj
[35/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_sleep.c.obj
[36/123] Building C object esp-idf/esp_hw_support/CMakeFiles/__idf_esp_hw_support.dir/port/esp32s3/rtc_time.c.obj
[37/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/esp32s3/esp_efuse_table.c.obj
[38/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/esp32s3/esp_efuse_fields.c.obj
[39/123] Linking C static library esp-idf/esp_hw_support/libesp_hw_support.a
[40/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/esp32s3/esp_efuse_rtc_calib.c.obj
[41/123] Linking C static library esp-idf/esp_system/libesp_system.a
[42/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/esp32s3/esp_efuse_utility.c.obj
[43/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/src/esp_efuse_fields.c.obj
[44/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/src/esp_efuse_api.c.obj
[45/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_common.c.obj
[46/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_common_loader.c.obj
[47/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_mem.c.obj
[48/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_efuse.c.obj
[49/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_clock_init.c.obj
[50/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_random.c.obj
[51/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/src/esp_efuse_utility.c.obj
[52/123] Building C object esp-idf/efuse/CMakeFiles/__idf_efuse.dir/src/efuse_controller/keys/with_key_purposes/esp_efuse_api_key.c.obj
[53/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/flash_encrypt.c.obj
[54/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/secure_boot.c.obj
[55/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_random_esp32s3.c.obj
[56/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/flash_partitions.c.obj
[57/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/bootloader_flash/src/flash_qio_mode.c.obj
[58/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/bootloader_flash/src/bootloader_flash_config_esp32s3.c.obj
[59/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/bootloader_flash/src/bootloader_flash.c.obj
[60/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_clock_loader.c.obj
[61/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_init.c.obj
[62/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/esp32s3/bootloader_soc.c.obj
[63/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_console.c.obj
[64/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_console_loader.c.obj
[65/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_panic.c.obj
[66/123] Linking C static library esp-idf/efuse/libefuse.a
[67/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_utility.c.obj
[68/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/esp_image_format.c.obj
[69/123] Building C object esp-idf/esp_bootloader_format/CMakeFiles/__idf_esp_bootloader_format.dir/esp_bootloader_desc.c.obj
[70/123] Building C object esp-idf/spi_flash/CMakeFiles/__idf_spi_flash.dir/spi_flash_wrap.c.obj
[71/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/mpu_hal.c.obj
[72/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/efuse_hal.c.obj
[73/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/esp32s3/bootloader_esp32s3.c.obj
[74/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/hal_utils.c.obj
[75/123] Building C object esp-idf/bootloader_support/CMakeFiles/__idf_bootloader_support.dir/src/bootloader_sha.c.obj
[76/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/lldesc.c.obj
[77/123] Linking C static library esp-idf/bootloader_support/libbootloader_support.a
[78/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/esp32s3/efuse_hal.c.obj
[79/123] Linking C static library esp-idf/esp_bootloader_format/libesp_bootloader_format.a
[80/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/cache_hal.c.obj
[81/123] Linking C static library esp-idf/spi_flash/libspi_flash.a
[82/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/dport_access_common.c.obj
[83/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/interrupts.c.obj
[84/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/gpio_periph.c.obj
[85/123] Building C object esp-idf/hal/CMakeFiles/__idf_hal.dir/mmu_hal.c.obj
[86/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/adc_periph.c.obj
[87/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/uart_periph.c.obj
[88/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/gdma_periph.c.obj
[89/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/dedic_gpio_periph.c.obj
[90/123] Linking C static library esp-idf/hal/libhal.a
[91/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/ledc_periph.c.obj
[92/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/spi_periph.c.obj
[93/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/pcnt_periph.c.obj
[94/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/rmt_periph.c.obj
[95/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/i2s_periph.c.obj
[96/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/temperature_sensor_periph.c.obj
[97/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/timer_periph.c.obj
[98/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/lcd_periph.c.obj
[99/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/mpi_periph.c.obj
[100/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/touch_sensor_periph.c.obj
[101/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/twai_periph.c.obj
[102/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/wdt_periph.c.obj
[103/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/usb_dwc_periph.c.obj
[104/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/rtc_io_periph.c.obj
[105/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/cam_periph.c.obj
[106/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/power_supply_periph.c.obj
[107/123] Building C object esp-idf/xtensa/CMakeFiles/__idf_xtensa.dir/eri.c.obj
[108/123] Building C object esp-idf/xtensa/CMakeFiles/__idf_xtensa.dir/xt_trax.c.obj
[109/123] Generating project_elf_src_esp32s3.c
[110/123] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/bootloader_start.c.obj
[111/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/i2c_periph.c.obj
[112/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/sdmmc_periph.c.obj
[113/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/sdm_periph.c.obj
[114/123] Building C object esp-idf/soc/CMakeFiles/__idf_soc.dir/esp32s3/mcpwm_periph.c.obj
[115/123] Building C object CMakeFiles/bootloader.elf.dir/project_elf_src_esp32s3.c.obj
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
[123/123] cd /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/bootloader/esp-idf/esptool_py && /home/runner/.espressif/python_env/idf5.5_py3.12_env/bin/python /home/runner/work/_temp/esp-idf/components/partition_table/check_sizes.py --offset 0x8000 bootloader 0x0 /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/bootloader/bootloader.bin
Bootloader binary size 0x5160 bytes. 0x2ea0 bytes (36%) free.
[1174/1180] No install step for 'bootloader'
[1175/1180] Completed 'bootloader'
[1176/1180] Generating esp-idf/esp_system/ld/sections.ld
[1177/1180] Building C object CMakeFiles/esp32_macro_keyboard.elf.dir/project_elf_src_esp32s3.c.obj
[1178/1180] Linking CXX executable esp32_macro_keyboard.elf
[1179/1180] Generating binary image from built executable
esptool.py v4.12.0
Creating esp32s3 image...
Merged 2 ELF sections
Successfully created esp32s3 image.
Generated /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/esp32_macro_keyboard.bin
[1180/1180] cd /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/esp-idf/esptool_py && /home/runner/.espressif/python_env/idf5.5_py3.12_env/bin/python /home/runner/work/_temp/esp-idf/components/partition_table/check_sizes.py --offset 0x8000 partition --type app /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/partition_table/partition-table.bin /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/esp32_macro_keyboard.bin
esp32_macro_keyboard.bin binary size 0xe5370 bytes. Smallest app partition is 0x280000 bytes. 0x19ac90 bytes (64%) free.

Project build complete. To flash, run:
 idf.py flash
or
 idf.py -p PORT flash
or
 python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0xf000 build/ota_data_initial.bin 0x20000 build/esp32_macro_keyboard.bin
or from the "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build" directory
 python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash "@flash_args"

```
