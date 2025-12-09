#include "plugin.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>

#include <stdlib.h>
#include <unistd.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/client/StreamingClient.h>

using namespace std;

using namespace rack;

struct DmxOut1 : Module {
    enum ParamId {
        BLACKOUT_BUTTON,
        PARAMS_LEN
    };

    enum InputId {
        INPUT_CHANNEL_0,
        INPUTS_LEN
    };

    enum LightIn {
        LIGHTS_LEN
    };

    enum OutputIds {
        OUTPUTS_LEN
    };

    std::string dmxAddressJsonKey = "dmxAddress";

    // module params
	float timeSinceLastLoop = 0.f;
    int loop = 0;
    bool debug = false;
    float sampleRate = 0.1f;

    // module chain
    bool isMaster = false;
    std::vector<DmxOut1*> moduleChain;
    int moduleIndex = 0;
    int moduleChainSize = 0;
    bool recalculateChain = true;

    // module working variables
    float input0;
    float clamped0;
    float dmx0;
    float dmxValue;

    // blackout button
    dsp::SchmittTrigger blackoutTrigger;
    bool blackoutTriggered = false;

    // DMX
    unsigned int dmxUniverse = 1;
    unsigned int dmxAddress = 1;
    unsigned int dmxChannel = 1;

    ola::DmxBuffer buffer;

    std::unique_ptr<ola::client::StreamingClient> ola_client;

    DmxOut1() {
        if (debug) {
            cout << "[DMX] construct DmxOut1" << endl;
        }

        // # init Module
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configButton(BLACKOUT_BUTTON, "Blackout");
        configInput(INPUT_CHANNEL_0, "channel 0");

        // # init OLA / DMX

        if (debug) {
            cout << "OLA : init logging" << endl;
        }
        ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
        

        if (debug) {
            cout << "OLA : init client" << endl;
        }
        ola_client = std::unique_ptr<ola::client::StreamingClient>(new ola::client::StreamingClient());

        if (debug) {
            cout << "OLA : setup client" << endl;
        }
        if (!ola_client->Setup()) {
            std::cerr << "Setup failed" << endl;
        }

        if (debug) {
            cout << "OLA : blackout" << endl;
        }
        buffer.Blackout();
    }

    void refreshModuleChain();
    void onExpanderChange(const ExpanderChangeEvent &e) override;

    void process(const ProcessArgs& arg) override;

    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJson) override;
    bool isSameModel(Module* otherModule);
};

