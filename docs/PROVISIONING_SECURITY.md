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
- reject the flash-encryption-backed and unprotected alternatives;
- reject manufacturing or legacy development credential logging.

`scripts/check-credential-logging.sh` permits plaintext bootstrap output only in the
explicitly guarded manufacturing block. `scripts/check-setup-route-isolation.sh`
requires the unprovisioned server to expose only setup routes and static assets.

These gates validate source and configuration intent. They do not prove that a physical
device has the required eFuse key.

## Bootstrap credential derivation

A production device has no display, so its protected first-run AP credentials and setup
code are delivered on a manufacturing label or QR code. Firmware and the offline label
generator derive the values from the same device-specific inputs.

The device identifier is the six-byte ESP32-S3 SoftAP MAC address. It is rendered as 12
uppercase hexadecimal characters. The bootstrap SSID is:

```text
ESP32-Macro-<last-six-device-id-hex-characters>
```

The AP passphrase and setup code use HMAC-SHA256 with `HMAC_KEY0`. The domain-separated
messages are exactly:

```text
AP passphrase: "macro-setup-ap-v1\0\0\0" || softap_mac_bytes
Setup code:    "macro-setup-code-v1\0"    || softap_mac_bytes
```

The first 12 digest bytes are encoded as 24 uppercase hexadecimal characters. Domain
separation prevents the AP passphrase and setup code from being interchangeable even
though both use the same protected eFuse key.

The firmware reads the SoftAP MAC, asks the ESP32-S3 HMAC peripheral to calculate each
digest, and securely clears the message and digest buffers. Software cannot read the
HMAC eFuse key back from the device.

The setup code is accepted only while the persistent configuration is unprovisioned.
Normal mode does not register setup routes. After a successful commit, the setup core
clears its RAM copy and requires an explicit restart. A later factory reset intentionally
returns the device to the same label-based bootstrap credentials; therefore “one-time”
means one provisioning cycle, not a permanently rotating printed secret.

## Manufacturing procedure

The following procedure is destructive because eFuse writes are irreversible. Use a
known target device, verify its identity and existing eFuse state, and retain the
command output as release evidence.

1. Generate a unique 256-bit HMAC key using the ESP-IDF NVS partition generator.
2. Determine and record the device SoftAP MAC address.
3. Generate the device label while the key file is still available offline.
4. Inspect `BLOCK_KEY0` and confirm that it is unused.
5. Burn the key into `BLOCK_KEY0` with purpose `HMAC_UP`.
6. Read the eFuse summary and verify the key purpose and read protection.
7. Flash the firmware and confirm that encrypted `nvs_flash_init()` succeeds.
8. Connect using the printed bootstrap AP credentials.
9. Submit the printed setup code and require the physical confirmation button unless
   the separately gated manufacturing mode is active.
10. Verify encrypted provisioning commit, readback, and restart into normal mode.
11. Power-cycle the device and verify that provisioning state can be read back.
12. Confirm that setup routes are unavailable after provisioning.
13. Run the negative test on an unkeyed or wrongly keyed fixture and confirm startup
    fails closed instead of creating plaintext NVS state.

Representative ESP-IDF and label-generation commands are:

```bash
python3 "$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py" \
  generate-key --key_protect_hmac --kp_hmac_keygen \
  --kp_hmac_keyfile hmac_key.bin --keyfile nvs_encr_key.bin

python3 scripts/generate-setup-label.py \
  hmac_key.bin 10:20:30:A0:B0:C0

espefuse.py --port "$PORT" burn_key BLOCK_KEY0 hmac_key.bin HMAC_UP
espefuse.py --port "$PORT" summary
```

The label generator emits one JSON object containing `device_id`, `ap_ssid`,
`ap_passphrase`, and `setup_code`. Transfer those values into the controlled label or QR
printing system without placing them in ordinary build logs.

Do not commit, archive with normal build artifacts, or transmit `hmac_key.bin`,
`nvs_encr_key.bin`, generated label JSON, AP passphrases, or setup codes through an
unapproved channel. Securely destroy retained key files according to the manufacturing
key-management policy after the required recovery escrow is complete.

## First-run network and HTTP isolation

An unprovisioned device initializes only the resources required for setup:

- encrypted NVS and the provisioning repository;
- the read-only web-assets and user-data mounts;
- authentication primitives needed to create the password verifier;
- device controls for physical confirmation;
- the protected bootstrap SoftAP;
- the setup-only HTTP server.

It does not initialize the storage object repository, USB HID, or macro executor. The
setup server registers only:

```text
GET  /api/v1/setup-state
POST /api/v1/setup/credentials
POST /api/v1/setup/complete
POST /api/v1/setup/restart
GET  /*
```

Setup credential submission requires exact bounded JSON, a matching Host and Origin,
the setup code, and physical confirmation unless manufacturing bypass is explicitly
compiled. The administrator plaintext password is converted to a PBKDF2 record before
persistence and all request, JSON, bootstrap, and temporary configuration buffers are
cleared after use.

Normal mode uses the persisted AP credentials and password record and does not register
any setup route.

## Validation status

The software configuration, derivation vectors, route-isolation policy, repository
fault injection, setup state machine, and secure-buffer-clearing behavior are
host-testable. Physical confidentiality is **not yet claimed**. Phase 20 must record real
ESP32-S3 evidence for:

- successful HMAC eFuse provisioning;
- firmware/label-generator derivation agreement on a real device;
- protected bootstrap AP association;
- physical-confirmation setup completion;
- encrypted NVS initialization and readback;
- failure on a device with the required key missing or assigned the wrong purpose;
- power-cycle persistence;
- normal-mode rejection of setup routes;
- confirmation that no plaintext credential values appear in raw NVS flash or ordinary
  UART logs.

Authoritative ESP-IDF references:

- ESP-IDF v5.5, ESP32-S3, *Security Features Enablement Workflows*.
- ESP-IDF v5.5.5, ESP32-S3, *Hash-Based Message Authentication Code (HMAC)*.
