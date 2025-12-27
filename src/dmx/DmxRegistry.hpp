#pragma once
#include "../plugin.hpp"
#include "DmxOut1.hpp"
#include <iostream>

using namespace std;

using namespace rack;

class DmxRegistry {

private:
    std::vector<DmxOut1*> modules;

    ola::DmxBuffer buffer;

    std::unique_ptr<ola::client::StreamingClient> ola_client = nullptr;

    unsigned int dmxUniverse = 1;

    bool debug = false;

    DmxRegistry();
    void initOla();

public:
    static DmxRegistry& instance();

    void registerModule(DmxOut1* module);

    void unregisterModule(DmxOut1* module);

    bool isMaster(int64_t id);
    void trigger(int64_t id);

    void sendDmx();
};
