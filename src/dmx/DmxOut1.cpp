#include "DmxOut1.hpp"
#include "DmxRegistry.hpp"

using namespace std;

using namespace rack;

DmxOut1::DmxOut1() {
    if (debug) {
        cout << "[DMX] construct DmxOut1" << endl;
    }

    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configButton(BLACKOUT_BUTTON, "Blackout");
    configInput(INPUT_CHANNEL_0, "channel 0");
    configInput(INPUT_BLACKOUT, "Blackout (Trigger or gate to blackout)");
    configLight(BLACKOUT_LIGHT, "Blackout triggered - deactivate it in the menu");

    configBlackout(BLACKOUT_LIGHT, BLACKOUT_BUTTON, INPUT_BLACKOUT);

    nbDmxInputs = 1;

    channelsValues.resize(nbDmxInputs);
    for (int i = 0; i < nbDmxInputs; i++) {
        channelsValues[i] = {0, 0};
    }
    
}

bool DmxOut1::isSameModel(Module* otherModule) {
    return otherModule->model->plugin->name == "QualiaTouch"
        && otherModule->model->slug == "DmxOut1";
}

Model* modelDmxOut1 = createModel<DmxOut1, DmxOut1Widget>("DmxOut1");
