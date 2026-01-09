#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"
#include "DmxOut4Widget.hpp"

using namespace rack;

struct DmxOut4 : AbstractDmxModule {
    static const constexpr char* MODEL_SLUG = "DmxOut4";

    enum ParamId {
        BLACKOUT_BUTTON,
        PARAMS_LEN
    };

    enum InputId {
        INPUT_CHANNEL_0,
        INPUT_CHANNEL_1,
        INPUT_CHANNEL_2,
        INPUT_CHANNEL_3,
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

    DmxOut4();

    const char* getModelSlug() const override;
};
