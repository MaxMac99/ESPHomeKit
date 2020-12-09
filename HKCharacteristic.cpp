/**
 * @file HKCharacteristic.cpp
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief 
 * @version 0.1
 * @date 2020-05-26
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "HKCharacteristic.h"

/**
 * @brief Construct a new HKCharacteristic::HKCharacteristic object
 * Look up infos for Characteristic in Apples HAP manual and look at HKDefinitions.h
 * 
 * @param type Type of characteristic
 * @param value Initial Value
 * @param permissions Permission needed to access values
 * @param description Description of the service
 * @param format Format of values
 * @param unit Unit for values
 * @param minValue Minimum value (only for float and int)
 * @param maxValue Maximum value (only for float and int)
 * @param minStep Minimum step size (only for float and int)
 * @param maxLen Maximum length
 * @param maxDataLen Maximum data length
 * @param validValues Valid values
 * @param validValuesRanges Valid values as range
 */
HKCharacteristic::HKCharacteristic(HKCharacteristicType type, const HKValue &value, uint8_t permissions,
                                   String description, HKFormat format, HKUnit unit, float *minValue, float *maxValue, float *minStep, uint *maxLen, uint *maxDataLen, HKValidValues validValues, HKValidValuesRanges validValuesRanges) : id(0), service(nullptr), type(type), value(value), permissions(permissions), description(std::move(description)), unit(unit), format(format), minValue(minValue), maxValue(maxValue), minStep(minStep), maxLen(maxLen), maxDataLen(maxDataLen), validValues(validValues), validValuesRanges(validValuesRanges), getter(nullptr), setter(nullptr) {

}

/**
 * @brief Destroy the HKCharacteristic::HKCharacteristic object
 * 
 */
HKCharacteristic::~HKCharacteristic() {
    delete minValue;
    delete maxValue;
    delete minStep;
    delete maxLen;
    delete maxDataLen;
}

/**
 * @brief Set function to call when it needs current value
 * 
 * @param getter Function
 */
void HKCharacteristic::setGetter(const std::function<const HKValue &()> &getter) {
    HKCharacteristic::getter = getter;
}

/**
 * @brief Set function to call when value was updated
 * 
 * @param setter Function
 */
void HKCharacteristic::setSetter(const std::function<void(const HKValue)> &setter) {
    HKCharacteristic::setter = setter;
}

/**
 * @brief Get assigned type of characteristic
 * 
 * @return HKCharacteristicType Type of the characteristic
 */
HKCharacteristicType HKCharacteristic::getType() const {
    return type;
}

/**
 * @brief Get current value of the characteristic
 * 
 * @return const HKValue& Current value
 */
const HKValue &HKCharacteristic::getValue() const {
    if (getter) {
        return getter();
    }
    return value;
}

/**
 * @brief Get parent service
 * 
 * @return HKService* Parent Service
 */
HKService *HKCharacteristic::getService() {
    return service;
}

/**
 * @brief Get id
 * 
 * @return uint Id
 */
uint HKCharacteristic::getId() const {
    return id;
}

/**
 * @brief Convert to JSON String
 * 
 * @param json Target JSON object
 * @param jsonValue Optional value to set value in json
 * @param jsonFormatOptions Options to select which parameters to print
 * @param client Client requesting characteristic
 */
