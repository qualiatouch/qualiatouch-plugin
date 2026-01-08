#include "DmxOut1.hpp"
#include "DmxRegistry.hpp"

using namespace std;

using namespace rack;

DmxOut1::DmxOut1() : AbstractDmxModule(1) {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configDmxInputs();

    configBlackout(BLACKOUT_LIGHT, BLACKOUT_BUTTON, INPUT_BLACKOUT);
}

const char* DmxOut1::getModelSlug() const {
    return MODEL_SLUG;
}

Model* modelDmxOut1 = createModel<DmxOut1, DmxOut1Widget>(DmxOut1::MODEL_SLUG);
