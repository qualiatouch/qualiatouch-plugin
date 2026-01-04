#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"
#include "SonicpulseWash10Widget.hpp"

using namespace std;

using namespace rack;

struct SonicpulseWash10 : AbstractDmxModule {
    static const constexpr char* MODEL_SLUG = "SonicpulseWash10";

    enum ParamId {
        BLACKOUT_BUTTON,
        PARAMS_LEN
    };

    enum InputId {
        INPUT_CHANNEL_PAN,
        INPUT_CHANNEL_TILT,
        INPUT_CHANNEL_SPEED,
        INPUT_CHANNEL_DIMMER,
        INPUT_CHANNEL_STROBE,
        INPUT_CHANNEL_RED,
        INPUT_CHANNEL_GREEN,
        INPUT_CHANNEL_BLUE,
        INPUT_CHANNEL_WHITE,
        INPUT_CHANNEL_FUNCTION,
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

    SonicpulseWash10();

    const char* getModelSlug() const override;
};

