#include "SonicpulseWash10.hpp"

SonicpulseWash10::SonicpulseWash10() : AbstractDmxModule(10) {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configDmxInputs();

    configBlackout(BLACKOUT_LIGHT, BLACKOUT_BUTTON, INPUT_BLACKOUT);
}

const vector<std::string> SonicpulseWash10::getDmxInputsNames() const {
    vector<std::string> names = {
        "Rotation/Pan ",
        "Inclination/Tilt ",
        "Pan/tilt speed ",
        "Dimmer intensity ",
        "Strobe ",
        "Red intensity ",
        "Green intensity ",
        "Blue intensity ",
        "White intensity ",
        "Special functions "
    };
    return names;
}

const char* SonicpulseWash10::getModelSlug() const {
    return MODEL_SLUG;
}

Model* modelSonicpulseWash10 = createModel<SonicpulseWash10, SonicpulseWash10Widget>(SonicpulseWash10::MODEL_SLUG);
