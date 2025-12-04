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

    // module working variables
    float input0;
    float clamped0;
    float dmx0;

    // blackout button
    dsp::SchmittTrigger blackoutTrigger;
    bool blackoutTriggered = false;

    // dmx params
    unsigned int dmxUniverse = 1;
    unsigned int dmxAddress = 1;

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

    void process(const ProcessArgs& arg) override;

    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJson) override;
    bool isSameModel(Module* otherModule);
};

bool DmxOut1::isSameModel(Module* otherModule) {
    return otherModule->model->plugin->name == "QualiaTouch"
        && otherModule->model->slug == "DmxOut1";
}

void DmxOut1::process(const ProcessArgs& args) {
    timeSinceLastLoop += args.sampleTime;
    if (timeSinceLastLoop < sampleRate) {
        return;
    }

    if (blackoutTrigger.process(params[BLACKOUT_BUTTON].getValue())) {
        blackoutTriggered = true;
        return;
    }

    Module* leftModule = getLeftExpander().module;
    if (leftModule && isSameModel(leftModule)) {
        // todo improve loop management
        loop++;
        timeSinceLastLoop = 0.0f;
        return;
    }

    DmxOut1* m = this;
    int channel = m->dmxAddress;

    if (debug) {
        cout << "loop " << loop << "(" << args.sampleTime << ") - begin" << endl;
    }
    int i = 0;

    while (m && i < 20) {
        // todo should trigger all blackouts
        if (m->blackoutTriggered) {
            if (debug) {
                cout << "blackout triggered on module " << i << " - sending blackout" << endl;
            }
            buffer.Blackout();
            break;
        }

        Input input = m->getInput(DmxOut1::INPUT_CHANNEL_0);
        if (input.isConnected()) {
            float voltage = input.getVoltage();
            float clamped = clamp(voltage, 0.f, 10.f);
            int dmx = static_cast<uint8_t>(clamped * 255.f / 10.f);
            if (debug) {
                cout << "   " << i << ": module " << m->id << "channel " << channel << " voltage " << voltage << " dmx " << dmx << endl;
            }

            buffer.SetChannel(channel, dmx);
        } else {
            if (debug) {
                cout << "   " << i << ": module " << m->id << " input not connected" << endl;
            }
        }

        Module* rightModule = m->getRightExpander().module;
        if (rightModule && isSameModel(rightModule))
        {
            m = dynamic_cast<DmxOut1*>(rightModule);
            channel++;
        } else {
            m = nullptr;
        }
        i++;
    }

    if (debug) {
        cout << "   got " << i << " modules - sending DMX : " << buffer.ToString();
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
    }
};

Model* modelDmxOut1 = createModel<DmxOut1, DmxOut1Widget>("DmxOut1");
