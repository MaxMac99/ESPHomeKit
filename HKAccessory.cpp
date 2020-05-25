//
// Created by Max Vissing on 2019-04-26.
//

#include "HKAccessory.h"


HKAccessory::HKAccessory(HKAccessoryCategory category) : id(1), category(category) {
}

void HKAccessory::addInfoService(const String& accName, const String& manufacturerName, const String& modelName, const String& serialNumber, const String &firmwareRevision) {
    auto *infoService = new HKService(HKServiceType::HKServiceAccessoryInfo);
    addService(infoService);

    HKValue identifyValue = HKValue(FormatBool);
    HKCharacteristic *identifyChar = new HKCharacteristic(HKCharacteristicIdentify, identifyValue, PermissionPairedWrite, "Identify", FormatBool);
    identifyChar->setSetter(std::bind(&HKAccessory::identify, this));
    infoService->addCharacteristic(identifyChar);

    HKValue manufacturerValue = HKValue(FormatString, manufacturerName);
    HKCharacteristic *manufacturerChar = new HKCharacteristic(HKCharacteristicManufactuer, manufacturerValue, PermissionPairedRead, "Manufacturer", FormatString);
    infoService->addCharacteristic(manufacturerChar);

    HKValue modelValue = HKValue(FormatString, modelName);
    HKCharacteristic *modelChar = new HKCharacteristic(HKCharacteristicModelName, modelValue, PermissionPairedRead, "Model", FormatString);
    infoService->addCharacteristic(modelChar);

    HKValue nameValue = HKValue(FormatString, accName);
    HKCharacteristic *nameChar = new HKCharacteristic(HKCharacteristicName, nameValue, PermissionPairedRead, "Name", FormatString);
    infoService->addCharacteristic(nameChar);

    HKValue serialValue = HKValue(FormatString, serialNumber);
    HKCharacteristic *serialChar = new HKCharacteristic(HKCharacteristicSerialNumber, serialValue, PermissionPairedRead, "Serial Number", FormatString);
    infoService->addCharacteristic(serialChar);
}

void HKAccessory::addService(HKService *service) {
    services.push_back(service);
    service->accessory = this;
}

HKService *HKAccessory::getService(HKServiceType serviceType) {
    auto value = std::find_if(services.begin(), services.end(), [serviceType](HKService * const& obj){
        return obj->getServiceType() == serviceType;
    });
    if (value != services.end()) {
        return *value;
    }
    return nullptr;
}

unsigned int HKAccessory::getId() const {
    return id;
}

HKAccessoryCategory HKAccessory::getCategory() const {
    return category;
}

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

HKCharacteristic *HKAccessory::findCharacteristic(unsigned int iid) {
    for (auto service : services) {
        if (HKCharacteristic *result = service->findCharacteristic(iid)) {
            return result;
        }
    }
    return nullptr;
}

void HKAccessory::clearCallbackEvents(HKClient *client) {
    for (auto service : services) {
        for (auto characteristic : service->characteristics) {
            characteristic->removeCallbackEvent(client);
        }
    }
}

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
