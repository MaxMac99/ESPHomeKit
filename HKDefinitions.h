//
// Created by Max Vissing on 2019-06-12.
//

#ifndef LED_HAP_ESP8266_HKDEFINITIONS_H
#define LED_HAP_ESP8266_HKDEFINITIONS_H

enum HAPStatus {
    // This specifies a success for the request
            HAPStatusSuccess = 0,
    // Request denied due to insufficient privileges
            HAPStatusInsufficientPrivileges = -70401,
    // Unable to communicate with requested services,
    // e.g. the power to the accessory was turned off
            HAPStatusNoAccessoryConnection = -70402,
    // Resource is busy, try again
            HAPStatusResourceBusy = -70403,
    // Connot write to read only characteristic
            HAPStatusReadOnly = -70404,
    // Cannot read from a write only characteristic
            HAPStatusWriteOnly = -70405,
    // Notification is not supported for characteristic
            HAPStatusNotificationsUnsupported = -70406,
    // Out of resources to process request
            HAPStatusOutOfResources = -70407,
    // Operation timed out
            HAPStatusTimeout = -70408,
    // Resource does not exist
            HAPStatusNoResource = -70409,
    // Accessory received an invalid value in a write request
            HAPStatusInvalidValue = -70410,
    // Insufficient Authorization
            HAPStatusInsufficientAuthorization = -70411,
};

enum HKAccessoryCategory {
    HKAccessoryOther               = 1,
    HKAccessoryBridge              = 2,
    HKAccessoryFan                 = 3,
    HKAccessoryGarage              = 4,
    HKAccessoryLightbulb           = 5,
    HKAccessoryDoorLock             = 6,
    HKAccessoryOutlet              = 7,
    HKAccessorySwitch              = 8,
    HKAccessoryThermostat          = 9,
    HKAccessorySensor              = 10,
    HKAccessorySecuritySystem      = 11,
    HKAccessoryDoor                = 12,
    HKAccessoryWindow              = 13,
    HKAccessoryWindowCovering      = 14,
    HKAccessoryProgrammableSwitch  = 15,
    HKAccessoryRangeExtender       = 16,
    HKAccessoryIPCamera            = 17,
    HKAccessoryVideoDoorBell       = 18,
    HKAccessoryAirPurifier         = 19,
    HKAccessoryHeater              = 20,
    HKAccessoryAirConditioner      = 21,
    HKAccessoryHumidifier          = 22,
    HKAccessoryDehumidifier        = 23,
    HKAccessoryAppleTV             = 24,
    HKAccessorySpeaker             = 26,
    HKAccessoryAirport             = 27,
    HKAccessorySprinkler           = 28,
    HKAccessoryFaucet              = 29,
    HKAccessoryShowerHead          = 30,
    HKAccessoryTelevision          = 31,
    HKAccessoryTargetController    = 32
};

enum HKServiceType {
    HKServiceAccessoryInfo      = 0x3E,
    HKServiceCamera             = 0x3F,
    HKServiceFan                = 0x40,
    HKServiceGarageDoorOpener   = 0x41,
    HKServiceLightBulb          = 0x43,
    HKServiceLockManagement     = 0x44,
    HKServiceLockMechanism      = 0x45,
    HKServiceMicrophone         = 0x46,
    HKServiceOutlet             = 0x47,
    HKServiceSpeaker            = 0x48,
    HKServiceSwitch             = 0x49,
    HKServiceThermostat         = 0x4A,

    HKServiceAlarmSystem        = 0x7E,
    HKServiceBridgingState      = 0x62,
    HKServiceCarbonMonoxideSensor = 0x7F,
    HKServiceContactSensor      = 0x80,
    HKServiceDoor               = 0x81,
    HKServiceHumiditySensor     = 0x82,
    HKServiceLeakSensor         = 0x83,
    HKServiceLightSensor        = 0x84,
    HKServiceMotionSensor       = 0x85,
    HKServiceOccupancySensor    = 0x86,
    HKServiceSmokeSensor        = 0x87,
    HKServiceProgrammableSwitch_stateful = 0x88,
    HKServiceProgrammableSwitch_stateless = 0x89,
    HKServiceTemperatureSensor  = 0x8A,
    HKServiceWindow             = 0x8B,
    HKServiceWindowCover        = 0x8C,
    HKServiceAirQualitySensor   = 0x8C,
    HKServiceSecurityAlarm      = 0x8E,
    HKServiceCharging           = 0x8F,

    HKServiceBattery            = 0x96,
    HKServiceCarbonDioxideSensor= 0x97,
};

