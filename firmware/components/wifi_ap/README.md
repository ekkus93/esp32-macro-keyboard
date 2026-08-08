# wifi_ap

Protected Wi-Fi SoftAP startup, configuration, lifecycle, and explicit failure reporting. No open-AP fallback is permitted.

The access point starts first and unconditionally (`app_core`'s `adapter_wifi_start` calls
`wifi_ap_start()` before ever touching station mode); a station join failure never blocks or tears
down the AP (SPEC_V2.md §12.1).

At most one station network is remembered, in `device_settings` (`station_configured`/
`station_ssid`/`station_passphrase`). `wifi_ap_connect_station()` bounds a join attempt to
`WIFI_STATION_MAX_ATTEMPTS` (3) tries before giving up — never an unbounded retry loop — and
publishes the outcome through `wifi_ap_get_station_status()` as one of `WIFI_STATION_DISABLED`
(never attempted), `WIFI_STATION_CONNECTING`, `WIFI_STATION_CONNECTED`, or `WIFI_STATION_FAILED`.
The bounded-retry state machine itself lives in `wifi_ap_station.c`, behind an ops seam
(`wifi_ap_station_ops_t`) so it is host-tested (`tests/host/test_wifi_ap_station.c`) without
touching `esp_wifi_*`; the hardware adapter in `wifi_ap.c` supplies the single blocking connection
attempt.
