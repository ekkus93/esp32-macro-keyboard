#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "device_settings_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *data;
    size_t length;
} app_v2_string_view_t;

typedef struct {
    bool present;
    app_v2_string_view_t value;
} app_v2_optional_string_view_t;

typedef enum {
    APP_V2_USB_UNINITIALIZED = 0,
    APP_V2_USB_DISCONNECTED,
    APP_V2_USB_ENUMERATING,
    APP_V2_USB_READY,
    APP_V2_USB_SUSPENDED,
    APP_V2_USB_ERROR,
} app_v2_usb_state_t;

typedef enum {
    APP_V2_SEND_AWAITING_CONFIRMATION = 0,
    APP_V2_SEND_RUNNING,
    APP_V2_SEND_COMPLETED,
    APP_V2_SEND_CANCELLED,
    APP_V2_SEND_FAILED,
    APP_V2_SEND_TIMED_OUT,
} app_v2_send_state_t;

typedef enum {
    APP_V2_HEALTH_HEALTHY = 0,
    APP_V2_HEALTH_DEGRADED,
    APP_V2_HEALTH_UNAVAILABLE,
    APP_V2_HEALTH_RECOVERING,
    APP_V2_HEALTH_FAILED,
    APP_V2_HEALTH_UNKNOWN,
} app_v2_health_state_t;

typedef struct {
    app_v2_string_view_t code;
    app_v2_string_view_t message;
    app_v2_optional_string_view_t field;
    bool has_parser_location;
    size_t byte_offset;
    size_t line;
    size_t column;
} app_v2_error_detail_t;

typedef struct {
    bool provisioned;
    app_v2_string_view_t device_name;
} app_v2_setup_state_response_t;

typedef struct {
    app_v2_string_view_t setup_code;
    app_v2_string_view_t device_name;
    app_v2_string_view_t ap_ssid;
    app_v2_string_view_t ap_passphrase;
    app_v2_string_view_t admin_password;
    bool require_serial_confirmation;
} app_v2_setup_request_t;

typedef struct {
    app_v2_string_view_t admin_password;
} app_v2_login_request_t;

typedef struct {
    bool authenticated;
    uint32_t idle_expires_in_seconds;
    uint32_t absolute_expires_in_seconds;
} app_v2_session_response_t;

typedef struct {
    bool accepted;
    bool connection_will_close;
    bool reprovisioning_required;
} app_v2_action_accepted_t;

typedef struct {
    app_v2_action_accepted_t action;
    bool restart_required;
} app_v2_setup_accepted_t;

typedef struct {
    app_v2_action_accepted_t action;
    bool repository_blobs_preserved;
} app_v2_reset_accepted_t;

typedef struct {
    app_v2_string_view_t state;
} app_v2_usb_status_t;

typedef struct {
    app_v2_string_view_t state;
    app_v2_string_view_t ssid;
    uint32_t client_count;
} app_v2_access_point_status_t;

typedef struct {
    bool configured;
    app_v2_string_view_t state;
    app_v2_optional_string_view_t ssid;
    app_v2_optional_string_view_t ipv4;
} app_v2_station_status_t;

typedef struct {
    app_v2_string_view_t state;
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t remaining_bytes;
    uint32_t blob_count;
} app_v2_storage_status_t;

typedef struct {
    bool present;
    app_v2_send_state_t state;
} app_v2_send_summary_t;

typedef struct {
    bool provisioned;
    app_v2_string_view_t device_name;
    app_v2_string_view_t firmware_version;
    app_v2_string_view_t build_id;
    uint64_t uptime_ms;
    app_v2_usb_status_t usb;
    app_v2_access_point_status_t access_point;
    app_v2_station_status_t station;
    app_v2_storage_status_t storage;
    app_v2_send_summary_t send;
} app_v2_status_response_t;

typedef struct {
    uint32_t package_name_max_bytes;
    uint32_t macro_name_max_bytes;
    uint32_t macro_source_max_bytes;
    uint32_t compiled_actions_max;
    uint32_t delay_directive_max_ms;
    uint32_t key_press_max_ms;
    uint32_t inter_key_max_ms;
    uint32_t estimated_duration_max_ms;
    uint32_t executor_absolute_deadline_ms;
    uint32_t json_body_max_bytes;
    uint32_t blob_max_bytes;
    uint32_t admin_password_min_bytes;
    uint32_t admin_password_max_bytes;
    uint32_t snapshot_retention_target_max;
} app_v2_limits_response_t;

