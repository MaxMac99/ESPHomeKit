//
// Created by Max Vissing on 2019-04-26.
//

#ifndef HAP_SERVER_HKSERVICE_H
#define HAP_SERVER_HKSERVICE_H

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
    explicit HKService(HKServiceType type, bool hidden=false, bool primary=false);
    void addLinkedService(HKService *service);
    void addCharacteristic(HKCharacteristic *characteristic);

    HKServiceType getServiceType() const;
    HKAccessory *getAccessory();
    HKCharacteristic *getCharacteristic(HKCharacteristicType characteristicType);
    HKCharacteristic *findCharacteristic(unsigned int iid);

    void serializeToJSON(JSON &json, HKValue *value, HKClient *client);

    friend HKAccessory;
private:
    unsigned int id;
    HKAccessory *accessory;

    HKServiceType serviceType;
    bool hidden;
    bool primary;
    std::vector<HKService *> linkedServices;
    std::vector<HKCharacteristic *> characteristics;
};


#endif //HAP_SERVER_HKSERVICE_H
