from pathlib import Path

path = Path("tests/host/test_web_api_repository_handlers.c")
text = path.read_text()
old = '''    static const web_api_route_t unavailable_routes[] = {
        WEB_API_ROUTE_SET_IMPORT,
    };
    for (size_t index = 0U; index < sizeof(unavailable_routes) / sizeof(unavailable_routes[0]);
         ++index) {
        response = invoke(web_api_handle_sets, unavailable_routes[index], WEB_API_METHOD_POST, NULL,
                          SET_ID, NULL, NULL);
        expect_status(&response, 503U, "requires the Phase 18");
    }'''
new = '''    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET_IMPORT, WEB_API_METHOD_POST, "{}",
                      NULL, NULL, NULL);
    expect_status(&response, 422U, "could not replace set");'''
if text.count(old) != 1:
    raise SystemExit("set import boundary assertion anchor changed")
path.write_text(text.replace(old, new, 1))