enum HKCharacteristicType {
    HKCharacteristicAdminOnlyAccess = 0x1,
    HKCharacteristicAudioChannels = 0x2,
    HKCharacteristicAudioCodexName = 0x3,
    HKCharacteristicAudioCodexParameter = 0x4,
    HKCharacteristicAudioFeedback = 0x5,
    HKCharacteristicAudioPropAttr = 0x6,
    HKCharacteristicAudioValAttr = 0x7,
    HKCharacteristicBrightness = 0x8,
    HKCharacteristicCameraNightVision = 0x9,
    HKCharacteristicCameraPan = 0xA,
    HKCharacteristicCameraTilt = 0xB,
    HKCharacteristicCameraZoom = 0xC,
    HKCharacteristicCoolingThreshold = 0xD,
    HKCharacteristicCurrentDoorState = 0xE,
    HKCharacteristicCurrentHeatCoolMode = 0xF,
    HKCharacteristicCurrentHumidity = 0x10,
    HKCharacteristicCurrentTemperature = 0x11,
    HKCharacteristicHeatingThreshold = 0x12,
    HKCharacteristicHue = 0x13,
    HKCharacteristicIdentify = 0x14,
    HKCharacteristicInputVolume = 0x15,
    HKCharacteristicIpCameraStart = 0x16,
    HKCharacteristicIpCameraStop = 0x17,
    HKCharacteristicLockControlPoint = 0x19,
    HKCharacteristicLockAutoTimeout = 0x1A,
    HKCharacteristicLockLastAction = 0x1C,
    HKCharacteristicLockCurrentState = 0x1D,
    HKCharacteristicLockTargetState = 0x1E,
    HKCharacteristicLogs = 0x1F,
    HKCharacteristicManufactuer = 0x20,
    HKCharacteristicModelName = 0x21,
    HKCharacteristicMotionDetect = 0x22,
    HKCharacteristicName = 0x23,
    HKCharacteristicObstruction = 0x24,
    HKCharacteristicOn = 0x25,
    HKCharacteristicOutletInUse = 0x26,
    HKCharacteristicOutputVolume = 0x27,
    HKCharacteristicRotationDirection = 0x28,
    HKCharacteristicRotationSpeed = 0x29,
    HKCharacteristicRtcpExtProp = 0x2A,
    HKCharacteristicRtcpVideoPayload = 0x2B,
    HKCharacteristicRtcpAudioPayload = 0x2C,
    HKCharacteristicRtcpAudioClock = 0x2D,
    HKCharacteristicRtcpProtocol = 0x2E,
    HKCharacteristicSaturation = 0x2F,
    HKCharacteristicSerialNumber = 0x30,
    HKCharacteristicSrtpCyptoSuite = 0x31,
    HKCharacteristicTargetDoorState = 0x32,
    HKCharacteristicTargetHeatCoolMode = 0x33,
    HKCharacteristicTargetHumidity = 0x34,
    HKCharacteristicTargetTemperature = 0x35,
    HKCharacteristicTemperatureUnit = 0x36,
    HKCharacteristicVersion = 0x37,
    HKCharacteristicVideoCodexName = 0x38,
    HKCharacteristicVideoCodexPara = 0x39,
    HKCharacteristicVideoMirror = 0x3A,
    HKCharacteristicVideoPropAttr = 0x3B,
    HKCharacteristicVideoRotation = 0x3C,
    HKCharacteristicVideoValAttr = 0x3D,

 // pragma - The following is only default by the device after iOS 9

    HKCharacteristicFirmwareRevision = 0x52,
    HKCharacteristicHardwareRevision = 0x53,
    HKCharacteristicSoftwareRevision = 0x54,

    HKCharacteristicReachable = 0x63,

    HKCharacteristicAirParticulateDensity = 0x64,
    HKCharacteristicAirParticulateSize = 0x65,
    HKCharacteristicAirQuality = 0x95,
    HKCharacteristicCarbonDioxideDetected = 0x92,
    HKCharacteristicCarbonMonoxideDetected = 0x69,
    HKCharacteristicCarbonDioxideLevel = 0x93,
    HKCharacteristicCarbonMonoxideLevel = 0x90,
    HKCharacteristicCarbonDioxidePeakLevel = 0x94,
    HKCharacteristicCarbonMonoxidePeakLevel = 0x91,
    HKCharacteristicSmokeDetected = 0x76,

    HKCharacteristicAlarmCurrentState = 0x66,
    HKCharacteristicAlarmTargetState = 0x67,
    HKCharacteristicBatteryLevel = 0x68,
    HKCharacteristicContactSensorState = 0x6A,
    HKCharacteristicHoldPosition = 0x6F,
    HKCharacteristicLeakDetected = 0x70,
    HKCharacteristicOccupancyDetected = 0x71,

    HKCharacteristicCurrentAmbientLightLevel = 0x6B,
    HKCharacteristicCurrentHorizontalTiltAngle = 0x6C,
    HKCharacteristicTargetHorizontalTiltAngle = 0x7B,
    HKCharacteristicCurrentPosition = 0x6D,
    HKCharacteristicTargetPosition = 0x7C,
    HKCharacteristicCurrentVerticalTiltAngle = 0x6E,
    HKCharacteristicTargetVerticalTiltAngle = 0x7D,

