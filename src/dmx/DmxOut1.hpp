#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"
#include "DmxOut1Widget.hpp"

using namespace rack;

struct DmxOut1 : AbstractDmxModule {
    static const constexpr char* MODEL_SLUG = "DmxOut1";

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

    DmxOut1();

    const char* getModelSlug() const override;
};
