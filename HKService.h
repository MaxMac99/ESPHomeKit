/**
 * @file HKService.h
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief 
 * @version 0.1
 * @date 2020-05-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef HAP_SERVER_HKSERVICE_H
#define HAP_SERVER_HKSERVICE_H

#define HKSERVICE_CLASS_ID 0

#include <Arduino.h>
#include "JSON/JSON.h"
#include "HKDefinitions.h"
#include "HKAccessory.h"
#include "HKCharacteristic.h"
#include "HKClient.h"

class HKAccessory;
class HKCharacteristic;
class HKClient;

class HKService {
public:
    explicit HKService(HKServiceType type, bool hidden=false, bool primary=false, String name="");
    virtual ~HKService() = default;
    virtual uint getClassId() { return HKSERVICE_CLASS_ID; };
    
    void addLinkedService(HKService *service);
    void addCharacteristic(HKCharacteristic *characteristic);
    void setName(String name);
    String getName();

    HKServiceType getServiceType() const;
    HKAccessory *getAccessory();
    HKCharacteristic *getCharacteristic(HKCharacteristicType characteristicType);
    std::vector<HKCharacteristic *> getCharacteristics();
private:
    HKCharacteristic *findCharacteristic(uint iid);
    void serializeToJSON(JSON &json, HKValue *value, HKClient *client);

    friend HKAccessory;
private:
    uint id;
    HKAccessory *accessory;

    HKServiceType serviceType;
    bool hidden;
    bool primary;
    std::vector<HKService *> linkedServices;
    std::vector<HKCharacteristic *> characteristics;
};


#endif //HAP_SERVER_HKSERVICE_H
