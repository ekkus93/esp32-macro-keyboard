int main(void)
{
    test_bounded_body_reads();
    test_body_timeout_and_io_failures();
    test_mutation_authorization();
    test_json_envelopes();
    test_static_file_selection();
    test_static_streaming();
    test_server_lifecycle();
    puts("web server adapter tests passed");
    return 0;
}
