#ifndef HAP_DEBUG_H
#define HAP_DEBUG_H

#ifndef HKLOGLEVEL
#define HKLOGLEVEL 4 // 0: DEBUG, 1: INFO, 2: WARNING, 3: ERROR, 4: NONE
#endif

#if HKLOGLEVEL == 0
#define HKLOGDEBUG(str, ...) Serial.printf("[HomeKit] [DEBUG] " str, ## __VA_ARGS__)
#define HKLOGDEBUGSINGLE(...) Serial.printf(__VA_ARGS__)
#else
#define HKLOGDEBUG(...)
#define HKLOGDEBUGSINGLE(...)
#endif

#if HKLOGLEVEL <= 1
#define HKLOGINFO(str, ...) Serial.printf("[HomeKit] [INFO ] " str, ## __VA_ARGS__)
#else
#define HKLOGINFO(...)
#endif

#if HKLOGLEVEL <= 2
#define HKLOGWARNING(str, ...) Serial.printf("[HomeKit] [WARN ] " str, ## __VA_ARGS__)
#else
#define HKLOGWARNING(...)
#endif

#if HKLOGLEVEL <= 3
#define HKLOGERROR(str, ...) Serial.printf("[HomeKit] [ERROR] " str, ## __VA_ARGS__)
#else
#define HKLOGERROR(...)
#endif

#endif // HAP_DEBUG_H