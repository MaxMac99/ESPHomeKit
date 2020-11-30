/**
 * @file HKAccessory.h
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief 
 * @version 0.1
 * @date 2020-05-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef HAP_SERVER_HKACCESSORY_H
#define HAP_SERVER_HKACCESSORY_H

#define HKACCESSORY_CLASS_ID 0

#include <Arduino.h>
#include "JSON/JSON.h"
#include "HKClient.h"
#include "HKDefinitions.h"
#include "HKService.h"
#include "HKCharacteristic.h"
#include "HKServer.h"
#include "ESPHomeKit.h"

class HKService;
class HKCharacteristic;
class HKClient;
class HKServer;
class ESPHomeKit;

/**
 * @brief Override HKAccessory
 * 
 */
class HKAccessory {
public:
    explicit HKAccessory(HKAccessoryCategory category=HKAccessoryOther);
    explicit HKAccessory(const String &accessoryName, const String &modelName, const String &firmwareRevision, HKAccessoryCategory category=HKAccessoryOther);
    virtual ~HKAccessory() = default;
    virtual uint getClassId() { return HKACCESSORY_CLASS_ID; };

    virtual void identify() {};
    virtual void run() = 0;
    virtual void setup() = 0;

    void addInfoService(const String& accName, const String& manufacturerName, const String& modelName, const String& serialNumber, const String &firmwareRevision);
    void addService(HKService *service);

    HKService *getService(HKServiceType serviceType);
    std::vector<HKService *> getServices();
    HKAccessoryCategory getCategory() const;
    uint getId() const;
private:
    HKCharacteristic *findCharacteristic(uint iid);
    void prepareIDs();
    void clearCallbackEvents(HKClient *client);
    void serializeToJSON(JSON &json, HKValue *value, HKClient *client = nullptr);
    
    friend HKServer;
    friend HKClient;
    friend ESPHomeKit;
private:
    uint id;
    HKAccessoryCategory category;
    std::vector<HKService *> services;
};


#endif //HAP_SERVER_HKACCESSORY_H
