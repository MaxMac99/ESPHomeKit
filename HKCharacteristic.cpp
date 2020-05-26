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


HKCharacteristic::HKCharacteristic(HKCharacteristicType type, const HKValue &value, uint8_t permissions,
                                   String description, HKFormat format, HKUnit unit, float *minValue, float *maxValue, float *minStep, unsigned int *maxLen, unsigned int *maxDataLen, HKValidValues validValues, HKValidValuesRanges validValuesRanges) : id(0), service(nullptr), type(type), value(value), permissions(permissions), description(std::move(description)), unit(unit), format(format), minValue(minValue), maxValue(maxValue), minStep(minStep), maxLen(maxLen), maxDataLen(maxDataLen), validValues(validValues), validValuesRanges(validValuesRanges), getter(nullptr), setter(nullptr), callback(nullptr) {

}

HKCharacteristic::~HKCharacteristic() {
    delete minValue;
    delete maxValue;
    delete minStep;
    delete maxLen;
    delete maxDataLen;
}

void HKCharacteristic::setGetter(const std::function<HKValue()> &getter) {
    HKCharacteristic::getter = getter;
}

void HKCharacteristic::setSetter(const std::function<void(const HKValue)> &setter) {
    HKCharacteristic::setter = setter;
}

HKCharacteristicType HKCharacteristic::getType() const {
    return type;
}

const HKValue &HKCharacteristic::getValue() const {
    if (getter) {
        return getter();
    }
    return value;
}

unsigned int HKCharacteristic::getId() const {
    return id;
}

