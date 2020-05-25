/**
 * @file HKService.cpp
 * @author Max Vissing (max_vissing@yahoo.de)
 * @brief 
 * @version 0.1
 * @date 2020-05-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "HKService.h"

/**
 * @brief Construct a new HKService::HKService object
 * 
 * @param type Service type (See Apples HAP Documentation)
 * @param hidden Is the service hidden
 * @param primary Is this the primary service
 */
HKService::HKService(HKServiceType type, bool hidden, bool primary) : id(0), accessory(nullptr), serviceType(type), hidden(hidden), primary(primary) {

}

/**
 * @brief Add characteristic to service
 * 
 * @param characteristic 
 */
void HKService::addCharacteristic(HKCharacteristic *characteristic) {
    characteristics.push_back(characteristic);
    characteristic->service = this;
}

/**
 * @brief Find characteristic with given type
 * 
 * @param characteristicType characteristic type to search for
 * @return HKCharacteristic* first characteristic or nullptr otherwise
 */
HKCharacteristic *HKService::getCharacteristic(HKCharacteristicType characteristicType) {
    auto value = std::find_if(characteristics.begin(), characteristics.end(), [characteristicType](HKCharacteristic * const& obj) {
        return obj->getType() == characteristicType;
    });
    if (value != characteristics.end()) {
        return *value;
    }
    return nullptr;
}

/**
 * @brief Get the service type
 * 
 * @return HKServiceType 
 */
HKServiceType HKService::getServiceType() const {
    return serviceType;
}

/**
 * @brief Serialize the service and add it to json
 * 
 * @param json 
 * @param value 
 * @param client 
 */
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

/**
 * @brief Find characteristic via id from all characteristics
 * 
 * @param iid Characteristic ID to search for
 * @return HKCharacteristic* characteristic or nullptr
 */
HKCharacteristic *HKService::findCharacteristic(unsigned int iid) {
    for (auto ch : characteristics) {
        if (ch->getId() == iid) {
            return ch;
        }
    }
    return nullptr;
}

/**
 * @brief Link a service to this service
 * 
 * @param service 
 */
void HKService::addLinkedService(HKService *service) {
    linkedServices.push_back(service);
}

/**
 * @brief Get parent accessory
 * 
 * @return HKAccessory* 
 */
HKAccessory *HKService::getAccessory() {
    return accessory;
}
