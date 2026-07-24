#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stddef.h>
#include <stdint.h>

#include "tusb.h"

const tusb_desc_device_t *usb_descriptors_device(void);
const uint8_t *usb_descriptors_configuration(void);
const char **usb_descriptors_strings(void);
size_t usb_descriptors_string_count(void);

#endif
