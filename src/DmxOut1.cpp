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
        INPUT_BLACKOUT,
        INPUTS_LEN
    };

    enum LightIn {
        BLACKOUT_LIGHT,
        LIGHTS_LEN
    };

    enum OutputIds {
        OUTPUTS_LEN
    };

    std::string useOwnDmxAddressJsonKey = "useOwnDmxAddress";
    std::string dmxAddressJsonKey = "dmxAddress";

    // module params
	float timeSinceLastLoop = 0.f;
    int loop = 0;
    bool debug = false;
    bool debugChain = false;
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
    dsp::SchmittTrigger blackoutButtonTrigger;
    dsp::SchmittTrigger blackoutInputTrigger;
    bool blackoutTriggered = false;

    // DMX
    unsigned int dmxUniverse = 1;
    bool useOwnDmxAddress = false;
    unsigned int dmxAddress = 1;
    unsigned int dmxChannel = 1;
    bool updateDmxChannelDisplayWidget = false;

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
        configInput(INPUT_BLACKOUT, "Blackout (Trigger or gate to blackout)");
        configLight(BLACKOUT_LIGHT, "Blackout triggered - deactivate it in the menu");

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
    void toggleUseOwnDmxAddress();

    void process(const ProcessArgs& arg) override;

    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJson) override;
    bool isSameModel(Module* otherModule);
};

void DmxOut1::refreshModuleChain() {
    if (debugChain) {
        cout << "module " << getId() << " in refreshModuleChain()" << endl;
    }
    recalculateChain = false;
    isMaster = false;
    moduleChain.clear();

    // identification du master + déclenchement de refreshModuleChain sur le module à gauche
    Module* leftModule = getLeftExpander().module;
    if (leftModule == nullptr || false == isSameModel(leftModule)) {
        isMaster = true;
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

    if (false == isMaster) {
        // todo improve loop management
        loop++;
        timeSinceLastLoop = 0.0f;
        return;
    }

    for (int i = 0; i < moduleChainSize; i++) {
        DmxOut1* m = moduleChain.at(i);
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

struct UseOwnDmxAddressItem : ui::MenuItem {
    DmxOut1* module;

    UseOwnDmxAddressItem(DmxOut1* moduleParam) {
        module = moduleParam;
    }

    void onAction(const event::Action& e) override {
        module->toggleUseOwnDmxAddress();
    }

    void step() override {
        rightText = module->useOwnDmxAddress ? "✔" : "";
        MenuItem::step();
    }
};

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
                module->dmxChannel = dmxAddress;
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

struct DmxChannelDisplayWidget : Widget {
    DmxOut1* module;
    FramebufferWidget* parentFrameBufferWidget;

    void setModule(DmxOut1* moduleParam) {
        module = moduleParam;
    }

    void setParent(FramebufferWidget* parent) {
        parentFrameBufferWidget = parent;
    }

    void step() override {
        if (!module) {
            return;
        }

        if (module->updateDmxChannelDisplayWidget) {
            module->updateDmxChannelDisplayWidget = false;
            if (module->debug) {
                cout << "updating channel display widget" << endl;
            }
            parentFrameBufferWidget->setDirty();
        }
    }

	void draw(const DrawArgs& args) override {
        std::string fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
        std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
        std::string boldFontPath = asset::system("res/fonts/ShareTechMono-Bold.ttf");
        std::shared_ptr<Font> boldFont = APP->window->loadFont(fontPath);

        if (!font) {
            cerr << "failed to load font " << fontPath << endl;
        }
        if (!boldFont) {
            cerr << "failed to load bold font " << boldFontPath << endl;
        }

        nvgFontSize(args.vg, 16.0);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

        char text[4];

        if (!module) {
            nvgText(args.vg, 0.0, 10, "DMX", NULL);
            return;
        }

        if (module->useOwnDmxAddress) {
            nvgFontFaceId(args.vg, boldFont->handle);
            nvgFillColor(args.vg, nvgRGBf(1.f, 1.f, 1.f));
        } else {
            nvgFontFaceId(args.vg, font->handle);
            nvgFillColor(args.vg, nvgRGBf(0.7f, 0.7f, 0.7f));
        }

        snprintf(text, 4, "%03d", module->dmxChannel);

        nvgText(args.vg, 0.0, 12, text, NULL);
    }
};

struct DmxOut1Widget : ModuleWidget {
    DmxOut1* module;
    FramebufferWidget* frameBufferWidget;
    DmxChannelDisplayWidget* dmxChannelDisplayWidget;

    DmxOut1Widget(DmxOut1* moduleParam) {
        module = moduleParam;
        setModule(module);

        setPanel(createPanel(asset::plugin(pluginInstance, "res/dmx-out-1.svg")));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 42.5)), module, DmxOut1::INPUT_CHANNEL_0));
        addParam(createParamCentered<CKD6>(mm2px(Vec(7.625, 90.0)), module, DmxOut1::BLACKOUT_BUTTON));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 107.5)), module, DmxOut1::INPUT_BLACKOUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(7.625, 100.0)), module, DmxOut1::BLACKOUT_LIGHT));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        frameBufferWidget = new FramebufferWidget;
        addChild(frameBufferWidget);

        dmxChannelDisplayWidget = createWidget<DmxChannelDisplayWidget>(Vec(11,150));
        dmxChannelDisplayWidget->setModule(module);
        dmxChannelDisplayWidget->setParent(frameBufferWidget);
        dmxChannelDisplayWidget->setSize(Vec(25, 12));
        frameBufferWidget->addChild(dmxChannelDisplayWidget);
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("DMX settings"));

        UseOwnDmxAddressItem* useOwnDmxAddressItem = new UseOwnDmxAddressItem(module);
        useOwnDmxAddressItem->text = "Use own DMX address";
        menu->addChild(useOwnDmxAddressItem);

        if (module->useOwnDmxAddress) {
            DmxAddressMenuItem* addressItem = new DmxAddressMenuItem;
            addressItem->text = "DMX Address";
            addressItem->rightText = std::to_string(module->dmxAddress) + " " + RIGHT_ARROW;
            addressItem->module = module;
            menu->addChild(addressItem);
        }

        menu->addChild(rack::createBoolPtrMenuItem("Blackout triggered", "", &module->blackoutTriggered));
        menu->addChild(rack::createBoolPtrMenuItem("Debug", "", &module->debug));

        // debug info
        if (module->debug) {
            menu->addChild(new MenuSeparator);
            menu->addChild(createMenuLabel("Debug info"));
            menu->addChild(createMenuLabel("Id " + std::to_string(module->getId())));
            menu->addChild(createMenuLabel("Master " + std::to_string(module->isMaster)));
            menu->addChild(createMenuLabel("Chain size " + std::to_string(module->moduleChainSize)));
            menu->addChild(createMenuLabel("Channel " + std::to_string(module->dmxChannel)));
            menu->addChild(createMenuLabel("Use own address " + std::to_string(module->useOwnDmxAddress)));
            menu->addChild(rack::createBoolPtrMenuItem("Debug Chain", "", &module->debugChain));
        }
    }
};

Model* modelDmxOut1 = createModel<DmxOut1, DmxOut1Widget>("DmxOut1");
