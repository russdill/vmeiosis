s/^#define USB_(BOOT_[^ ]+|HAS_CFG_(IMPLEMENT|HAVE_INTRIN)_[^ ]+) (.*)$//g
s/^#define USB_(CFG_(VENDOR|DEVICE)_(ID|NAME|NAME_LEN)) (.*)$/#define USB_BOOT_\1 \4/g
s/^#define USB_(CFG_(DEVICE_VERSION|SERIAL_NUMBER|SERIAL_NUMBER_LEN)) (.*)$/#define USB_BOOT_\1 \3/g
s/^#define USB_(CFG_(DEVICE|INTERFACE)_(CLASS|SUBCLASS|PROTOCOL)) (.*)$/#define USB_BOOT_\1 \4/g
s/^#define USB_(CFG_HID_REPORT_DESCRIPTOR_LENGTH) (.*)$/#define USB_BOOT_\1 \2/g
s/^#define USB_(CFG_HAVE_INTRIN_ENDPOINT(|3)) (.*)$/#define USB_HAS_\1 \3/g
s/^#define USB_(CFG_SUPPRESS_INTRIN_ENDPOINTS) (.*)$/#define USB_BOOT_\1 \2/g
s/^#define USB_(CFG_DESCR_PROPS_[^ ]+) (.*)$/#define USB_BOOT_\1 \2/g
s/^#define USB_(CFG_INTR_POLL_INTERVAL) (.*)$/#define USB_BOOT_\1 \2/g
s/^#define USB_(CFG_MAX_BUS_POWER) (.*)$/#define USB_BOOT_\1 \2/g
s/^#define USB_(CFG_IMPLEMENT_FN_(WRITE|READ|WRITEOUT)) (.*)$/#define USB_HAS_\1 \3/g
s/^#define __USBCONFIG_H__.*$//g
s/^(#define USB_PUBLIC.*)$/\/\* \1 \*\//g
