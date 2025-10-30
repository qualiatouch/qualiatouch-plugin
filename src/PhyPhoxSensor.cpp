#include "plugin.hpp"
#include <curl/curl.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>

#include "json.hpp"
using json = nlohmann::json;
using namespace std;

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

struct PhyPhoxSensor : Module {
	enum ParamId {
		PARAMS_LEN
	};

    enum InputId {
		INPUTS_LEN
	};

	enum LightId {
		LIGHTS_LEN
	};

	enum OutputIds {
		OUT_X,
		OUT_Y,
		OUT_Z,
		NUM_OUTPUTS
	};

    std::atomic<bool> dataReady;

    std::string http = "http://";
    std::string ip = "192.168.1.25";
    std::string port = "8080";
    std::string queryParams = "magX&magY&magZ";
    std::string url;

    enum Sensor {
        SENSOR_MAG,
        SENSOR_ACC
    };

    Sensor sensor = SENSOR_MAG;

	float outMagX = 0.f;
	float outMagY = 0.f;
	float outMagZ = 0.f;

	float timeSinceLastRequest = 0.f;
	bool isFetching = false;

    bool debug = false;

	std::atomic<int> nextRequestId;
	int nextExpectedId = 0;
	
	struct OrderedResult {
		int requestId;
		float magX;
		float magY;
		float magZ;
	};	
	
	std::mutex resultMutex;
	std::priority_queue<
		OrderedResult,
		std::vector<OrderedResult>,
		std::function<bool(const OrderedResult&, const OrderedResult&)>
	> resultQueue{[](const OrderedResult& a, const OrderedResult& b) {
		return a.requestId > b.requestId;  // Min-heap
	}};

	PhyPhoxSensor() {
		config(PARAMS_LEN, INPUTS_LEN, NUM_OUTPUTS, LIGHTS_LEN);

        configOutput(OUT_X, "X");
        configOutput(OUT_Y, "Y");
        configOutput(OUT_Z, "Z");

        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        url = http.append(ip).append(":").append(port).append("/get?").append(queryParams);
        if (debug) {
            cout << "url is " << url << endl;
        }

		this->dataReady = false;
		this->nextRequestId = 0;
	}

    void process(const ProcessArgs& args) override;
};

// TODO will not work correctly with more than one module

void fetchHttpAsync(PhyPhoxSensor* module, int requestId) {
    if (module->debug) {
	    cout << "fetchHttpAsync" << endl;
    }
    // todo in constructor ?
    CURL* curl = curl_easy_init();
    if (!curl) {
        cout << "error with curl init : " << endl;
        return;
    }

    std::string response;
    if (module->debug) {
        cout << "querying " << module->url << endl;
    }

    // curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.1.25:8080/get?magX&magY&magZ");
    curl_easy_setopt(curl, CURLOPT_URL, module->url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    if (module->debug) {
	    cout << "curl_easy_perform" << endl;
    }

    CURLcode res = curl_easy_perform(curl);

    if (module->debug) {
	    cout << "res = " << res << endl;
    }

    if (res == CURLE_OK) {
        try {
            if (module->debug) {
                cout << "response = " << response << endl;
            }
            json j = json::parse(response);
            if (module->debug) {
                cout << "j = " << j << endl;
            }
            float magX = j["buffer"]["magX"]["buffer"][0];
            float magY = j["buffer"]["magY"]["buffer"][0];
            float magZ = j["buffer"]["magZ"]["buffer"][0];

            if (module->debug) {
                cout << "magX = " << magX << endl;
                cout << "magY = " << magY << endl;
                cout << "magZ = " << magZ << endl;
            }

            // todo params + clearer
            float scaledMagX = clamp((magX + 500.0f) * (10.0f / 1500.0f) - 5.0f, -5.0f, 5.0f);
			float scaledMagY = clamp(((magY + 200.0f) * (10.0f / 400.0f)) - 5.0f, -5.0f, 5.0f);
			float scaledMagZ = clamp(((magZ + 40.0f) * (10.0f / 2540.0f)) - 5.0f, -5.0f, 5.0f);

            if (module->debug) {
                cout << "scaledMagX = " << scaledMagX << endl;
                cout << "scaledMagY = " << scaledMagY << endl;
                cout << "scaledMagZ = " << scaledMagZ << endl;
            }

            std::lock_guard<std::mutex> lock(module->resultMutex);
			module->resultQueue.push({requestId, scaledMagX, scaledMagY, scaledMagZ});
		} catch (const std::exception& e) {
            if (module->debug) {
                cout << "error " << "" << " : " << e.what() << endl;
            }
			module->dataReady = true;
		}
    } else {
        if (module->debug) {
            cout << "error with curl : " << res << " - " << curl_easy_strerror(res) << endl;
        }
    }

    curl_easy_cleanup(curl);
    module->isFetching = false;
}

void PhyPhoxSensor::process(const ProcessArgs& args) {
		timeSinceLastRequest += args.sampleTime;
		if (!isFetching && timeSinceLastRequest >= 0.01f) { // change time here
			int id = nextRequestId++;
            if (debug) {
			    cout << "next request : " << id << std::endl;
            }
			isFetching = true;
			timeSinceLastRequest = 0.f;
			std::thread(fetchHttpAsync, this, id).detach();
		}

		{
			std::lock_guard<std::mutex> lock(resultMutex);
			while (!resultQueue.empty() && resultQueue.top().requestId == nextExpectedId) {
				auto result = resultQueue.top();
				resultQueue.pop();
		
				outMagX = result.magX;
				outMagY = result.magY;
				outMagZ = result.magZ;

				nextExpectedId++;
			}
		}
		
		outputs[OUT_X].setVoltage(outMagX);
		outputs[OUT_Y].setVoltage(outMagY);
		outputs[OUT_Z].setVoltage(outMagZ);
}

struct PhyPhoxWidget : ModuleWidget {
	PhyPhoxWidget(PhyPhoxSensor* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/phyphox-sensor.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 52.5)), module, PhyPhoxSensor::OUT_X));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 72.5)), module, PhyPhoxSensor::OUT_Y));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 92.5)), module, PhyPhoxSensor::OUT_Z));
	}
};


Model* modelPhyPhoxSensor = createModel<PhyPhoxSensor, PhyPhoxWidget>("PhyPhoxSensor");
