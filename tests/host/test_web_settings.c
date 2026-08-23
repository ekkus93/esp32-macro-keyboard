#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"
#include "test_assert.h"
#include "web_settings.h"

/* The fixture defines the fakes and helpers the suites below depend on, so it
 * must be included first; the blank line keeps it in its own include block so
 * include sorting cannot reorder it after the alphabetized suites. */
#include "web_settings_test_fixture.inc"

#include "web_settings_change_password_tests.inc"
#include "web_settings_get_tests.inc"
#include "web_settings_put_tests.inc"

int main(void) {
    test_get_success();
    test_get_success_with_optional_fields_present();
    test_get_invalid_ops_rejected();
    test_get_unprovisioned_backend_record_is_internal_error();
    test_get_backend_unavailable();
    test_put_device_name_success();
    test_put_access_point_sets_restart_and_reconnect();
    test_put_last_selected_package_id_null_clears();
    test_put_station_removal();
    test_put_empty_object_rejected();
    test_put_unknown_field_rejected();
    test_put_duplicate_field_rejected();
    test_put_wrong_type_rejected();
    test_put_invalid_send_mode_rejected();
    test_put_access_point_missing_passphrase_rejected();
    test_put_station_wrong_type_rejected();
    test_put_invalid_device_name_rejected();
    test_put_snapshot_retention_out_of_range_rejected();
    test_put_settings_read_backend_unavailable();
    test_put_settings_replace_backend_unavailable();
    test_put_invalid_ops_wipes_body();
    test_put_body_without_nul_terminator_rejected();
    test_put_embedded_nul_escape_rejected();
    test_put_malformed_json_rejected();
    test_put_trailing_content_rejected();
    test_put_non_object_top_level_rejected();
    test_put_device_name_wrong_type_rejected();
    test_put_access_point_extra_credential_field_rejected();
    test_put_require_serial_confirmation_success();
    test_put_send_mode_preview_success();
    test_put_send_mode_quick_success();
    test_put_snapshot_retention_negative_rejected();
    test_put_snapshot_retention_fractional_rejected();
    test_put_last_selected_package_id_wrong_type_rejected();
    test_put_last_selected_package_id_valid_string_success();
    test_put_station_valid_object_success();
    test_put_send_mode_wrong_type_rejected();
    test_put_access_point_ssid_too_long_rejected();
    test_put_access_point_passphrase_too_short_rejected();
    test_put_station_ssid_too_long_rejected();
    test_put_station_passphrase_too_short_rejected();
    test_put_last_selected_package_id_malformed_uuid_rejected();
    test_change_password_success();
    test_change_password_incorrect_current_password();
    test_change_password_new_password_too_short_rejected();
    test_change_password_missing_field_rejected();
    test_change_password_settings_replace_backend_unavailable();
    test_change_password_transition_begin_failure_changes_nothing();
    test_change_password_invalidate_failure();
    test_change_password_invalid_ops_wipes_body();
    test_change_password_duplicate_field_rejected();
    test_change_password_unknown_field_rejected();
    test_change_password_wrong_type_field_rejected();
    test_change_password_settings_read_backend_unavailable();
    test_change_password_verify_backend_unavailable();
    test_change_password_create_backend_unavailable();
    test_change_password_prepare_candidate_failure_is_internal();
    puts("web settings tests passed");
    return EXIT_SUCCESS;
}
