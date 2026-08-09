int main(void) {
    test_cookie_boundaries();
    test_cookie_build_session_header();
    test_static_path_boundaries();
    test_content_boundaries();
    puts("web security tests passed");
    return EXIT_SUCCESS;
}
