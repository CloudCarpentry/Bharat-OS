#ifndef BHARAT_INPUT_H
#define BHARAT_INPUT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Input Event Types (Inspired by Linux input subsystem)
 */
typedef enum {
    EV_SYN = 0x00,
    EV_KEY = 0x01,
    EV_REL = 0x02,
    EV_ABS = 0x03,
    EV_MSC = 0x04,
    EV_SW  = 0x05,
    EV_LED = 0x11,
    EV_SND = 0x12,
    EV_REP = 0x14,
    EV_FF  = 0x15,
    EV_PWR = 0x16,
    EV_FF_STATUS = 0x17,
} bharat_input_event_type_t;

/**
 * Standard Key Codes
 */
#define KEY_RESERVED    0
#define KEY_ESC         1
#define KEY_1           2
#define KEY_2           3
#define KEY_3           4
#define KEY_4           5
#define KEY_5           6
#define KEY_6           7
#define KEY_7           8
#define KEY_8           9
#define KEY_9           10
#define KEY_0           11
#define KEY_MINUS       12
#define KEY_EQUAL       13
#define KEY_BACKSPACE   14
#define KEY_TAB         15
#define KEY_Q           16
#define KEY_W           17
#define KEY_E           18
#define KEY_R           19
#define KEY_T           20
#define KEY_Y           21
#define KEY_U           22
#define KEY_I           23
#define KEY_O           24
#define KEY_P           25
#define KEY_LEFTBRACE   26
#define KEY_RIGHTBRACE  27
#define KEY_ENTER       28
#define KEY_LEFTCTRL    29
#define KEY_A           30
#define KEY_S           31
#define KEY_D           32
#define KEY_F           33
#define KEY_G           34
#define KEY_H           35
#define KEY_J           36
#define KEY_K           37
#define KEY_L           38
#define KEY_SEMICOLON   39
#define KEY_APOSTROPHE  40
#define KEY_GRAVE       41
#define KEY_LEFTSHIFT   42
#define KEY_BACKSLASH   43
#define KEY_Z           44
#define KEY_X           45
#define KEY_C           46
#define KEY_V           47
#define KEY_B           48
#define KEY_N           49
#define KEY_M           50
#define KEY_COMMA       51
#define KEY_DOT         52
#define KEY_SLASH       53
#define KEY_RIGHTSHIFT  54
#define KEY_KPASTERISK  55
#define KEY_LEFTALT     56
#define KEY_SPACE       57
#define KEY_CAPSLOCK    58

/**
 * Input Event Structure
 */
typedef struct {
    uint32_t timestamp_sec;
    uint32_t timestamp_usec;
    uint16_t type;
    uint16_t code;
    int32_t  value;
} bharat_input_event_t;

struct bharat_input_device;

/**
 * Operations for an input device.
 */
typedef struct {
    int (*open)(struct bharat_input_device *dev);
    void (*close)(struct bharat_input_device *dev);
    int (*flush)(struct bharat_input_device *dev);
} bharat_input_device_ops_t;

/**
 * Input Device Abstraction
 */
typedef struct bharat_input_device {
    const char *name;
    uint32_t id;
    const bharat_input_device_ops_t *ops;
    void *priv_data;

    // Capability bitmasks
    uint32_t evbit[8];  // Supports EV_*
    uint32_t keybit[24]; // Supports KEY_*
    uint32_t relbit[4];  // Supports REL_*
    uint32_t absbit[4];  // Supports ABS_*
} bharat_input_device_t;

// Core API
int bharat_input_register(bharat_input_device_t *dev);
void bharat_input_report_event(bharat_input_device_t *dev, uint16_t type, uint16_t code, int32_t value);
void bharat_input_sync(bharat_input_device_t *dev);

#endif // BHARAT_INPUT_H
