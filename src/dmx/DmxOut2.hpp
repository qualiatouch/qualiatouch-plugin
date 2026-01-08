#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"
#include "DmxOut2Widget.hpp"

using namespace rack;

struct DmxOut2 : AbstractDmxModule {
    static const constexpr char* MODEL_SLUG = "DmxOut2";

    enum ParamId {
        BLACKOUT_BUTTON,
        PARAMS_LEN
    };

    enum InputId {
        INPUT_CHANNEL_0,
        INPUT_CHANNEL_1,
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

    DmxOut2();

    const char* getModelSlug() const override;
};
