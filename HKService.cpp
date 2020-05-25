//
// Created by Max Vissing on 2019-04-26.
//

#include "HKService.h"

HKService::HKService(HKServiceType type, bool hidden, bool primary) : id(0), accessory(nullptr), serviceType(type), hidden(hidden), primary(primary) {

}

void HKService::addCharacteristic(HKCharacteristic *characteristic) {
    characteristics.push_back(characteristic);
    characteristic->service = this;
}

HKCharacteristic *HKService::getCharacteristic(HKCharacteristicType characteristicType) {
    auto value = std::find_if(characteristics.begin(), characteristics.end(), [characteristicType](HKCharacteristic * const& obj) {
        return obj->getType() == characteristicType;
    });
    if (value != characteristics.end()) {
        return *value;
    }
    return nullptr;
}

HKServiceType HKService::getServiceType() const {
    return serviceType;
}

void HKService::serializeToJSON(JSON &json, HKValue *value, HKClient *client) {
    json.setString("iid");
    json.setInt(id);

    json.setString("type");
    String typeConv = String(serviceType, HEX);
    typeConv.toUpperCase();
    json.setString(typeConv.c_str());

    json.setString("hidden");
    json.setBool(hidden);

    json.setString("primary");
    json.setBool(primary);

    if (!linkedServices.empty()) {
        json.setString("linkedServices");
        json.startArray();

        for (auto link : linkedServices) {
            json.setInt(link->id);
        }

        json.endArray();
    }

    json.setString("characteristics");
    json.startArray();

    for (auto characteristic : characteristics) {
        json.startObject();
        characteristic->serializeToJSON(json, value, 0xF, client);
        json.endObject();
    }

    json.endArray();
}

HKCharacteristic *HKService::findCharacteristic(unsigned int iid) {
    for (auto ch : characteristics) {
        if (ch->getId() == iid) {
            return ch;
        }
    }
    return nullptr;
}

void HKService::addLinkedService(HKService *service) {
    linkedServices.push_back(service);
}

HKAccessory *HKService::getAccessory() {
    return accessory;
}
