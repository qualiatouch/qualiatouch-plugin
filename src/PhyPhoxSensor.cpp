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
    std::string queryParams = "";
    std::string url;

    enum Sensor {
        SENSOR_MAG = 0,
        SENSOR_ACC = 1
    };

    Sensor sensor = SENSOR_MAG;
    int modeParam = SENSOR_MAG;

	float outX = 0.f;
	float outY = 0.f;
	float outZ = 0.f;

	float timeSinceLastRequest = 0.f;
	bool isFetching = false;

    bool debug = true;

	std::atomic<int> nextRequestId;
	int nextExpectedId = 0;
	
	struct OrderedResult {
		int requestId;
		float resultX;
		float resultY;
		float resultZ;
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

        initUrl();
        if (debug) {
            cout << "url is " << url << endl;
        }

		this->dataReady = false;
		this->nextRequestId = 0;
	}

    void initUrl();

    std::string getQueryParams(Sensor sensor);

    void process(const ProcessArgs& args) override;
};

void PhyPhoxSensor::initUrl() {
    sensor = (Sensor) modeParam;
    if (debug) {
        cout << "initUrl() sensor = " << sensor << endl;
    }

    queryParams = getQueryParams(sensor);

    if (debug) {
        cout << "queryParams = " << queryParams << endl;
    }

    url = http;
    url = url.append(ip).append(":").append(port).append("/get?").append(queryParams);
    if (debug) {
        cout << "url = " << url << endl;
    }
}

std::string PhyPhoxSensor::getQueryParams(Sensor sensor) {
    switch (sensor)
    {
        case PhyPhoxSensor::SENSOR_MAG:
            return "magX&magY&magZ";
        case PhyPhoxSensor::SENSOR_ACC:
            return "accX&accY&ccZ";
        default:
            return "";
    }
}

static std::string getType(const PhyPhoxSensor::Sensor sensor) {
    switch (sensor)
    {
        case PhyPhoxSensor::SENSOR_MAG:
            return "mag";
        case PhyPhoxSensor::SENSOR_ACC:
            return "acc";
        default:
            return "";
    }
}

static float getValue(const json j, const PhyPhoxSensor::Sensor sensor, const char* coord) {
    if (j["buffer"].empty()) {
        cout << "empty" << endl;
        return 0.f;
    }
    std::string p = getType(sensor).append(coord);
    if (false == j["buffer"].contains(p)) {
        cout << "is not object" << endl;
        return 0.f;
    }

    return j["buffer"][p]["buffer"][0];
}

static float scaleAndClamp(
    float rawValue,
    float rawMin, float rawMax,
    float outMin, float outMax
) {
    // 1. Normalize input to 0–1 range
    float normalized = (rawValue - rawMin) / (rawMax - rawMin);

    // 2. Scale to output range
    float scaled = normalized * (outMax - outMin) + outMin;

    // 3. Clamp to ensure bounds
    return clamp(scaled, outMin, outMax);
}

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
            float sensorX = getValue(j, module->sensor, "X");
            float sensorY = getValue(j, module->sensor, "Y");
            float sensorZ = getValue(j, module->sensor, "Z");

            if (module->debug) {
                cout << "sensorX = " << sensorX << endl;
                cout << "sensorY = " << sensorY << endl;
                cout << "sensorZ = " << sensorZ << endl;
            }

            float scaledX = scaleAndClamp(sensorX, -500.0f, 1000.0f, -5.0f, 5.0f);
            float scaledY = scaleAndClamp(sensorY, -200.0f, 200.0f, -5.0f, 5.0f);
            float scaledZ = scaleAndClamp(sensorZ, -40.0f, 2500.0f, -5.0f, 5.0f);

            if (module->debug) {
                cout << "scaledX = " << scaledX << endl;
                cout << "scaledY = " << scaledY << endl;
                cout << "scaledZ = " << scaledZ << endl;
            }

            std::lock_guard<std::mutex> lock(module->resultMutex);
			module->resultQueue.push({requestId, scaledX, scaledY, scaledZ});
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
		if (!isFetching && timeSinceLastRequest >= 0.01f) {
            if (modeParam != sensor) {
                initUrl();
            }

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
		
				outX = result.resultX;
				outY = result.resultY;
				outZ = result.resultZ;

				nextExpectedId++;
			}
		}
		
		outputs[OUT_X].setVoltage(outX);
		outputs[OUT_Y].setVoltage(outY);
		outputs[OUT_Z].setVoltage(outZ);
}

struct SensorTypeWidget : Widget {
	void draw(const DrawArgs& args) override {
        std::string fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
        std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
        cout << "draw()" << endl;

	    if (font) {
		    nvgFontFaceId(args.vg, font->handle);
            nvgFontSize(args.vg, 13.0);
            nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
            std::string text = "MAG";
            nvgText(args.vg, 0.0, 10.0, text.c_str(), NULL);
        } else {
            cerr << "failed to load font " << fontPath << endl;
        }
    }
};

struct PhyPhoxWidget : ModuleWidget {
    PhyPhoxSensor* module;
    FramebufferWidget* frameBufferWidget;
    SensorTypeWidget* sensorTypeWidget;

	PhyPhoxWidget(PhyPhoxSensor* moduleParam) {
        module = moduleParam;
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/phyphox-sensor.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 52.5)), module, PhyPhoxSensor::OUT_X));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 72.5)), module, PhyPhoxSensor::OUT_Y));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 92.5)), module, PhyPhoxSensor::OUT_Z));

        frameBufferWidget = new FramebufferWidget;
        addChild(frameBufferWidget);

        sensorTypeWidget = createWidget<SensorTypeWidget>(Vec(13.0, 120.0));
        sensorTypeWidget->setSize(Vec(100, 100));
        frameBufferWidget->addChild(sensorTypeWidget);
	}

	void appendContextMenu(Menu* menu) override {
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Sensor settings"));

		menu->addChild(createIndexPtrSubmenuItem("Sensor type",
			{
                "Magnetic",
                "Acceleration"
            },
			&module->modeParam
		));
    }
};

Model* modelPhyPhoxSensor = createModel<PhyPhoxSensor, PhyPhoxWidget>("PhyPhoxSensor");
