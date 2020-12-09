/**
 * @file HKCharacteristic.h
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief 
 * @version 0.1
 * @date 2020-05-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef HAP_SERVER_HKCHARACTERISTIC_H
#define HAP_SERVER_HKCHARACTERISTIC_H

#define HKCHARACTERISTIC_CLASS_ID 0

#include <Arduino.h>
#include "JSON/JSON.h"
#include <ArduinoJson.h>
#include "HKClient.h"
#include "HKService.h"
#include "ESPHomeKit.h"

struct HKValidValues {
    int count;
    uint8_t *values;
};

struct HKValidValuesRange {
    uint8_t start;
    uint8_t end;
};

struct HKValidValuesRanges {
    int count;
    HKValidValuesRange *ranges;
};

class HKAccessory;
class HKService;
class HKClient;
class ESPHomeKit;

class HKCharacteristic {
public:
    HKCharacteristic(HKCharacteristicType type, const HKValue &value, uint8_t permissions,
                     String description, HKFormat format, HKUnit unit=HKUnitNone, float *minValue=nullptr, float *maxValue=nullptr, float *minStep=nullptr, uint *maxLen=nullptr, uint *maxDataLen=nullptr, HKValidValues validValues=HKValidValues(), HKValidValuesRanges validValuesRanges=HKValidValuesRanges());
    virtual ~HKCharacteristic();
    virtual uint getClassId() { return HKCHARACTERISTIC_CLASS_ID; };

    uint getId() const;
    HKCharacteristicType getType() const;
    const HKValue &getValue() const;
    HKService *getService();

    void setGetter(const std::function<const HKValue &()> &getter);
    void setSetter(const std::function<void(const HKValue)> &setter);
    void notify(const HKValue& newValue);
private:
    HAPStatus setValue(const String& jsonValue);
    HAPStatus setEvent(HKClient *client, const String& jsonValue);
    void addCallbackEvent(HKClient *client);
    void removeCallbackEvent(HKClient *client);
    bool hasCallbackEvent(HKClient *client);
    void serializeToJSON(JSON &json, HKValue *jsonValue, uint format = 0xF, HKClient *client = nullptr);

    friend HKAccessory;
    friend HKService;
    friend ESPHomeKit;
    friend HKClient;
private:
    uint id;
    HKService *service;

    HKCharacteristicType type;
    HKValue value;
    uint8_t permissions;
    String description;
    HKUnit unit;
    HKFormat format;

    float *minValue;
    float *maxValue;
    float *minStep;
    uint *maxLen;
    uint *maxDataLen;

    HKValidValues validValues;
    HKValidValuesRanges validValuesRanges;

    std::function<const HKValue &()> getter;
    std::function<void(const HKValue)> setter;

    std::vector<HKClient *> notifiers;
};

struct ClientEvent {
    HKCharacteristic *characteristic;
    HKValue value;
    ClientEvent *next;
};

class HKEvent {
public:
    inline HKEvent(HKCharacteristic *characteristic, HKValue value) : characteristic(characteristic), value(value) {};
    inline HKCharacteristic *getCharacteristic() { return characteristic; };
    inline HKValue *getValue() { return &value; };
private:
    HKCharacteristic *characteristic;
    HKValue value;
};


#endif //HAP_SERVER_HKCHARACTERISTIC_H
