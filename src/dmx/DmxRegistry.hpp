#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/client/StreamingClient.h>

using namespace std;

using namespace rack;

struct AbstractDmxModule;

class DmxRegistry {

private:
    std::vector<AbstractDmxModule*> modules;

    ola::DmxBuffer buffer;

    std::unique_ptr<ola::client::StreamingClient> ola_client = nullptr;

    unsigned int dmxUniverse = 1;

    bool debug = false;

    DmxRegistry();
    void initOla();

public:
    static DmxRegistry& instance();

    void registerModule(AbstractDmxModule* module);

    void unregisterModule(AbstractDmxModule* module);

    bool isMaster(int64_t id);
    void trigger(int64_t id);

    void setDmxUniverse(int u);
    int getDmxUniverse();

    void sendDmx();
};
