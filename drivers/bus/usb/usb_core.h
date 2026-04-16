#ifndef BHARAT_DRIVER_USB_CORE_H
#define BHARAT_DRIVER_USB_CORE_H

#include "../../include/driver_core.h"
#include <stdint.h>

// This file contains minimal architecture headers for the USB subsystem
// to establish the structure, without xHCI or runtime implementations yet.

// ---------------------------------------------------------
// USB Core Types
// ---------------------------------------------------------

typedef enum {
    USB_SPEED_UNKNOWN = 0,
    USB_SPEED_LOW,
    USB_SPEED_FULL,
    USB_SPEED_HIGH,
    USB_SPEED_SUPER
} usb_speed_t;

typedef enum {
    USB_TRANSFER_CONTROL = 0,
    USB_TRANSFER_ISOCHRONOUS,
    USB_TRANSFER_BULK,
    USB_TRANSFER_INTERRUPT
} usb_transfer_type_t;

// ---------------------------------------------------------
// Standard Descriptors (Shapes only)
// ---------------------------------------------------------

typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

// ---------------------------------------------------------
// USB Host Controller Contract
// ---------------------------------------------------------

// Extends the standard device_desc_t bus_data
typedef struct {
    device_desc_t* controller_dev;
    int (*start_controller)(device_desc_t* dev);
    int (*stop_controller)(device_desc_t* dev);
    int (*submit_transfer)(device_desc_t* dev, void* urb); // urb structure omitted for now
} usb_hcd_ops_t;

// Minimal USB core initialization
int usb_core_init(void);

#endif // BHARAT_DRIVER_USB_CORE_H