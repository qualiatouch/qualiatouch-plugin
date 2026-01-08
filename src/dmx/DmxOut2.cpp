#include "DmxOut2.hpp"
#include "DmxRegistry.hpp"

using namespace std;
using namespace rack;

DmxOut2::DmxOut2() : AbstractDmxModule(2) {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configDmxInputs();

    configBlackout(BLACKOUT_LIGHT, BLACKOUT_BUTTON, INPUT_BLACKOUT);
}

const char* DmxOut2::getModelSlug() const {
    return MODEL_SLUG;
}

Model* modelDmxOut2 = createModel<DmxOut2, DmxOut2Widget>(DmxOut2::MODEL_SLUG);
