# V2-040 Cutover A — V2 settings boot authority

This intermediate implementation slice intentionally does not mark V2-040 complete.

It establishes the fail-closed runtime boundary required before the transactional setup submission can land:

- canonical `device_settings` is the only boot configuration authority;
- no legacy provisioning record is read during startup;
- normal-mode AP/station/password state is sourced from the validated V2 settings record;
- an unprovisioned boot generates a fresh eight-digit decimal setup code from ESP32 randomness;
- only that setup code is intentionally emitted on the trusted serial surface;
- setup mode exposes only `GET /api/v1/setup`, `POST /api/v1/setup`, and required static assets;
- `GET /api/v1/setup` returns only `provisioned:false` and `deviceName`;
- legacy normal-mode configuration mutations are explicitly unavailable rather than allowed to write state V2 startup would ignore.

`POST /api/v1/setup` remains deliberately fail-closed with `503` in this slice. The next slice replaces that temporary explicit failure with the already-reviewed V2 setup contract and transactional `device_settings_replace()` flow. No V1-to-V2 migration, conversion, alias, dual read, dual write, or fallback is introduced.
