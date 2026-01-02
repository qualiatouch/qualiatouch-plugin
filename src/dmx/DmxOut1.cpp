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
}

bool DmxOut1::isSameModel(Module* otherModule) {
    return otherModule->model->plugin->name == "QualiaTouch"
        && otherModule->model->slug == "DmxOut1";
}

void DmxOut1::process(const ProcessArgs& args) {
    if (moduleChainSize < 1 || recalculateChain) {
        if (debugChain) {
            cout << "module " << getId() << " : we need to recalculate module chain (chainSize " << moduleChainSize << "; recalculate " << (recalculateChain ? "t" : "f") << ") calling refreshModuleChain()" << endl;
        }
        refreshModuleChain();
    }

    lights[BLACKOUT_LIGHT].setBrightness(blackoutTriggered ? 1.f : 0.f);

    if (blackoutButtonTrigger.process(params[BLACKOUT_BUTTON].getValue())
        || blackoutInputTrigger.process(inputs[INPUT_BLACKOUT].getVoltage())) {
        blackoutTriggered = true;
        return;
    }

    timeSinceLastLoop += args.sampleTime;
    if (timeSinceLastLoop < sampleRate) {
        return;
    }

    if (debug) {
        cout << "loop " << loop << "(" << args.sampleTime << ")" << " module " << moduleIndex << " - begin" << endl;
    }

    Input input = inputs[DmxOut1::INPUT_CHANNEL_0];
    
    if (input.isConnected()) {
        float voltage = input.getVoltage();
        float clamped = clamp(voltage, 0.f, 10.f);
        dmxValue = static_cast<uint8_t>(clamped * 255.f / 10.f);
        if (debug) {
            cout << "   module " << moduleIndex << " channel " << dmxChannel << " voltage " << voltage << " dmx " << dmxValue << endl;
        }
    }

    if (isMaster()) {
        DmxRegistry::instance().trigger(getId());
    }

    timeSinceLastLoop = 0.0f;
    loop++;
}

std::vector<std::pair<unsigned int, uint8_t>> DmxOut1::getDmxChannelValues() {
    std::vector<std::pair<unsigned int, uint8_t>> values;

    values.push_back({dmxChannel, dmxValue});

    return values;
}

Model* modelDmxOut1 = createModel<DmxOut1, DmxOut1Widget>("DmxOut1");
