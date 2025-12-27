#include "DmxOut1.hpp"
#include "DmxRegistry.hpp"

using namespace std;

using namespace rack;

DmxOut1::DmxOut1() {
    if (debug) {
        cout << "[DMX] construct DmxOut1" << endl;
    }

    // # init Module
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configButton(BLACKOUT_BUTTON, "Blackout");
    configInput(INPUT_CHANNEL_0, "channel 0");
    configInput(INPUT_BLACKOUT, "Blackout (Trigger or gate to blackout)");
    configLight(BLACKOUT_LIGHT, "Blackout triggered - deactivate it in the menu");
}

bool DmxOut1::isMaster() {
    return DmxRegistry::instance().isMaster(getId());
}

void DmxOut1::onAdd() {
    recalculateChain = true;
    DmxRegistry::instance().registerModule(this);
}

void DmxOut1::onRemove() {
    DmxRegistry::instance().unregisterModule(this);
}

bool DmxOut1::isLeftModuleDmx() {
    Module* leftModule = getLeftExpander().module;
    if (leftModule == nullptr) {
        return false;
    }

    return isSameModel(leftModule);
}

void DmxOut1::refreshModuleChain() {
    if (debugChain) {
        cout << "module " << getId() << " in refreshModuleChain()" << endl;
    }
    recalculateChain = false;
    moduleChain.clear();

    // identification du premier module de la chaîne / sinon déclenchement de refreshModuleChain sur le module à gauche
    Module* leftModule = getLeftExpander().module;
    if (leftModule == nullptr || false == isSameModel(leftModule)) {
        useOwnDmxAddress = true;
        dmxAddress = dmxChannel;
        if (debugChain) {
            cout << "module " << getId() << " is master" << endl;
        }
    } else {
        // not master
        moduleChain.push_back(this);
        moduleChainSize = moduleChain.size();
        if (debugChain) {
            cout << "module " << getId() << " is NOT master" << endl;
        }
        Module* rightModule = getRightExpander().module;
        if (rightModule && rightModule->getId() == leftModule->getId()) {
            // if (debugChain) {
                cerr << "   module " << getId() << " : some swap occured (module " << leftModule->getId() << " on both sides) - stopping the loop" << endl;
            // }
            return;
        }
        if (debugChain) {
            cout << "   module " << getId() << " : calling refreshModuleChain() to the left (" << leftModule->getId() << ")" << endl;
        }
        DmxOut1* m = dynamic_cast<DmxOut1*>(leftModule);
        m->refreshModuleChain();
        return;
    }

    if (debugChain) {
        cout << "module " << getId() << " reconstructing chain" << endl;
    }

    DmxOut1* m = this;
    int i = 0;
    int address = dmxAddress;
    int relativeCounter = 0;

    // remplissage de la chaîne
    while (m && i < 20) { // todo const limit
        m->moduleIndex = i;
        if (m->useOwnDmxAddress) {
            address = m->dmxAddress;
            relativeCounter = 0;
        }
        m->dmxChannel = address + relativeCounter;
        m->updateDmxChannelDisplayWidget = true;
        if (debugChain) {
            cout << "       adding module " << m->moduleIndex << " channel " << m->dmxChannel << endl;
        }
        moduleChain.push_back(m);
        if (debugChain) {
            cout << "       getting right expander" << endl;
        }
        Module* rightModule = m->getRightExpander().module;
        if (debugChain) {
            cout << "   rightModule " << (rightModule ? rightModule->getId() : 0) << endl;
        }

        if (rightModule) {
            if (debugChain) {
                cout << "           rightModule : " << rightModule->model->slug;
            }
        } else {
            if (debugChain) {
                cout << "           no rightModule";
            }
        }
        if (rightModule && isSameModel(rightModule))
        {
            if (debugChain) {
                cout << " -> continue" << endl;
            }
            m = dynamic_cast<DmxOut1*>(rightModule);
        } else {
            if (debugChain) {
                cout << " -> end" << endl;
            }
            m = nullptr;
        }

        i++;
        relativeCounter++;
    }

    moduleChainSize = moduleChain.size();

    if (debugChain) {
        cout << "   module " << getId() << " chain has " << moduleChainSize << " modules" << endl;
    }
}

/**
 * (empirical) notes on onExpanderChange
 * when moving one module B right between two others A and C
 * ABC
 * 1. A onExpanderChange side 1 has B to the right
 * 2. B onExpanderChange side 0 has A to the left and nothing to the right
 * 3. B onExpanderChange side 1 has A to the left and C to the right
 * 4. C onExpanderChange side 0 has B to the left and nothing to the right
 *
 * il vaut mieux ne regarder le module d'un côté que si on vient de l'expanderchange de ce côté
 */
void DmxOut1::onExpanderChange(const ExpanderChangeEvent &e) {
    Module* rightModule = getRightExpander().module;
    Module* leftModule = getLeftExpander().module;

    if (debugChain) {
        cout << "module " << getId() << " onExpanderChange() side " << e.side
            << " left:" << (leftModule ? leftModule->getId() : 0)
            << " right: " << (rightModule ? rightModule->getId() : 0) << endl;
    }

    if (e.side == 1) {
        if (debugChain) {
            cout << "   change was to the right - resetting chain" << endl;
        }
        recalculateChain = true;
    } else {
        if (debugChain) {
            cout << "   change was to the left - resetting chain" << endl;
        }
        recalculateChain = true;
    }
}

bool DmxOut1::isSameModel(Module* otherModule) {
    return otherModule->model->plugin->name == "QualiaTouch"
        && otherModule->model->slug == "DmxOut1";
}

void DmxOut1::toggleUseOwnDmxAddress() {
    useOwnDmxAddress = !useOwnDmxAddress;
    recalculateChain = true;
    updateDmxChannelDisplayWidget = true;
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

int DmxOut1::getDmxUniverse() {
    return DmxRegistry::instance().getDmxUniverse();
}

void DmxOut1::setDmxUniverse(int universe) {
    DmxRegistry::instance().setDmxUniverse(universe);
}

json_t* DmxOut1::dataToJson() {
    json_t* rootJson = json_object();
    json_object_set_new(rootJson, dmxAddressJsonKey.c_str(), json_integer(dmxAddress));
    json_object_set_new(rootJson, useOwnDmxAddressJsonKey.c_str(), json_boolean(useOwnDmxAddress));

    return rootJson;
}

void DmxOut1::dataFromJson(json_t* rootJson)  {
    if (debug) {
        char* jsonStr = json_dumps(rootJson, JSON_INDENT(2));
        cout << "Loading JSON: " << jsonStr << endl;
        free(jsonStr);
    }

    json_t* useOwnDmxAddressParamJson = json_object_get(rootJson, useOwnDmxAddressJsonKey.c_str());
    if (useOwnDmxAddressParamJson) {
        useOwnDmxAddress = json_boolean_value(useOwnDmxAddressParamJson);
    }

    json_t* dmxAddressParamJson = json_object_get(rootJson, dmxAddressJsonKey.c_str());
    if (dmxAddressParamJson) {
        dmxAddress = json_integer_value(dmxAddressParamJson);
    }
}

Model* modelDmxOut1 = createModel<DmxOut1, DmxOut1Widget>("DmxOut1");
