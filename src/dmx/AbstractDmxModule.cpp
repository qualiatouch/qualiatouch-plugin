#include "AbstractDmxModule.hpp"

AbstractDmxModule::AbstractDmxModule() {}

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
    cout << "dataToJson() useOwnDmxAddress " << useOwnDmxAddress << endl;
    json_t* rootJson = json_object();
    json_object_set_new(rootJson, dmxAddressJsonKey.c_str(), json_integer(dmxAddress));
    json_object_set_new(rootJson, useOwnDmxAddressJsonKey.c_str(), json_boolean(useOwnDmxAddress));
    json_object_set_new(rootJson, dmxUniverseJsonKey.c_str(), json_integer(getDmxUniverse()));

    char* jsonStr = json_dumps(rootJson, JSON_INDENT(2));
    cout << "   saving JSON: " << jsonStr << endl;
    free(jsonStr);

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
    }

    json_t* dmxUniverseParamJson = json_object_get(rootJson, dmxUniverseJsonKey.c_str());
    if (dmxUniverseParamJson) {
        DmxRegistry::instance().setDmxUniverse(json_integer_value(dmxUniverseParamJson));
    }
}