typedef struct {
    uint64_t id;
    uint32_t size_bytes;
} app_v2_blob_summary_t;

typedef struct {
    const app_v2_blob_summary_t *blobs;
    size_t blob_count;
    uint64_t used_bytes;
    uint64_t remaining_bytes;
} app_v2_blob_list_response_t;

typedef struct {
    app_v2_string_view_t device_name;
    bool require_serial_confirmation;
    app_v2_send_mode_t send_mode;
    uint8_t snapshot_retention_target;
    bool show_macro_source_previews;
    app_v2_optional_string_view_t last_selected_package_id;
    app_v2_string_view_t ap_ssid;
    bool station_configured;
    app_v2_optional_string_view_t station_ssid;
} app_v2_settings_response_t;

typedef struct {
    app_v2_string_view_t ssid;
    app_v2_string_view_t passphrase;
} app_v2_network_credentials_t;

typedef struct {
    bool has_device_name;
    app_v2_string_view_t device_name;
    bool has_require_serial_confirmation;
    bool require_serial_confirmation;
    bool has_send_mode;
    app_v2_send_mode_t send_mode;
    bool has_snapshot_retention_target;
    uint8_t snapshot_retention_target;
    bool has_show_macro_source_previews;
    bool show_macro_source_previews;
    bool has_last_selected_package_id;
    app_v2_optional_string_view_t last_selected_package_id;
    bool has_access_point;
    app_v2_network_credentials_t access_point;
    bool has_station;
    bool remove_station;
    app_v2_network_credentials_t station;
} app_v2_settings_update_request_t;

typedef struct {
    app_v2_settings_response_t settings;
    bool restart_required;
    bool reconnect_required;
} app_v2_settings_updated_response_t;

typedef struct {
    app_v2_string_view_t current_password;
    app_v2_string_view_t new_password;
} app_v2_password_change_request_t;

typedef struct {
    app_v2_string_view_t source;
    uint32_t key_press_ms;
    uint32_t inter_key_ms;
} app_v2_send_request_t;

typedef struct {
    app_v2_string_view_t id;
    app_v2_send_state_t state;
    uint32_t action_count;
    uint32_t estimated_duration_ms;
} app_v2_send_accepted_response_t;

typedef struct {
    app_v2_string_view_t id;
    app_v2_send_state_t state;
    uint32_t action_index;
    uint32_t action_count;
    uint32_t estimated_duration_ms;
    bool cancellation_requested;
    app_v2_string_view_t error;
    app_v2_string_view_t release_error;
} app_v2_send_status_response_t;

typedef struct {
    app_v2_string_view_t confirmation;
} app_v2_reset_settings_request_t;

typedef struct {
    app_v2_string_view_t admin_password;
    app_v2_string_view_t confirmation;
} app_v2_factory_reset_request_t;

typedef struct {
    app_v2_string_view_t name;
    app_v2_health_state_t state;
} app_v2_subsystem_health_t;

typedef struct {
    app_v2_string_view_t firmware_version;
    app_v2_string_view_t build_id;
    app_v2_string_view_t reset_reason;
    uint64_t uptime_ms;
    uint64_t free_heap_bytes;
    uint64_t minimum_free_heap_bytes;
    uint64_t largest_free_block_bytes;
    app_v2_usb_state_t usb_state;
    app_v2_string_view_t access_point_state;
    app_v2_string_view_t station_state;
    app_v2_string_view_t storage_state;
    uint64_t webfs_total_bytes;
    uint64_t webfs_used_bytes;
    uint64_t userdata_total_bytes;
    uint64_t userdata_used_bytes;
    uint32_t blob_count;
    const app_v2_string_view_t *invalid_names;
    size_t invalid_name_count;
    const app_v2_string_view_t *temporary_files;
    size_t temporary_file_count;
    app_v2_send_summary_t send;
    const app_v2_subsystem_health_t *subsystems;
    size_t subsystem_count;
} app_v2_diagnostics_response_t;

#ifdef __cplusplus
}
#endif