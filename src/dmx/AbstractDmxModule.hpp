#pragma once
#include "../plugin.hpp"
#include "DmxRegistry.hpp"

#include <thread>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>

#include <stdlib.h>
#include <unistd.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/client/StreamingClient.h>

using namespace std;

using namespace rack;

struct AbstractDmxModule : rack::engine::Module {
    std::string useOwnDmxAddressJsonKey = "useOwnDmxAddress";
    std::string dmxAddressJsonKey = "dmxAddress";
    std::string dmxUniverseJsonKey = "dmxUniverse";

    // module params
	float timeSinceLastLoop = 0.f;
    int loop = 0;
    bool debug = false;
    bool debugChain = false;
    float sampleRate = 0.1f;

    // module chain
    std::vector<AbstractDmxModule*> moduleChain;
    int moduleIndex = 0;
    int moduleChainSize = 0;
    bool recalculateChain = true;

    // DMX
    bool useOwnDmxAddress = false;
    unsigned int dmxAddress = 1;
    unsigned int dmxChannel = 1;
    bool updateDmxChannelDisplayWidget = false;
    bool blackoutTriggered = false;

    AbstractDmxModule();

    bool isMaster();
    void onAdd() override;
    void onRemove() override;
    bool isLeftModuleDmx();
    void refreshModuleChain();
    void onExpanderChange(const ExpanderChangeEvent &e) override;
    void toggleUseOwnDmxAddress();
    int getDmxUniverse();
    void setDmxUniverse(int universe);

    virtual void process(const ProcessArgs& arg) override = 0;
    virtual bool isSameModel(Module* otherModule) = 0;
    virtual std::vector<std::pair<unsigned int, uint8_t>> getDmxChannelValues() = 0;

    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJson) override;
};