void DmxOut1::refreshModuleChain() {
    cout << "module " << getId() << " in refreshModuleChain()" << endl;
    recalculateChain = false;
    isMaster = false;
    moduleChain.clear();

    // todo improve all of that, for non-master
    Module* leftModule = getLeftExpander().module;
    if (leftModule == nullptr || false == isSameModel(leftModule)) {
        isMaster = true;
        cout << "module " << getId() << " is master" << endl;
    } else {
        // not master
        moduleChain.push_back(this);
        moduleChainSize = moduleChain.size();
        cout << "module " << getId() << " is NOT master" << endl;
        Module* rightModule = getRightExpander().module;
        if (rightModule && rightModule->getId() == leftModule->getId()) {
            cout << "   module " << getId() << " : some swap occured (module " << leftModule->getId() << " on both sides) - stopping the loop" << endl;
            return;
        }
        cout << "   module " << getId() << " : calling refreshModuleChain() to the left (" << leftModule->getId() << ")" << endl;
        DmxOut1* m = dynamic_cast<DmxOut1*>(leftModule);
        m->refreshModuleChain();
        return;
    }

    cout << "module " << getId() << " reconstructing chain" << endl;

    DmxOut1* m = this;
    int i = 0;

    while (m && i < 20) { // todo const limit
        m->moduleIndex = i;
        m->dmxChannel = dmxAddress + i;
        cout << "       adding module " << m->moduleIndex << " channel " << m->dmxChannel << endl;
        moduleChain.push_back(m);
        cout << "       getting right expander" << endl;
        Module* rightModule = m->getRightExpander().module;
        cout << "   145 rightModule " << (rightModule ? rightModule->getId() : 0) << endl;
        if (rightModule) {
            cout << "           rightModule : " << rightModule->model->slug;
        } else {
            cout << "           no rightModule";
        }
        if (rightModule && isSameModel(rightModule))
        {
            cout << " -> continue" << endl;
            m = dynamic_cast<DmxOut1*>(rightModule);
        } else {
            cout << " -> end" << endl;
            m = nullptr;
        }

        i++;
    }

    moduleChainSize = moduleChain.size();
    cout << "   module " << getId() << " chain has " << moduleChainSize << " modules" << endl;
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
void DmxOut1::onExpanderChange (const ExpanderChangeEvent &e) {
    Module* rightModule = getRightExpander().module;
    Module* leftModule = getLeftExpander().module;
    cout << "module " << getId() << " onExpanderChange() side " << e.side
        << " left:" << (leftModule ? leftModule->getId() : 0)
        << " right: " << (rightModule ? rightModule->getId() : 0) << endl;

    if (e.side == 1) {
        cout << "   change was to the right - resetting chain" << endl;
        recalculateChain = true;
    } else {
        cout << "   change was to the left - resetting chain" << endl;
        recalculateChain = true;
    }
}

bool DmxOut1::isSameModel(Module* otherModule) {
    return otherModule->model->plugin->name == "QualiaTouch"
        && otherModule->model->slug == "DmxOut1";
}

void DmxOut1::process(const ProcessArgs& args) {
    // cout << "> module " << moduleIndex << " 180 " << endl;
    if (moduleChainSize < 1 || recalculateChain) {
        cout << "module " << getId() << " : we need to recalculate module chain (chainSize " << moduleChainSize << "; recalculate " << (recalculateChain ? "t" : "f") << ") calling refreshModuleChain()" << endl;
        refreshModuleChain();
    }
    timeSinceLastLoop += args.sampleTime;
    if (timeSinceLastLoop < sampleRate) {
        return;
    }

    if (blackoutTrigger.process(params[BLACKOUT_BUTTON].getValue())) {
        blackoutTriggered = true;
        return;
    }
    
    // tmp for debug
    // timeSinceLastLoop = 0.0f;
    // loop++;
    // return;

    if (debug) {
        cout << "loop " << loop << "(" << args.sampleTime << ")" << " module " << moduleIndex << " - begin" << endl;
    }

    //cout << "208 > module " << moduleIndex << " master " << (isMaster ? "true" : "false") << endl;
    Input input = inputs[DmxOut1::INPUT_CHANNEL_0];
    
    if (input.isConnected()) {
        float voltage = input.getVoltage();
        float clamped = clamp(voltage, 0.f, 10.f);
        dmxValue = static_cast<uint8_t>(clamped * 255.f / 10.f);
        if (debug) {
            cout << "   module " << moduleIndex << " channel " << dmxChannel << " voltage " << voltage << " dmx " << dmxValue << endl;
        }
    }

    if (false == isMaster) {
        // todo improve loop management
        loop++;
        timeSinceLastLoop = 0.0f;
        return;
    }

    for (int i = 0; i < moduleChainSize; i++) {
        DmxOut1* m = moduleChain.at(i);
        cout << "  checking module " << m->moduleIndex << endl;
        if (m->blackoutTriggered) {
            if (debug) {
                cout << "BLACKOUT triggered on module " << i << " - sending blackout" << endl;
            }
            buffer.Blackout();
            break;
        }

        if (debug) {
            cout << "   setting channel " << m->dmxChannel << " to DMX value " << m->dmxValue << endl;
        }
        buffer.SetChannel(m->dmxChannel, m->dmxValue);
    }

    if (debug) {
        cout << "sending DMX : " << buffer.ToString();
    }

    if (!ola_client->SendDmx(dmxUniverse, buffer)) {
        if (debug) {
            cout << "Sending DMX failed" << endl;
        }
    } else {
        if (debug) {
            cout << "sent" << endl;
        }
    }

    timeSinceLastLoop = 0.0f;
    loop++;
}

json_t* DmxOut1::dataToJson() {
    json_t* rootJson = json_object();
    json_object_set_new(rootJson, dmxAddressJsonKey.c_str(), json_integer(dmxAddress));

    return rootJson;
}

void DmxOut1::dataFromJson(json_t* rootJson)  {
    if (debug) {
        char* jsonStr = json_dumps(rootJson, JSON_INDENT(2));
        cout << "Loading JSON: " << jsonStr << endl;
        free(jsonStr);
    }

    json_t* dmxAddressParamJson = json_object_get(rootJson, dmxAddressJsonKey.c_str());
    if (dmxAddressParamJson) {
        dmxAddress = json_integer_value(dmxAddressParamJson);
    }
}

struct DmxAddressField : ui::TextField {
    DmxOut1* module;

    DmxAddressField(DmxOut1* moduleParam) {
        module = moduleParam;
        box.size.x = 100;
        placeholder = "0";
    }

    void onSelectKey(const event::SelectKey& e) override {
        if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ENTER) {
            int dmxAddress = std::stoi(text);
            if (module) {
                module->dmxAddress = dmxAddress;
                module->recalculateChain = true;
            }
            ui::MenuOverlay* overlay = getAncestorOfType<ui::MenuOverlay>();
            if (overlay) {
                overlay->requestDelete();
            }
            e.consume(this);
        }
        if (!e.getTarget()) {
            TextField::onSelectKey(e);
        }
    }
};

struct DmxAddressMenuItem : ui::MenuItem {
    DmxOut1* module;

    Menu* createChildMenu() override {
        Menu* menu = new Menu;

        DmxAddressField* addressField = new DmxAddressField(module);
        addressField->text = std::to_string(module->dmxAddress);
        menu->addChild(addressField);

        return menu;
    }
};

struct DmxOut1Widget : ModuleWidget {
    DmxOut1* module;

    DmxOut1Widget(DmxOut1* moduleParam) {
        cout << "[DMX] construct DmxOut1Widget" << endl;

        module = moduleParam;
        setModule(module);

        setPanel(createPanel(asset::plugin(pluginInstance, "res/dmx-out-1.svg")));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 42.5)), module, DmxOut1::INPUT_CHANNEL_0));
        addParam(createParamCentered<CKD6>(mm2px(Vec(7.625, 90.0)), module, DmxOut1::BLACKOUT_BUTTON));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("DMX settings"));

        DmxAddressMenuItem* addressItem = new DmxAddressMenuItem;
        addressItem->text = "DMX Address";
        addressItem->rightText = std::to_string(module->dmxAddress) + " " + RIGHT_ARROW;
        addressItem->module = module;
        menu->addChild(addressItem);

        menu->addChild(rack::createBoolPtrMenuItem("Blackout triggered", "", &module->blackoutTriggered));
        menu->addChild(rack::createBoolPtrMenuItem("Debug Mode", "", &module->debug));

        // debug info
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Debug info"));
        menu->addChild(createMenuLabel("Id " + std::to_string(module->getId())));
        menu->addChild(createMenuLabel("Master " + std::to_string(module->isMaster)));
        menu->addChild(createMenuLabel("Chain size " + std::to_string(module->moduleChainSize)));
        menu->addChild(createMenuLabel("Channel " + std::to_string(module->dmxChannel)));
    }
};

Model* modelDmxOut1 = createModel<DmxOut1, DmxOut1Widget>("DmxOut1");
