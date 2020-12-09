#ifndef HAP_DEBUG_H
#define HAP_DEBUG_H

#ifndef HKLOGLEVEL
#define HKLOGLEVEL 4 // 0: DEBUG, 1: INFO, 2: WARNING, 3: ERROR, 4: NONE
#endif

#include <HardwareSerial.h>

#if HKLOGLEVEL == 0
#define HKLOGDEBUG(str, ...) Serial.printf_P(PSTR("[HomeKit] [DEBUG] " str), ## __VA_ARGS__)
#define HKLOGDEBUGSINGLE(str, ...) Serial.printf_P(PSTR(str), ## __VA_ARGS__)
#define HKLOGDEBUGBLOCK(str, body, bodySize)  HKLOGDEBUGSINGLE(str " formatted: \r\n"); \
for (size_t i = 0; i < bodySize;) { \
    if (i < 0x10) { \
        HKLOGDEBUGSINGLE("000%x:   ", i); \
    } else if (i < 0x100) { \
        HKLOGDEBUGSINGLE("00%x:   ", i); \
    } else if (i < 0x1000) { \
        HKLOGDEBUGSINGLE("0%x:   ", i); \
    } else { \
        HKLOGDEBUGSINGLE("%x:   ", i); \
    } \
    for (size_t j = 0; j < 8 && i < bodySize; j++) { \
        uint8_t item = *(body + i++); \
        if (item < 0x10) { \
            HKLOGDEBUGSINGLE("0%x ", item); \
        } else { \
            HKLOGDEBUGSINGLE("%x ", item); \
        } \
    } \
    HKLOGDEBUGSINGLE("\r\n"); \
} \
HKLOGDEBUGSINGLE("\r\n-----------\r\n");
#define HKLOGDEBUGBLOCKCMP(str, body1, body2, bodySize)  HKLOGDEBUGSINGLE(str ": \r\n"); \
for (size_t i = 0; i < bodySize;) { \
    if (i < 0x10) { \
        HKLOGDEBUGSINGLE("000%x:   ", i); \
    } else if (i < 0x100) { \
        HKLOGDEBUGSINGLE("00%x:   ", i); \
    } else if (i < 0x1000) { \
        HKLOGDEBUGSINGLE("0%x:   ", i); \
    } else { \
        HKLOGDEBUGSINGLE("%x:   ", i); \
    } \
    for (size_t j = 0; j < 8 && i < bodySize; j++) { \
        uint8_t item = *(body1 + i + j); \
        if (item < 0x10) { \
            HKLOGDEBUGSINGLE("0%x ", item); \
        } else { \
            HKLOGDEBUGSINGLE("%x ", item); \
        } \
    } \
    HKLOGDEBUGSINGLE("    "); \
    for (size_t j = 0; j < 8 && i < bodySize; j++) { \
        uint8_t item = *(body2 + i + j); \
        if (item < 0x10) { \
            HKLOGDEBUGSINGLE("0%x ", item); \
        } else { \
            HKLOGDEBUGSINGLE("%x ", item); \
        } \
    } \
    i += 8; \
    HKLOGDEBUGSINGLE("\r\n"); \
} \
HKLOGDEBUGSINGLE("\r\n-----------\r\n");
#else
#define HKLOGDEBUG(...)
#define HKLOGDEBUGSINGLE(...)
#endif

#if HKLOGLEVEL <= 1
#define HKLOGINFO(str, ...) Serial.printf_P(PSTR("[HomeKit] [INFO ] " str), ## __VA_ARGS__)
#else
#define HKLOGINFO(...)
#endif

#if HKLOGLEVEL <= 2
#define HKLOGWARNING(str, ...) Serial.printf_P(PSTR("[HomeKit] [WARN ] " str), ## __VA_ARGS__)
#else
#define HKLOGWARNING(...)
#endif

#if HKLOGLEVEL <= 3
#define HKLOGERROR(str, ...) Serial.printf_P(PSTR("[HomeKit] [ERROR] " str), ## __VA_ARGS__)
#else
#define HKLOGERROR(...)
#endif

#endif // HAP_DEBUG_H