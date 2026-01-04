#include "SonicpulseWash10.hpp"

SonicpulseWash10::SonicpulseWash10() : AbstractDmxModule(10) {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configInput(INPUT_CHANNEL_PAN, "Rotation/Pan (1)");
    configInput(INPUT_CHANNEL_TILT, "Inclination/Tilt (2)");
    configInput(INPUT_CHANNEL_SPEED, "Pan/tilt speed (3)");
    configInput(INPUT_CHANNEL_DIMMER, "Dimmer intensity (4)");
    configInput(INPUT_CHANNEL_STROBE, "Strobe (5)");
    configInput(INPUT_CHANNEL_RED, "Red intensity (6)");
    configInput(INPUT_CHANNEL_GREEN, "Green intensity (7)");
    configInput(INPUT_CHANNEL_BLUE, "Blue intensity (8)");
    configInput(INPUT_CHANNEL_WHITE, "White intensity (9)");
    configInput(INPUT_CHANNEL_FUNCTION, "Special functions (10)");

    configBlackout(BLACKOUT_LIGHT, BLACKOUT_BUTTON, INPUT_BLACKOUT);
}

const char* SonicpulseWash10::getModelSlug() const {
    return MODEL_SLUG;
}

Model* modelSonicpulseWash10 = createModel<SonicpulseWash10, SonicpulseWash10Widget>(SonicpulseWash10::MODEL_SLUG);
