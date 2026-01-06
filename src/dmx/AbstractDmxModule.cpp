#include "AbstractDmxModule.hpp"

AbstractDmxModule::AbstractDmxModule(int nbInputs) {
    nbDmxInputs = nbInputs;
    channelsValues.resize(nbDmxInputs);
    for (int i = 0; i < nbDmxInputs; i++) {
        channelsValues[i] = {0, 0};
    }
}

void AbstractDmxModule::configBlackout(uint8_t lightId, uint8_t buttonId, uint8_t inputId) {
    blackoutLightId = lightId;
    blackoutButtonId = buttonId;
    blackoutInputId = inputId;

    configLight(blackoutLightId, "Blackout triggered - deactivate it in the menu");
    configButton(blackoutButtonId, "Blackout");
    configInput(blackoutInputId, "Blackout (Trigger or gate to blackout)");
}

void AbstractDmxModule::onAdd() {
    recalculateChain = true;
    DmxRegistry::instance().registerModule(this);
}

void AbstractDmxModule::onRemove() {
    DmxRegistry::instance().unregisterModule(this);
}

bool AbstractDmxModule::isMaster() {
    return DmxRegistry::instance().isMaster(getId());
}

bool AbstractDmxModule::isLeftModuleDmx() {
    Module* leftModule = getLeftExpander().module;
    if (leftModule == nullptr) {
        return false;
    }

    return isSameModel(leftModule);
}

void AbstractDmxModule::refreshModuleChain() {
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
            cout << "module " << getId() << " is first of chain" << endl;
        }
    } else {
        // not master
        moduleChain.push_back(this);
        moduleChainSize = moduleChain.size();
        if (debugChain) {
            cout << "module " << getId() << " is NOT first of chain" << endl;
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
        AbstractDmxModule* m = dynamic_cast<AbstractDmxModule*>(leftModule);
        m->refreshModuleChain();
        return;
    }

    if (debugChain) {
        cout << "module " << getId() << " reconstructing chain" << endl;
    }

    AbstractDmxModule* m = this;
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

        m->channelsValues[0].first = m->dmxChannel;
        if (debugChain) {
            cout << "   (i=" << i << ") channelsValues[0].first = " << channelsValues[0].first << endl;
        }

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
            m = dynamic_cast<AbstractDmxModule*>(rightModule);
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
void AbstractDmxModule::onExpanderChange(const ExpanderChangeEvent &e) {
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

void AbstractDmxModule::toggleUseOwnDmxAddress() {
    useOwnDmxAddress = !useOwnDmxAddress;
    recalculateChain = true;
    updateDmxChannelDisplayWidget = true;
}

int AbstractDmxModule::getDmxUniverse() {
    return DmxRegistry::instance().getDmxUniverse();
}

void AbstractDmxModule::setDmxUniverse(int universe) {
    DmxRegistry::instance().setDmxUniverse(universe);
}

json_t* AbstractDmxModule::dataToJson() {
    json_t* rootJson = json_object();
    json_object_set_new(rootJson, dmxAddressJsonKey.c_str(), json_integer(dmxAddress));
    json_object_set_new(rootJson, useOwnDmxAddressJsonKey.c_str(), json_boolean(useOwnDmxAddress));
    json_object_set_new(rootJson, dmxUniverseJsonKey.c_str(), json_integer(getDmxUniverse()));

    return rootJson;
}

void AbstractDmxModule::dataFromJson(json_t* rootJson)  {
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
        dmxChannel = dmxAddress;
    }

    json_t* dmxUniverseParamJson = json_object_get(rootJson, dmxUniverseJsonKey.c_str());
    if (dmxUniverseParamJson) {
        DmxRegistry::instance().setDmxUniverse(json_integer_value(dmxUniverseParamJson));
    }
}

bool AbstractDmxModule::isSameModel(Module* otherModule) const {
    return otherModule->model->plugin->name == "QualiaTouch"
        && otherModule->model->slug == getModelSlug();
}

void AbstractDmxModule::updateInputsLabels() {
    for (int i = 0; i < nbDmxInputs; i++) {
        configInput(i, "channel " + to_string(channelsValues[i].first));
    }
}

void AbstractDmxModule::process(const ProcessArgs& args) {
    if (moduleChainSize < 1 || recalculateChain) {
        if (debugChain) {
            cout << "module " << getId() << " : we need to recalculate module chain (chainSize " << moduleChainSize << "; recalculate " << (recalculateChain ? "t" : "f") << ") calling refreshModuleChain()" << endl;
        }
        refreshModuleChain();
    }

    lights[blackoutLightId].setBrightness(blackoutTriggered ? 1.f : 0.f);

    if (blackoutButtonTrigger.process(params[blackoutButtonId].getValue())
        || blackoutInputTrigger.process(inputs[blackoutInputId].getVoltage())) {
        blackoutTriggered = true;
        return;
    }

    timeSinceLastLoop += args.sampleTime;
    if (timeSinceLastLoop < processPeriod) {
        return;
    }

    if (debug) {
        //cout << "loop " << loop << "(" << timeSinceLastLoop << ")" << " module " << moduleIndex << " - begin" << endl;
    }

    for (int i = 0; i < nbDmxInputs; i++) {
        if (debug) {
            cout << "244 module " << getId() << " input " << i;
        }
        if (inputs[i].isConnected()) {
            float voltage = inputs[i].getVoltage();
            float clamped = clamp(voltage, 0.f, 10.f);
            uint8_t dmxValue = static_cast<uint8_t>(clamped * 255.f / 10.f);
            if (debug) {
                cout << " : " << i + 1 << " -> " << (int) dmxValue << endl;
            }

            channelsValues[i].second = dmxValue;
        } else {
            if (debug) {
                cout << " : not connected" << endl;
            }
        }
    }

    if (isMaster()) {
        DmxRegistry::instance().trigger(getId());
    }

    timeSinceLastLoop = 0.0f;
    loop++;
}


int AbstractDmxModule::getModuleChainSize() {
    return moduleChainSize;
}

unsigned int AbstractDmxModule::getDmxAddress() {
    return dmxAddress;
}

void AbstractDmxModule::setDmxAddress(int address) {
    dmxAddress = address;
}

unsigned int AbstractDmxModule::getDmxChannel() {
    return dmxChannel;
}

void AbstractDmxModule::setDmxChannel(unsigned int channel) {
    dmxChannel = channel;
}

bool AbstractDmxModule::getUseOwnDmxAddress() {
    return useOwnDmxAddress;
}

void AbstractDmxModule::setRecalculateChain(bool recalculate) {
    recalculateChain = recalculate;
}

vector<pair<unsigned int, uint8_t>> AbstractDmxModule::getChannelsValues() {
    return channelsValues;
}
