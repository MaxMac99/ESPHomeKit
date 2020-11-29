//
// Created by Max Vissing on 2019-04-26.
//

#include "HKAccessory.h"

/**
 * @brief Construct a new HKAccessory::HKAccessory object
 * 
 * @param category Look up your Category in HKDefinitions.h or Apples HAP Documentation
 */
HKAccessory::HKAccessory(HKAccessoryCategory category) : id(1), category(category) {
}

/**
 * @brief Construct a new HKAccessory::HKAccessory object
 * 
 * @param accessoryName Sets name of the Accessory Info Service
 * @param modelName Sets model name of the Accessory Info Service
 * @param firmwareRevision Sets firmware revision of the Accessory Info SErvice
 * @param category Look up your Category in HKDefinitions.h or Apples HAP Documentation
 */
HKAccessory::HKAccessory(const String &accessoryName, const String &modelName, const String &firmwareRevision, HKAccessoryCategory category) : id(1), category(category) {
    addInfoService(accessoryName, "MaxMac Co.", modelName, String(ESP.getChipId()), firmwareRevision);
}

/**
 * @brief Add the Info Service to your accessory. For more information have a look at Apples HAP Documentation
 * 
 * @param accName Accessory name
 * @param manufacturerName name of the manufacturer
 * @param modelName Model name
 * @param serialNumber Serial number
 * @param firmwareRevision Firmware revision
 */
void HKAccessory::addInfoService(const String& accName, const String& manufacturerName, const String& modelName, const String& serialNumber, const String &firmwareRevision) {
    auto *infoService = new HKService(HKServiceType::HKServiceAccessoryInfo);
    addService(infoService);

    HKValue identifyValue = HKValue(FormatBool);
    HKCharacteristic *identifyChar = new HKCharacteristic(HKCharacteristicIdentify, identifyValue, PermissionPairedWrite, "Identify", FormatBool);
    identifyChar->setSetter(std::bind(&HKAccessory::identify, this));
    infoService->addCharacteristic(identifyChar);

    HKValue manufacturerValue = HKValue(FormatString, "MaxMac Co.");
    HKCharacteristic *manufacturerChar = new HKCharacteristic(HKCharacteristicManufactuer, manufacturerValue, PermissionPairedRead, "Manufacturer", FormatString);
    infoService->addCharacteristic(manufacturerChar);

    HKValue modelValue = HKValue(FormatString, modelName);
    HKCharacteristic *modelChar = new HKCharacteristic(HKCharacteristicModelName, modelValue, PermissionPairedRead, "Model", FormatString);
    infoService->addCharacteristic(modelChar);

    HKValue nameValue = HKValue(FormatString, accName);
    HKCharacteristic *nameChar = new HKCharacteristic(HKCharacteristicName, nameValue, PermissionPairedRead, "Name", FormatString);
    infoService->addCharacteristic(nameChar);

    HKValue serialValue = HKValue(FormatString, String(ESP.getChipId()));
    HKCharacteristic *serialChar = new HKCharacteristic(HKCharacteristicSerialNumber, serialValue, PermissionPairedRead, "Serial Number", FormatString);
    infoService->addCharacteristic(serialChar);

    HKValue firmwareValue = HKValue(FormatString, firmwareRevision);
    HKCharacteristic *firmwareChar = new HKCharacteristic(HKCharacteristicFirmwareRevision, firmwareValue, PermissionPairedRead, "Firmware Revision", FormatString);
    infoService->addCharacteristic(firmwareChar);
}

/**
 * @brief Adds service to accessory
 * 
 * @param service 
 */
void HKAccessory::addService(HKService *service) {
    services.push_back(service);
    service->accessory = this;
}

/**
 * @brief Find service with given type
 * 
 * @param serviceType service type to search for
 * @return HKService* first service found or nullptr otherwise
 */
HKService *HKAccessory::getService(HKServiceType serviceType) {
    auto value = std::find_if(services.begin(), services.end(), [serviceType](HKService * const& obj){
        return obj->getServiceType() == serviceType;
    });
    if (value != services.end()) {
        return *value;
    }
    return nullptr;
}

/**
 * @brief Get the id
 * 
 * @return unsigned int id
 */
unsigned int HKAccessory::getId() const {
    return id;
}

/**
 * @brief Get the category
 * 
 * @return HKAccessoryCategory 
 */
HKAccessoryCategory HKAccessory::getCategory() const {
    return category;
}

/**
 * @brief Serialize the accessory and add it to json
 * 
 * @param json 
 * @param value 
 * @param client 
 */
void HKAccessory::serializeToJSON(JSON &json, HKValue *value, HKClient *client) {
    json.startObject();

    json.setString("aid");
    json.setInt(id);

    json.setString("services");
    json.startArray();

    for (auto service : services) {
        json.startObject();
        service->serializeToJSON(json, value, client);
        json.endObject();
    }

    json.endArray();

    json.endObject();
}

/**
 * @brief Find characteristic via id from all services
 * 
 * @param iid Characteristic ID to search for
 * @return HKCharacteristic* characteristic or nullptr
 */
HKCharacteristic *HKAccessory::findCharacteristic(unsigned int iid) {
    for (auto service : services) {
        if (HKCharacteristic *result = service->findCharacteristic(iid)) {
            return result;
        }
    }
    return nullptr;
}

/**
 * @brief Clear all callback events
 * 
 * @param client 
 */
void HKAccessory::clearCallbackEvents(HKClient *client) {
    for (auto service : services) {
        for (auto characteristic : service->characteristics) {
            characteristic->removeCallbackEvent(client);
        }
    }
}

/**
 * @brief Initialize IDs during setup
 * 
 */
void HKAccessory::prepareIDs() {
    setup();

    unsigned int iid = 1;
    for (auto service: services) {
        service->id = iid++;
        for (auto characteristic : service->characteristics) {
            characteristic->id = iid++;
        }
    }
}
