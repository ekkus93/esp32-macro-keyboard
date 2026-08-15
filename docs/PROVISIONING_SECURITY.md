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

`scripts/check-credential-logging.sh` rejects plaintext credentials from ordinary
firmware output and permits exactly one narrowly allowlisted disclosure: the
user-invoked `setup-code` command in the physical UART console.
`scripts/check-setup-route-isolation.sh` requires the unprovisioned server to
expose only setup routes and static assets.

These gates validate source and configuration intent. They do not prove that a physical
device has the required eFuse key.

## Bootstrap credential derivation

A production device has no display. Its protected first-run **SoftAP** credentials
are delivered on a manufacturing label or QR code. The one-time setup code submitted over HTTP
is deliberately *not* a manufacturing-label secret: firmware generates a fresh
eight-digit decimal code on every unprovisioned boot and reveals it only when a
person with physical UART access explicitly runs `setup-code`.

The device identifier is the six-byte ESP32-S3 SoftAP MAC address. It is rendered
as 12 uppercase hexadecimal characters. The bootstrap SSID is:

```text
ESP32-Macro-<last-six-device-id-hex-characters>
```

The bootstrap AP passphrase uses HMAC-SHA256 with `HMAC_KEY0`. The
domain-separated message is exactly:

```text
AP passphrase: "macro-setup-ap-v1\0\0\0" || softap_mac_bytes
```

The first 12 digest bytes are encoded as 24 uppercase hexadecimal characters.
Firmware reads the SoftAP MAC, asks the ESP32-S3 HMAC peripheral to calculate the
digest, and securely clears the message and digest buffers. Software cannot read
the HMAC eFuse key back from the device.

The setup code follows a separate lifecycle. `app_v2_setup_session_generate()`
uses the hardware random source to generate exactly eight decimal digits for the
current unprovisioned boot. The value is never returned over HTTP and is never
emitted automatically to logs. The explicit UART `setup-code` command copies the
current code while holding its disclosure mutex, formats one bounded response,
and writes that response directly to UART0 rather than `stdout`; this bypass is
required because ESP-IDF may mirror normal `stdout`/`stderr` to the secondary
USB-Serial-JTAG console. The command then wipes both command-local buffers. Successful setup consumes the setup session and
clears the console copy before the accepted response/restart; startup failure and
reboot clear it as well. A later factory reset therefore creates a *new* setup
code on the next unprovisioned boot while retaining the deterministic
manufacturing-label SoftAP bootstrap path.

## Manufacturing procedure

The following procedure is destructive because eFuse writes are irreversible. Use a
known target device, verify its identity and existing eFuse state, and retain the
command output as release evidence.

1. Generate a unique 256-bit HMAC key using the ESP-IDF NVS partition generator.
2. Determine and record the device SoftAP MAC address.
3. Generate the device label while the key file is still available offline. The
   label contains the device ID, bootstrap SSID, and bootstrap AP passphrase --
   not a setup code.
4. Inspect `BLOCK_KEY0` and confirm that it is unused.
5. Burn the key into `BLOCK_KEY0` with purpose `HMAC_UP`.
6. Read the eFuse summary and verify the key purpose and read protection.
7. Flash the firmware and confirm that encrypted `nvs_flash_init()` succeeds.
8. Connect using the printed bootstrap AP credentials (or use the trusted UART
   `wifi-connect` command on a development bench). A successful UART join is
   persisted in the authoritative v2 settings record and is preserved by the
   subsequent setup transaction; the helper treats persistence failure as a
   hard failure rather than accepting a temporary association.
9. On the physical UART console, explicitly run `setup-code` and use the returned
   eight-digit value for the current setup submission. Do not copy it into build
   logs or manufacturing artifacts.
10. Verify encrypted provisioning commit and automatic restart into normal mode.
11. Power-cycle the device and verify that provisioning state can be read back.
12. Confirm that setup routes and `setup-code` disclosure are unavailable after
    provisioning.
13. Run the negative test on an unkeyed or wrongly keyed fixture and confirm
    startup fails closed instead of creating plaintext NVS state.

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

The label generator emits one JSON object containing `device_id`, `ap_ssid`, and
`ap_passphrase`. Transfer those values into the controlled label or QR printing
system without placing them in ordinary build logs. It intentionally emits no
setup code; that code exists only for the current unprovisioned boot.

Do not commit, archive with normal build artifacts, or transmit `hmac_key.bin`,
`nvs_encr_key.bin`, generated label JSON, AP passphrases, or setup codes through an
unapproved channel. Securely destroy retained key files according to the manufacturing
key-management policy after the required recovery escrow is complete.

## First-run network and HTTP isolation

An unprovisioned device initializes only the resources required for setup:

- encrypted NVS and the authoritative v2 device-settings record;
- the read-only web-assets and user-data mounts;
- authentication primitives needed to create the password verifier;
- the protected bootstrap SoftAP;
- the setup-only HTTP server.

It does not initialize the blob repository, USB HID, or macro executor. The
setup server registers only:

```text
GET  /api/v1/setup
POST /api/v1/setup
GET  /*
```

`GET /api/v1/setup` returns the minimal unprovisioned state (`provisioned:
false` and the current device name) and `404` once provisioning is complete.
`POST /api/v1/setup` accepts the complete setup request in one call - it does
not use separate credentials/complete/restart steps (`docs/SPEC_V2.md` §13.4;
`firmware/components/web_server/web_server_lifecycle.c`).

Setup credential submission requires exact bounded JSON and the current setup
code. `requireSerialConfirmation` in that request sets the preference used by
subsequent confirmation-gated operations; setup submission itself does not add a
second confirmation step. Under the current v2 development-appliance profile there
is no separate CSRF token and no `Host`/`Origin` check; a product distributed to
third parties must revisit DNS-rebinding protection before release. The
administrator plaintext password is converted to a PBKDF2 record before persistence
and all request, JSON, bootstrap, and temporary configuration buffers are cleared
after use.

Normal mode uses the persisted AP credentials and password record and does not register
any setup route.

## Validation status

The software configuration, derivation vectors, route-isolation policy, repository
fault injection, setup state machine, and secure-buffer-clearing behavior are
host-testable. Physical confidentiality is **not yet claimed**. Phase 20 must record real
ESP32-S3 evidence for:

- successful HMAC eFuse provisioning;
- firmware/label-generator AP-credential derivation agreement on a real device;
- protected bootstrap AP association;
- explicit physical-UART retrieval of the current one-time setup code;
- setup completion and immediate retirement of that UART-disclosed code;
- encrypted NVS initialization and readback;
- failure on a device with the required key missing or assigned the wrong purpose;
- power-cycle persistence;
- normal-mode rejection of setup routes;
- confirmation that no plaintext credential values appear in raw NVS flash or ordinary
  UART logs.

Authoritative ESP-IDF references:

- ESP-IDF v5.5, ESP32-S3, *Security Features Enablement Workflows*.
- ESP-IDF v5.5.5, ESP32-S3, *Hash-Based Message Authentication Code (HMAC)*.
