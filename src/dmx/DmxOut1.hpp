#pragma once
#include "../plugin.hpp"
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

struct DmxOut1 : Module {
    enum ParamId {
        BLACKOUT_BUTTON,
        PARAMS_LEN
    };

    enum InputId {
        INPUT_CHANNEL_0,
        INPUT_BLACKOUT,
        INPUTS_LEN
    };

    enum LightIn {
        BLACKOUT_LIGHT,
        LIGHTS_LEN
    };

    enum OutputIds {
        OUTPUTS_LEN
    };

    std::string useOwnDmxAddressJsonKey = "useOwnDmxAddress";
    std::string dmxAddressJsonKey = "dmxAddress";

    // module params
	float timeSinceLastLoop = 0.f;
    int loop = 0;
    bool debug = false;
    bool debugChain = false;
    float sampleRate = 0.1f;

    // module chain
    bool isMaster = false;
    std::vector<DmxOut1*> moduleChain;
    int moduleIndex = 0;
    int moduleChainSize = 0;
    bool recalculateChain = true;

    // module working variables
    float input0;
    float clamped0;
    float dmx0;
    float dmxValue;

    // blackout button
    dsp::SchmittTrigger blackoutButtonTrigger;
    dsp::SchmittTrigger blackoutInputTrigger;
    bool blackoutTriggered = false;

    // DMX
    unsigned int dmxUniverse = 1;
    bool useOwnDmxAddress = false;
    unsigned int dmxAddress = 1;
    unsigned int dmxChannel = 1;
    bool updateDmxChannelDisplayWidget = false;

    ola::DmxBuffer buffer;

    std::unique_ptr<ola::client::StreamingClient> ola_client;

    DmxOut1();

    void onAdd() override;
    void refreshModuleChain();
    void onExpanderChange(const ExpanderChangeEvent &e) override;
    void toggleUseOwnDmxAddress();
    void process(const ProcessArgs& arg) override;
    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJson) override;
    bool isSameModel(Module* otherModule);
};