void HKCharacteristic::serializeToJSON(JSON &json, HKValue *jsonValue, unsigned int jsonFormatOptions, HKClient *client) {
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
        if (permissions & PermissionPairedRead) {
            json.setString("pr");
        }
        if (permissions & PermissionPairedWrite) {
            json.setString("pw");
        }
        if (permissions & PermissionNotify) {
            json.setString("ev");
        }
        if (permissions & PermissionAdditionalAuthorization) {
            json.setString("aa");
        }
        if (permissions & PermissionTimedWrite) {
            json.setString("tw");
        }
        if (permissions & PermissionHidden) {
            json.setString("hd");
        }
        json.endArray();
    }

    if (client && (jsonFormatOptions & HKCharacteristicFormatEvents) && (permissions & PermissionNotify)) {
        json.setString("ev");
        json.setBool(hasCallbackEvent(client));
    }

    if (jsonFormatOptions & HKCharacteristicFormatMeta) {
        json.setString("description");
        json.setString(description.c_str());

        json.setString("format");
        switch (format) {
            case FormatBool:
                json.setString("bool");
                break;
            case FormatUInt8:
                json.setString("uint8");
                break;
            case FormatUInt16:
                json.setString("uint16");
                break;
            case FormatUInt32:
                json.setString("uint32");
                break;
            case FormatUInt64:
                json.setString("uint64");
                break;
            case FormatInt:
                json.setString("int");
                break;
            case FormatFloat:
                json.setString("float");
                break;
            case FormatString:
                json.setString("string");
                break;
            case FormatTLV:
                json.setString("tlv8");
                break;
            case FormatData:
                json.setString("data");
                break;
        }

        switch (unit) {
            case UnitNone:
                break;
            case Celsius:
                json.setString("unit");
                json.setString("celsius");
                break;
            case UnitPercentage:
                json.setString("unit");
                json.setString("percentage");
                break;
            case UnitArcdegrees:
                json.setString("unit");
                json.setString("arcdegrees");
                break;
            case UnitLux:
                json.setString("unit");
                json.setString("lux");
                break;
            case UnitSeconds:
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

    if (permissions & PermissionPairedRead) {
        HKValue v = jsonValue ? *jsonValue : getter ? getter() : value;

        if (v.isNull) {
            json.setString("value");
            json.setNull();
        } else if (v.format != format) {
            HKLOGERROR("[HKCharacteristic::serializeToJSON] Value format is different from format (id=%d.%d: %d != %d, service=%s, type=%d)\r\n", service->getAccessory()->getId(), id, v.format, format, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
        } else {
            switch (v.format) {
                case FormatBool:
                    json.setString("value");
                    json.setBool(v.boolValue);
                    break;
                case FormatUInt8:
                case FormatUInt16:
                case FormatUInt32:
                case FormatUInt64:
                case FormatInt:
                    json.setString("value");
                    json.setInt(v.intValue);
                    break;
                case FormatFloat:
                    json.setString("value");
                    json.setFloat(v.floatValue);
                    break;
                case FormatString:
                    json.setString("value");
                    json.setString(v.stringValue);
                    break;
                case FormatTLV:
                    break;
                case FormatData:
                    break;
                default:
                    break;
            }
        }
    }
}

HAPStatus HKCharacteristic::setValue(const String& jsonValue) {
    if (!(permissions & PermissionPairedWrite)) {
        HKLOGERROR("[HKCharacteristic::setValue] Failed to set characteristic value (id=%d.%d, service=%s, type=%d): no write permission\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
        return HAPStatusReadOnly;
    }

    HKValue hkValue = HKValue();
    switch (format) {
        case FormatBool: {
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
                hkValue = HKValue(FormatBool, result);
                setter(hkValue);
            } else {
                hkValue = value;
                value.boolValue = result;
            }
            break;
        }
        case FormatUInt8:
        case FormatUInt16:
        case FormatUInt32:
        case FormatUInt64:
        case FormatInt: {
            uint64_t result = jsonValue.toInt();

            uint64_t checkMinValue = 0;
            uint64_t checkMaxValue = 0;
            switch (format) {
                case FormatUInt8: {
                    checkMinValue = 0;
                    checkMaxValue = 0xFF;
                    break;
                }
                case FormatUInt16:{
                    checkMinValue = 0;
                    checkMaxValue = 0xFFFF;
                    break;
                }
                case FormatUInt32: {
                    checkMinValue = 0;
                    checkMaxValue = 0xFFFFFFFF;
                    break;
                }
                case FormatUInt64: {
                    checkMinValue = 0;
                    checkMaxValue = 0xFFFFFFFFFFFFFFFF;
                    break;
                }
                case FormatInt: {
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

            HKLOGINFO("[HKCharacteristic::setValue] Update Characteristic (id=%d.%d, service=%s, type=%d) with int: %d\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, result);

            if (setter) {
                hkValue = HKValue(format, result);
                setter(hkValue);
            } else {
                hkValue = value;
                value.intValue = result;
            }
            break;
        }
        case FormatFloat: {
            float result = jsonValue.toFloat();

            if ((minValue && result < *minValue) || (maxValue && result > *maxValue)) {
                HKLOGERROR("[HKCharacteristic::setValue] Failed to update (id=%d.%d, service=%s, type=%d): float is not in range\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
                return HAPStatusInvalidValue;
            }

            HKLOGINFO("[HKCharacteristic::setValue] Update Characteristic (id=%d.%d, service=%s, type=%d) with float: %f\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, result);

            if (setter) {
                hkValue = HKValue(FormatFloat, result);
                setter(hkValue);
            } else {
                hkValue = value;
                value.floatValue = result;
            }
            break;
        }
        case FormatString: {
            const char *result = jsonValue.c_str();

            int checkMaxLen = maxLen ? *maxLen : 64;
            if (strlen(result) > checkMaxLen) {
                HKLOGERROR("[HKCharacteristic::setValue] Failed to update (id=%d.%d, service=%s, type=%d): String is too long\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
                return HAPStatusInvalidValue;
            }

            HKLOGINFO("[HKCharacteristic::setValue] Update Characteristic (id=%d.%d, service=%s, type=%d) with string: %s\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type, result);

            if (setter) {
                hkValue = HKValue(FormatString, result);
                setter(hkValue);
            } else {
                hkValue = value;
                value.stringValue = result;
            }
            break;
        }
        case FormatTLV: {
            HKLOGERROR("[HKCharacteristic::setValue] (id=%d.%d, service=%s, type=%d) TLV not supported yet\r\n", service->getAccessory()->getId(), id, service->getCharacteristic(HKCharacteristicName)->getValue().stringValue, type);
            break;
        }
        case FormatData: {
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

    if (!(permissions & PermissionNotify)) {
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

void HKCharacteristic::notify(const HKValue& newValue) {
    ChangeCallback *temp = callback;
    while (temp) {
        temp->function(temp->client, this, newValue);
        temp = temp->next;
    }
}

bool HKCharacteristic::hasCallbackEvent(HKClient *client) {
    ChangeCallback *temp = callback;
    while (temp) {
        if (temp->client == client) {
            return true;
        }
        temp = temp->next;
    }
    return false;
}

void HKCharacteristic::removeCallbackEvent(HKClient *client) {
    while (callback) {
        if (callback->client != client) {
            break;
        }

        ChangeCallback *temp = callback;
        callback = callback->next;
        delete temp;
    }

    if (!callback) {
        return;
    }

    ChangeCallback *temp = callback;
    while (temp->next) {
        if (temp->next->client == client) {
            ChangeCallback *next = temp->next;
            temp->next = next->next;
            delete next;
        } else {
            temp = temp->next;
        }
    }
}

void HKCharacteristic::addCallbackEvent(HKClient *client) {
    auto newCallback = new ChangeCallback();
    newCallback->client = client;
    newCallback->function = std::bind(&HKClient::scheduleEvent, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    if (callback) {
        ChangeCallback *temp = callback;
        if (temp->client == newCallback->client) {
            delete newCallback;
            return;
        }

        while (temp->next) {
            if (temp->client == newCallback->client) {
                delete newCallback;
                return;
            }
            temp = temp->next;
        }
        temp->next = newCallback;
    } else {
        callback = newCallback;
    }
}