void HKCharacteristic::serializeToJSON(JSON &json, HKValue *jsonValue, uint jsonFormatOptions, HKClient *client) {
    json.setString("iid");
    json.setInt(id);

    if (jsonFormatOptions & HKCharacteristicFormatType) {
        json.setString("type");
        String typeConv = String(type, HEX);
        typeConv.toUpperCase();
        json.setString(typeConv.c_str());
    }

    if (jsonFormatOptions & HKCharacteristicFormatPerms) {
        json.setString("perms");
        json.startArray();
        if (permissions & HKPermissionPairedRead) {
            json.setString("pr");
        }
        if (permissions & HKPermissionPairedWrite) {
            json.setString("pw");
        }
        if (permissions & HKPermissionNotify) {
            json.setString("ev");
        }
        if (permissions & HKPermissionAdditionalAuthorization) {
            json.setString("aa");
        }
        if (permissions & HKPermissionTimedWrite) {
            json.setString("tw");
        }
        if (permissions & HKPermissionHidden) {
            json.setString("hd");
        }
        json.endArray();
    }

    if (client && (jsonFormatOptions & HKCharacteristicFormatEvents) && (permissions & HKPermissionNotify)) {
        json.setString("ev");
        json.setBool(hasCallbackEvent(client));
    }

    if (jsonFormatOptions & HKCharacteristicFormatMeta) {
        json.setString("description");
        json.setString(description.c_str());

        json.setString("format");
        switch (format) {
            case HKFormatBool:
                json.setString("bool");
                break;
            case HKFormatUInt8:
                json.setString("uint8");
                break;
            case HKFormatUInt16:
                json.setString("uint16");
                break;
            case HKFormatUInt32:
                json.setString("uint32");
                break;
            case HKFormatUInt64:
                json.setString("uint64");
                break;
            case HKFormatInt:
                json.setString("int");
                break;
            case HKFormatFloat:
                json.setString("float");
                break;
            case HKFormatString:
                json.setString("string");
                break;
            case HKFormatTLV:
                json.setString("tlv8");
                break;
            case HKFormatData:
                json.setString("data");
                break;
        }

        switch (unit) {
            case HKUnitNone:
                break;
            case HKUnitCelsius:
                json.setString("unit");
                json.setString("celsius");
                break;
            case HKUnitPercentage:
                json.setString("unit");
                json.setString("percentage");
                break;
            case HKUnitArcdegrees:
                json.setString("unit");
                json.setString("arcdegrees");
                break;
            case HKUnitLux:
                json.setString("unit");
                json.setString("lux");
                break;
            case HKUnitSeconds:
                json.setString("unit");
                json.setString("seconds");
                break;
        }

        if (minValue) {
            json.setString("minValue");
            json.setFloat(*minValue);
        }

        if (maxValue) {
            json.setString("maxValue");
            json.setFloat(*maxValue);
        }

        if (minStep) {
            json.setString("minStep");
            json.setFloat(*minStep);
        }

        if (maxLen) {
            json.setString("maxLen");
            json.setFloat(*maxLen);
        }

        if (maxDataLen) {
            json.setString("maxDataLen");
            json.setFloat(*maxDataLen);
        }

        if (validValues.count) {
            json.setString("valid-values");
            json.startArray();

            for (int i = 0; i < validValues.count; i++) {
                json.setInt(validValues.values[i]);
            }
            json.endArray();
        }

        if (validValuesRanges.count) {
            json.setString("valid-values-range");
            json.startArray();

            for (int i = 0; i < validValuesRanges.count; i++) {
                json.startArray();

                json.setInt(validValuesRanges.ranges[i].start);
                json.setInt(validValuesRanges.ranges[i].end);

                json.endArray();
            }
            json.endArray();
        }
    }

    if (permissions & HKPermissionPairedRead) {
        HKValue v = jsonValue ? *jsonValue : getter ? getter() : value;

        if (v.isNull) {
            json.setString("value");
            json.setNull();
        } else if (v.format != format) {
            HKLOGERROR("[HKCharacteristic::serializeToJSON] Value format is different from format (id=%d.%d: %d != %d, service=%s, type=%d)\r\n", service->getAccessory()->getId(), id, v.format, format, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
        } else {
            switch (v.format) {
                case HKFormatBool:
                    json.setString("value");
                    json.setBool(v.boolValue);
                    break;
                case HKFormatUInt8:
                case HKFormatUInt16:
                case HKFormatUInt32:
                case HKFormatUInt64:
                case HKFormatInt:
                    json.setString("value");
                    json.setInt(v.intValue);
                    break;
                case HKFormatFloat:
                    json.setString("value");
                    json.setFloat(v.floatValue);
                    break;
                case HKFormatString:
                    json.setString("value");
                    json.setString(v.stringValue);
                    break;
                case HKFormatTLV:
                    break;
                case HKFormatData:
                    break;
                default:
                    break;
            }
        }
    }
}

/**
 * @brief Set current value from JSON input
 * 
 * @param jsonValue input as JSON String
 * @return HAPStatus Was setting the value successful
 */
HAPStatus HKCharacteristic::setValue(const String& jsonValue) {
    if (!(permissions & HKPermissionPairedWrite)) {
        HKLOGERROR("[HKCharacteristic::setValue] Failed to set characteristic value (id=%d.%d, service=%s, type=%d): no write permission\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
        return HAPStatusReadOnly;
    }

    HKValue hkValue = HKValue();
    switch (format) {
        case HKFormatBool: {
            bool result;
            String compare = jsonValue;
            compare.toLowerCase();
            if (compare == "false" || compare == "0") {
                result = false;
            } else if (compare == "true" || compare == "1") {
                result = true;
            } else {
                HKLOGERROR("[HKCharacteristic::setValue] Failed to update (id=%d.%d, service=%s, type=%d): Json is not of type bool (%s)\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, jsonValue.c_str());
                return HAPStatusInvalidValue;
            }

            HKLOGINFO("[HKCharacteristic::setValue] Update Characteristic (id=%d.%d, service=%s, type=%d) with bool: %d\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, result);

            if (setter) {
                hkValue = HKValue(HKFormatBool, result);
                setter(hkValue);
            } else {
                hkValue = value;
                value.boolValue = result;
            }
            break;
        }
        case HKFormatUInt8:
        case HKFormatUInt16:
        case HKFormatUInt32:
        case HKFormatUInt64:
        case HKFormatInt: {
            uint64_t result = jsonValue.toInt();

            uint64_t checkMinValue = 0;
            uint64_t checkMaxValue = 0;
            switch (format) {
                case HKFormatUInt8: {
                    checkMinValue = 0;
                    checkMaxValue = 0xFF;
                    break;
                }
                case HKFormatUInt16:{
                    checkMinValue = 0;
                    checkMaxValue = 0xFFFF;
                    break;
                }
                case HKFormatUInt32: {
                    checkMinValue = 0;
                    checkMaxValue = 0xFFFFFFFF;
                    break;
                }
                case HKFormatUInt64: {
                    checkMinValue = 0;
                    checkMaxValue = 0xFFFFFFFFFFFFFFFF;
                    break;
                }
                case HKFormatInt: {
                    checkMinValue = -2147483648;
                    checkMaxValue = 2147483647;
                    break;
                }
                default:
                    break;
            }

            if (minValue) {
                checkMinValue = std::llrintf(*minValue);
            }
            if (maxValue) {
                checkMaxValue = std::llrintf(*maxValue);
            }

            if (result < checkMinValue || result > checkMaxValue) {
                HKLOGERROR("[HKCharacteristic::setValue] Failed to update (id=%d.%d, service=%s, type=%d): int is not in range\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
                return HAPStatusInvalidValue;
            }

            if (validValues.count) {
                bool matches = false;
                for (int i = 0; i < validValues.count; i++) {
                    if (result == validValues.values[i]) {
                        matches = true;
                        break;
                    }
                }

                if (!matches) {
                    HKLOGERROR("[HKCharacteristic::setValue] Failed to update (id=%d.%d, service=%s, type=%d): int is not one of valid values\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
                    return HAPStatusInvalidValue;
                }
            }

            if (validValuesRanges.count) {
                bool matches = false;
                for (int i = 0; i < validValuesRanges.count; i++) {
                    if (result >= validValuesRanges.ranges[i].start && result <= validValuesRanges.ranges[i].end) {
                        matches = true;
                    }
                }

                if (!matches) {
                    HKLOGERROR("[HKCharacteristic::setValue] Failed to update (id=%d.%d, service=%s, type=%d): int is not one of valid values range\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
                    return HAPStatusInvalidValue;
                }
            }

            HKLOGINFO("[HKCharacteristic::setValue] Update Characteristic (id=%d.%d, service=%s, type=%d) with int: %" PRIu64 "\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, result);

            if (setter) {
                hkValue = HKValue(format, result);
                setter(hkValue);
            } else {
                hkValue = value;
                value.intValue = result;
            }
            break;
        }
        case HKFormatFloat: {
            float result = jsonValue.toFloat();

            if ((minValue && result < *minValue) || (maxValue && result > *maxValue)) {
                HKLOGERROR("[HKCharacteristic::setValue] Failed to update (id=%d.%d, service=%s, type=%d): float is not in range\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
                return HAPStatusInvalidValue;
            }

            HKLOGINFO("[HKCharacteristic::setValue] Update Characteristic (id=%d.%d, service=%s, type=%d) with float: %f\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, result);

            if (setter) {
                hkValue = HKValue(HKFormatFloat, result);
                setter(hkValue);
            } else {
                hkValue = value;
                value.floatValue = result;
            }
            break;
        }
        case HKFormatString: {
            const char *result = jsonValue.c_str();

            uint checkMaxLen = maxLen ? *maxLen : 64;
            if (strlen(result) > checkMaxLen) {
                HKLOGERROR("[HKCharacteristic::setValue] Failed to update (id=%d.%d, service=%s, type=%d): String is too long\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
                return HAPStatusInvalidValue;
            }

            HKLOGINFO("[HKCharacteristic::setValue] Update Characteristic (id=%d.%d, service=%s, type=%d) with string: %s\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, result);

            if (setter) {
                hkValue = HKValue(HKFormatString, result);
                setter(hkValue);
            } else {
                hkValue = value;
                value.stringValue = result;
            }
            break;
        }
        case HKFormatTLV: {
            HKLOGERROR("[HKCharacteristic::setValue] (id=%d.%d, service=%s, type=%d) TLV not supported yet\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
            break;
        }
        case HKFormatData: {
            HKLOGERROR("[HKCharacteristic::setValue] (id=%d.%d, service=%s, type=%d) Data not supported yet\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
            break;
        }
    }

    if (!hkValue.isNull) {
        if (getter) {
            hkValue = getter();
        }
        notify(hkValue);
    }
    return HAPStatusSuccess;
}

/**
 * @brief Register client for updates of characteristic (with notify)
 * 
 * @param client Client to register
 * @param jsonValue register or deregister as JSON String
 * @return HAPStatus Successfully set value
 */
HAPStatus HKCharacteristic::setEvent(HKClient *client, const String& jsonValue) {
    bool events;
    String compare = jsonValue;
    compare.toLowerCase();
    if (compare == "false" || compare == "0") {
        events = false;
    } else if (compare == "true" || compare == "1") {
        events = true;
    } else {
        HKLOGERROR("[HKCharacteristic::setEvent] Failed to update (id=%d.%d, service=%s, type=%d): Json is not of type bool (%s)\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, jsonValue.c_str());
        return HAPStatusInvalidValue;
    }

    if (!(permissions & HKPermissionNotify)) {
        HKLOGERROR("[HKCharacteristic::setEvent] Failed to update (id=%d.%d, service=%s, type=%d): notifications are not supported\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
        return HAPStatusNotificationsUnsupported;
    }

    if (events) {
        HKLOGDEBUG("Add callback id=%d\r\n", id);
        addCallbackEvent(client);
    } else {
        HKLOGDEBUG("Remove callback id=%d\r\n", id);
        removeCallbackEvent(client);
    }
    return HAPStatusSuccess;
}

/**
 * @brief Notify connected clients about a change of value
 * 
 * @param newValue New value
 */
void HKCharacteristic::notify(const HKValue& newValue) {
    for (HKClient *client : notifiers) {
        client->scheduleEvent(this, newValue);
    }
}

/**
 * @brief Is client registered for update notifications
 * 
 * @param client Registered client
 * @return true Client was registered
 * @return false Client was not registered
 */
bool HKCharacteristic::hasCallbackEvent(HKClient *client) {
    return std::find(notifiers.begin(), notifiers.end(), client) != notifiers.end();
}

/**
 * @brief Remove client for update notifications
 * 
 * @param client Client to remove
 */
void HKCharacteristic::removeCallbackEvent(HKClient *client) {
    auto comp = std::find(notifiers.begin(), notifiers.end(), client);
    if (comp != notifiers.end()) {
        notifiers.erase(comp);
    }
}

/**
 * @brief Add client for update notifications
 * 
 * @param client Client to add
 */
void HKCharacteristic::addCallbackEvent(HKClient *client) {
    auto comp = std::find(notifiers.begin(), notifiers.end(), client);
    if (comp != notifiers.end()) {
        notifiers.push_back(client);
    }
}

