int main(void) {
    test_bounded_body_reads();
    test_body_timeout_and_io_failures();
    test_mutation_authorization();
    test_json_envelopes();
    test_diagnostics_exact_allowed_fields();
    test_diagnostics_secret_sentinels_absent();
    test_diagnostics_output_passes_secret_sentinel_scanner();
    test_diagnostics_bounded_output();
    test_diagnostics_rejects_partial_invalid_name_reporting();
    test_diagnostics_failed_query_never_fabricates_data();
    test_diagnostics_no_false_healthy_state();
    test_static_file_selection();
    test_static_streaming();
    test_server_lifecycle();
    puts("web server adapter tests passed");
    return 0;
}
