typedef struct {
    const uint8_t *data;
    size_t length;
    size_t cursor;
    size_t chunk_size;
    bool read_failure;
    int send_failure_index;
    int close_result;
    size_t send_calls;
    size_t close_calls;
    size_t final_calls;
} stream_fake_t;

static int read_stream(void *context,
                       void *handle,
                       uint8_t *output,
                       size_t capacity,
                       size_t *out_count,
                       bool *out_eof)
{
    stream_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(handle != NULL);
    if (fake->read_failure) {
        return -1;
    }
    size_t count = fake->length - fake->cursor;
    if (fake->chunk_size > 0U && count > fake->chunk_size) {
        count = fake->chunk_size;
    }
    if (count > capacity) {
        count = capacity;
    }
    if (count > 0U) {
        memcpy(output, fake->data + fake->cursor, count);
    }
    fake->cursor += count;
    *out_count = count;
    *out_eof = fake->cursor == fake->length;
    return 0;
}

static int send_stream_chunk(void *context, const uint8_t *buffer, size_t length)
{
    stream_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    if (buffer == NULL && length == 0U) {
        ++fake->final_calls;
        return fake->send_failure_index == -2 ? -1 : 0;
    }
    ++fake->send_calls;
    return (int)fake->send_calls == fake->send_failure_index ? -1 : 0;
}

static int close_stream(void *context, void *handle)
{
    stream_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(handle != NULL);
    ++fake->close_calls;
    return fake->close_result;
}

static web_adapter_stream_ops_t stream_ops(stream_fake_t *fake)
{
    return (web_adapter_stream_ops_t){
        .context = fake,
        .read = read_stream,
        .send_chunk = send_stream_chunk,
        .close = close_stream,
    };
}

static void test_static_streaming(void)
{
    static const uint8_t data[] = "abcdef";
    int handle = 0;
    stream_fake_t fake = {.data = data, .length = 6U, .chunk_size = 2U};
    web_adapter_stream_ops_t ops = stream_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_adapter_stream_file(&handle, &ops));
    TEST_CHECK_EQ_U64(3U, fake.send_calls);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);
    TEST_CHECK_EQ_U64(1U, fake.final_calls);

    fake = (stream_fake_t){.data = data, .length = 6U, .send_failure_index = 1};
    ops = stream_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, web_adapter_stream_file(&handle, &ops));
    TEST_CHECK_EQ_U64(1U, fake.close_calls);
    TEST_CHECK_EQ_U64(0U, fake.final_calls);

    fake = (stream_fake_t){.data = data, .length = 6U, .read_failure = true};
    ops = stream_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, web_adapter_stream_file(&handle, &ops));
    TEST_CHECK_EQ_U64(1U, fake.close_calls);

    fake = (stream_fake_t){.data = data, .length = 6U, .close_result = -1};
    ops = stream_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, web_adapter_stream_file(&handle, &ops));

    fake = (stream_fake_t){.data = data, .length = 0U, .send_failure_index = -2};
    ops = stream_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, web_adapter_stream_file(&handle, &ops));
}

typedef struct {
    int start_result;
    size_t failed_route;
    int stop_result;
    size_t register_calls;
    size_t stop_calls;
    int handle;
} lifecycle_fake_t;

static int start_server(void *context, void **out_handle)
{
    lifecycle_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    if (fake->start_result != 0) {
        return -1;
    }
    *out_handle = &fake->handle;
    return 0;
}

static int register_route(void *context, void *handle, size_t route_index)
{
    lifecycle_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(handle == &fake->handle);
    ++fake->register_calls;
    return route_index == fake->failed_route ? -1 : 0;
}

static int stop_server(void *context, void *handle)
{
    lifecycle_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(handle == &fake->handle);
    ++fake->stop_calls;
    return fake->stop_result;
}

static web_adapter_lifecycle_ops_t lifecycle_ops(lifecycle_fake_t *fake)
{
    return (web_adapter_lifecycle_ops_t){
        .context = fake,
        .start = start_server,
        .register_route = register_route,
        .stop = stop_server,
    };
}

static void test_server_lifecycle(void)
{
    web_adapter_lifecycle_t lifecycle;
    web_adapter_lifecycle_reset(&lifecycle);
    lifecycle_fake_t fake = {.failed_route = SIZE_MAX};
    web_adapter_lifecycle_ops_t ops = lifecycle_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_lifecycle_start(&lifecycle, &ops, 3U));
    TEST_CHECK_EQ_U64(3U, lifecycle.registered_routes);
    TEST_CHECK(lifecycle.handle != NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_lifecycle_stop(&lifecycle, &ops));
    TEST_CHECK(lifecycle.handle == NULL);

    fake = (lifecycle_fake_t){.failed_route = 1U};
    ops = lifecycle_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         web_adapter_lifecycle_start(&lifecycle, &ops, 3U));
    TEST_CHECK(lifecycle.handle == NULL);
    TEST_CHECK_EQ_U64(1U, fake.stop_calls);

    fake = (lifecycle_fake_t){.failed_route = 1U, .stop_result = -1};
    ops = lifecycle_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         web_adapter_lifecycle_start(&lifecycle, &ops, 3U));
    TEST_CHECK(lifecycle.handle != NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, lifecycle.cleanup_error);
    fake.stop_result = 0;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_lifecycle_stop(&lifecycle, &ops));
    TEST_CHECK(lifecycle.handle == NULL);
}
