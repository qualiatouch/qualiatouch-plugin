#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"
#include "DmxOut1Widget.hpp"

using namespace rack;

struct DmxOut1 : AbstractDmxModule {
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

    // module working variables
    float input0;
    float clamped0;
    float dmx0;
    uint8_t dmxValue;

    // blackout button
    dsp::SchmittTrigger blackoutButtonTrigger;
    dsp::SchmittTrigger blackoutInputTrigger;

    DmxOut1();

    void process(const ProcessArgs& arg) override;

    std::vector<std::pair<unsigned int, uint8_t>> getDmxChannelValues() override;

    bool isSameModel(Module* otherModule) override;
};