    HKCharacteristicPositionState = 0x72,
    HKCharacteristicProgrammableSwitchEvent = 0x73,
    HKCharacteristicProgrammableSwitchOutputState = 0x74,

    HKCharacteristicSensorActive = 0x75,
    HKCharacteristicSensorFault = 0x77,
    HKCharacteristicSensorJammed = 0x78,
    HKCharacteristicSensorLowBattery = 0x79,
    HKCharacteristicSensorTampered = 0x7A,
    HKCharacteristicSensorChargingState = 0x8F,
};

enum HKFormat {
    HKFormatBool,
    HKFormatUInt8,
    HKFormatUInt16,
    HKFormatUInt32,
    HKFormatUInt64,
    HKFormatInt,
    HKFormatFloat,
    HKFormatString,
    HKFormatTLV,
    HKFormatData
};

enum HKUnit {
    HKUnitNone,
    HKUnitCelsius,
    HKUnitPercentage,
    HKUnitArcdegrees,
    HKUnitLux,
    HKUnitSeconds
};

enum HKPermission {
    HKPermissionPairedRead = 1,
    HKPermissionPairedWrite = 2,
    HKPermissionNotify = 4,
    HKPermissionAdditionalAuthorization = 8,
    HKPermissionTimedWrite = 16,
    HKPermissionHidden = 32
};

enum HKCharacteristicFormat {
    HKCharacteristicFormatType = 1,
    HKCharacteristicFormatMeta = 2,
    HKCharacteristicFormatPerms = 4,
    HKCharacteristicFormatEvents = 8
};

enum PairingPermission {
    PairingPermissionAdmin = (1 << 0)
};

class HKValue {
public:
    bool isNull : 1;
    bool isStatic : 1;
    HKFormat format : 6;
    union {
        bool boolValue;
        int intValue;
        float floatValue;
        const char *stringValue;
        //TLVValues *tlvValues;
        // Data
    };

    inline HKValue() : isNull(true), isStatic(false), format(HKFormatBool) {};
    inline explicit HKValue(HKFormat format) : isNull(false), isStatic(false), format(format) {};
    inline HKValue(HKFormat format, bool value) : isNull(false), isStatic(false), format(format), boolValue(value) {};
    inline explicit HKValue(HKFormat format, int value) : isNull(false), isStatic(false), format(format), intValue(value) {};
    inline explicit HKValue(HKFormat format, uint8_t value) : isNull(false), isStatic(false), format(format), intValue(value) {};
    inline explicit HKValue(HKFormat format, uint16_t value) : isNull(false), isStatic(false), format(format), intValue(value) {};
    inline explicit HKValue(HKFormat format, uint32_t value) : isNull(false), isStatic(false), format(format), intValue(value) {};
    inline explicit HKValue(HKFormat format, uint64_t value) : isNull(false), isStatic(false), format(format), intValue(value) {};
    inline explicit HKValue(HKFormat format, float value) : isNull(false), isStatic(false), format(format), floatValue(value) {};
    inline explicit HKValue(HKFormat format, const char *value) : isNull(false), isStatic(false), format(format) {
        stringValue = strdup(value);
    };
    inline explicit HKValue(HKFormat format, const String& value) : isNull(false), isStatic(false), format(format) {
        stringValue = strdup(value.c_str());
    };
    inline HKValue(const HKValue &other) : isNull(other.isNull), isStatic(other.isStatic), format(other.format) {
        switch (format) {
            case HKFormatBool:
                boolValue = other.boolValue;
                break;
            case HKFormatInt:
            case HKFormatUInt8:
            case HKFormatUInt16:
            case HKFormatUInt32:
            case HKFormatUInt64:
                intValue = other.intValue;
                break;
            case HKFormatFloat:
                floatValue = other.floatValue;
                break;
            case HKFormatString:
                stringValue = strdup(other.stringValue);
                break;
            default:
                break;
        }
    };
    inline ~HKValue() {
        if (format == HKFormatString) {
            free((char *) stringValue);
        }
    }

    inline bool operator==(const HKValue& b)
    {
        if (isNull != b.isNull) {
            return false;
        }
        if (format != b.format) {
            return false;
        }

        switch (format) {
            case HKFormatBool:
                return boolValue == b.boolValue;
            case HKFormatUInt8:
            case HKFormatUInt16:
            case HKFormatUInt32:
            case HKFormatUInt64:
            case HKFormatInt:
                return intValue == b.intValue;
            case HKFormatFloat:
                return floatValue == b.floatValue;
            case HKFormatString:
                return !strcmp(stringValue, b.stringValue);
            case HKFormatTLV:
                /*if (!tlvValues && !b.tlvValues) {
                    return true;
                }
                if (!tlvValues || !b.tlvValues) {
                    return false;
                }
                */
                return false;

            case HKFormatData:
                return false;
            default:
                return false;
        }
    }
};

#endif //LED_HAP_ESP8266_HKDEFINITIONS_H
