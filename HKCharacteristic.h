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

#include <Arduino.h>
#include "JSON/JSON.h"
#include <ArduinoJson.h>
#include "HKClient.h"
#include "HKService.h"

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

class HKCharacteristic {
public:
    HKCharacteristic(HKCharacteristicType type, const HKValue &value, uint8_t permissions,
                     String description, HKFormat format, HKUnit unit=UnitNone, float *minValue=nullptr, float *maxValue=nullptr, float *minStep=nullptr, unsigned int *maxLen=nullptr, unsigned int *maxDataLen=nullptr, HKValidValues validValues=HKValidValues(), HKValidValuesRanges validValuesRanges=HKValidValuesRanges());

    virtual ~HKCharacteristic();

    unsigned int getId() const;
    HKCharacteristicType getType() const;
    const HKValue &getValue() const;

    void setGetter(const std::function<HKValue()> &getter);
    void setSetter(const std::function<void(const HKValue)> &setter);

    void serializeToJSON(JSON &json, HKValue *jsonValue, unsigned int format = 0xF, HKClient *client = nullptr);
    HAPStatus setValue(const String& jsonValue);
    HAPStatus setEvent(HKClient *client, const String& jsonValue);
    void addCallbackEvent(HKClient *client);
    void removeCallbackEvent(HKClient *client);
    bool hasCallbackEvent(HKClient *client);
    void notify(const HKValue& newValue);

    friend HKAccessory;
    friend HKService;
    friend HKClient;

    struct ChangeCallback {
        std::function<void(HKClient *, HKCharacteristic *, HKValue)> function;
        HKClient *client;
        ChangeCallback *next;
    };
private:
    unsigned int id;
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
    unsigned int *maxLen;
    unsigned int *maxDataLen;

    HKValidValues validValues;
    HKValidValuesRanges validValuesRanges;

    std::function<HKValue()> getter;
    std::function<void(const HKValue)> setter;

    ChangeCallback *callback;
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
    inline HKValue getValue() { return value; };
private:
    HKCharacteristic *characteristic;
    HKValue value;
};


#endif //HAP_SERVER_HKCHARACTERISTIC_H
