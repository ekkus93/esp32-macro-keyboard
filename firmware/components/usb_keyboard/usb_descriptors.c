#include "usb_descriptors.h"

#define USB_VID 0x303aU
#define USB_PID 0x4001U
#define USB_BCD 0x0100U
#define INTERFACE_HID 0U
#define INTERFACE_TOTAL 1U
#define ENDPOINT_HID 0x81U
#define CONFIGURATION_TOTAL_LENGTH (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static const tusb_desc_device_t device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200U,
    .bDeviceClass = 0x00U,
    .bDeviceSubClass = 0x00U,
    .bDeviceProtocol = 0x00U,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = USB_BCD,
    .iManufacturer = 0x01U,
    .iProduct = 0x02U,
    .iSerialNumber = 0x03U,
    .bNumConfigurations = 0x01U,
};

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, INTERFACE_TOTAL, 0, CONFIGURATION_TOTAL_LENGTH,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(INTERFACE_HID, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_descriptor), ENDPOINT_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 10),
};

static const char *string_descriptors[] = {
    (const char[]){0x09, 0x04},
    "ESP32 Macro Keyboard Project",
    "ESP32 Macro Keyboard",
    "ESP32S3-MACRO-01",
};

const tusb_desc_device_t *usb_descriptors_device(void)
{
    return &device_descriptor;
}

const uint8_t *usb_descriptors_configuration(void)
{
    return configuration_descriptor;
}

const char **usb_descriptors_strings(void)
{
    return string_descriptors;
}

size_t usb_descriptors_string_count(void)
{
    return sizeof(string_descriptors) / sizeof(string_descriptors[0]);
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t requested_length)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)requested_length;
    return 0U;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t buffer_size)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)buffer_size;
}
