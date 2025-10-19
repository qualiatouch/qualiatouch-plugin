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

    // module params
	float timeSinceLastLoop = 0.f;
    int loop = 0;
    bool debug = false;
    float sampleRate = 0.1f;

    // module working variables
    float input0;
    float clamped0;
    float dmx0;

    // dmx params
    unsigned int dmxUniverse = 1;
    unsigned int dmxAddress = 0;

    ola::DmxBuffer buffer;

    std::unique_ptr<ola::client::StreamingClient> ola_client;

    DmxOut1() {
        if (debug) {
            cout << "[DMX] construct DmxOut1" << endl;
        }

        // # init Module
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

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
};

void DmxOut1::process(const ProcessArgs& args) {
	timeSinceLastLoop += args.sampleTime;
    if (timeSinceLastLoop < sampleRate) {
        return;
    }

    if (false == inputs[DmxOut1::INPUT_CHANNEL_0].isConnected()) {
        return;
    }

    input0 = inputs[DmxOut1::INPUT_CHANNEL_0].getVoltage();
    clamped0 = clamp(input0, 0.f, 10.f);
    dmx0 = static_cast<uint8_t>(clamped0 * 255.f / 10.f);

    if (debug) {
        cout << loop << " : " << args.sampleTime << " " << "channel0 = " << input0 << " dmx0 = " << dmx0 << " : ";
    }

    buffer.SetChannel(dmxAddress, dmx0);
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


struct DmxOut1Widget : ModuleWidget {
    DmxOut1Widget(DmxOut1* module) {
        cout << "[DMX] construct DmxOut1Widget" << endl;

        setModule(module);

        setPanel(createPanel(asset::plugin(pluginInstance, "res/dmx-out-1.svg")));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 42.5)), module, DmxOut1::INPUT_CHANNEL_0));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }
};

Model* modelDmx1Out = createModel<DmxOut1, DmxOut1Widget>("DmxOut1");
