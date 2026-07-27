# Provisioning Security

## Selected NVS encryption scheme

Production firmware uses the ESP32-S3 HMAC peripheral-based NVS encryption scheme:

```text
CONFIG_NVS_ENCRYPTION=y
CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC=y
CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID=0
```

The manufacturing process reserves `HMAC_KEY0` (`BLOCK_KEY0`) for NVS key derivation.
The eFuse key must have purpose `HMAC_UP`. The NVS XTS keys are derived at runtime;
they are not stored in SPI flash.

FIX1 also requires a 4 KiB `nvs_keys` partition with the `encrypted` flag. It remains
in the partition table as a reserved, specification-required partition. The selected
HMAC scheme does not read its key material from that partition.

## Production gate

`scripts/check-production-config.sh` fails unless the committed production defaults:

- enable NVS encryption;
- select only the HMAC protection scheme;
- reserve HMAC key ID 0;
- reject the flash-encryption-backed and unprotected alternatives.

The gate validates configuration intent. It does not prove that a physical device has
the required eFuse key.

## Manufacturing procedure

The following procedure is destructive because eFuse writes are irreversible. Use a
known target device, verify its identity and existing eFuse state, and retain the
command output as release evidence.

1. Generate a unique 256-bit HMAC key using the ESP-IDF NVS partition generator.
2. Inspect `BLOCK_KEY0` and confirm that it is unused.
3. Burn the key into `BLOCK_KEY0` with purpose `HMAC_UP`.
4. Read the eFuse summary and verify the key purpose and read protection.
5. Flash the firmware and confirm that encrypted `nvs_flash_init()` succeeds.
6. Power-cycle the device and verify that provisioning state can be read back.
7. Attempt the documented negative test on an unprovisioned fixture and confirm that
   startup fails closed instead of creating plaintext NVS state.

Representative ESP-IDF commands are:

```bash
python3 "$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py" \
  generate-key --key_protect_hmac --kp_hmac_keygen \
  --kp_hmac_keyfile hmac_key.bin --keyfile nvs_encr_key.bin
espefuse.py --port "$PORT" burn_key BLOCK_KEY0 hmac_key.bin HMAC_UP
espefuse.py --port "$PORT" summary
```

Do not commit, archive with normal build artifacts, or transmit `hmac_key.bin` or
`nvs_encr_key.bin` through an unapproved channel.

## Validation status

The software configuration and fail-closed policy are host-testable. Physical
confidentiality is **not yet claimed**. Phase 20 must record real ESP32-S3 evidence for:

- successful HMAC eFuse provisioning;
- encrypted NVS initialization and readback;
- failure on a device with the required key missing or assigned the wrong purpose;
- power-cycle persistence;
- confirmation that no plaintext credential values appear in raw NVS flash.

Authoritative ESP-IDF references:

- ESP-IDF v5.5, ESP32-S3, *Security Features Enablement Workflows*.
- ESP-IDF v5.5.5, ESP32-S3, *Hash-Based Message Authentication Code (HMAC)*.
