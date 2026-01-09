#include "DmxOut4.hpp"
#include "DmxRegistry.hpp"

using namespace std;
using namespace rack;

DmxOut4::DmxOut4() : AbstractDmxModule(4) {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configDmxInputs();

    configBlackout(BLACKOUT_LIGHT, BLACKOUT_BUTTON, INPUT_BLACKOUT);
}

const char* DmxOut4::getModelSlug() const {
    return MODEL_SLUG;
}

Model* modelDmxOut4 = createModel<DmxOut4, DmxOut4Widget>(DmxOut4::MODEL_SLUG);
