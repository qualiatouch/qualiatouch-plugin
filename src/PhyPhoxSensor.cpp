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

struct PhyPhoxWidget;

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
    std::string ip = "192.168.1.25:8080";
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

    const float DEFAULT_MIN_X_MAG = -500.f;
    const float DEFAULT_MAX_X_MAG = 1000.f;
    const float DEFAULT_MIN_Y_MAG = -200.f;
    const float DEFAULT_MAX_Y_MAG = 200.f;
    const float DEFAULT_MIN_Z_MAG = -40.f;
    const float DEFAULT_MAX_Z_MAG = 2500.f;

    const float DEFAULT_MIN_X_ACC = -100.f;
    const float DEFAULT_MAX_X_ACC = 100.f;
    const float DEFAULT_MIN_Y_ACC = -100.f;
    const float DEFAULT_MAX_Y_ACC = 100.f;
    const float DEFAULT_MIN_Z_ACC = -100.f;
    const float DEFAULT_MAX_Z_ACC = 100.f;

    float sensorMinX = DEFAULT_MIN_X_MAG;
    float sensorMaxX = DEFAULT_MAX_X_MAG;
    float sensorMinY = DEFAULT_MIN_Y_MAG;
    float sensorMaxY = DEFAULT_MAX_Y_MAG;
    float sensorMinZ = DEFAULT_MIN_Z_MAG;
    float sensorMaxZ = DEFAULT_MAX_Z_MAG;

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

    PhyPhoxWidget* widget;

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

    void setWidget(PhyPhoxWidget* widgetParam);

    void initUrl();
    void initLimits();
    void initSensor();

    void setIpAddress(std::string newIp);

    std::string getQueryParams(Sensor sensor);

    void process(const ProcessArgs& args) override;
};

void PhyPhoxSensor::setWidget(PhyPhoxWidget* widgetParam) {
    widget = widgetParam;
}

void PhyPhoxSensor::setIpAddress(std::string newIp) {
    ip = newIp;
    initUrl();
}

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
    url = url.append(ip).append("/get?").append(queryParams);
    if (debug) {
        cout << "url = " << url << endl;
    }
}

void PhyPhoxSensor::initLimits() {
    switch (sensor) {
        case Sensor::SENSOR_MAG:
            sensorMinX = DEFAULT_MIN_X_MAG;
            sensorMaxX = DEFAULT_MAX_X_MAG;
            sensorMinY = DEFAULT_MIN_Y_MAG;
            sensorMaxY = DEFAULT_MAX_Y_MAG;
            sensorMinZ = DEFAULT_MIN_Z_MAG;
            sensorMaxZ = DEFAULT_MAX_Z_MAG;
            break;
        case Sensor::SENSOR_ACC:
            sensorMinX = DEFAULT_MIN_X_ACC;
            sensorMaxX = DEFAULT_MAX_X_ACC;
            sensorMinY = DEFAULT_MIN_Y_ACC;
            sensorMaxY = DEFAULT_MAX_Y_ACC;
            sensorMinZ = DEFAULT_MIN_Z_ACC;
            sensorMaxZ = DEFAULT_MAX_Z_ACC;
            break;
    }
}

std::string PhyPhoxSensor::getQueryParams(Sensor sensor) {
    switch (sensor)
    {
        case PhyPhoxSensor::SENSOR_MAG:
            return "magX&magY&magZ";
        case PhyPhoxSensor::SENSOR_ACC:
            return "accX&accY&accZ";
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

            float scaledX = scaleAndClamp(sensorX, module->sensorMinX, module->sensorMaxX, -5.0f, 5.0f);
            float scaledY = scaleAndClamp(sensorY, module->sensorMinY, module->sensorMaxY, -5.0f, 5.0f);
            float scaledZ = scaleAndClamp(sensorZ, module->sensorMinZ, module->sensorMaxZ, -5.0f, 5.0f);

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

// ui::OptionButton

// todo save params
// todo same for debug
// todo same for dmx address

struct IpAddressField : ui::TextField {
    PhyPhoxSensor* module;

    IpAddressField(PhyPhoxSensor* moduleParam) {
        module = moduleParam;
        box.size.x = 200;
        placeholder = "192.168.1.25:8080";
    }

    void onSelectKey(const event::SelectKey& e) override {
        if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ENTER) {
            std::string ip = text;
            if (module) {
                module->setIpAddress(ip);
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

struct IpAddressMenuItem : ui::MenuItem {
    PhyPhoxSensor* module;

    Menu* createChildMenu() override {
        Menu* menu = new Menu;

        IpAddressField* ipField = new IpAddressField(module);
        ipField->text = module->ip;
        menu->addChild(ipField);

        return menu;
    }
};

struct SensorTypeWidget : Widget {
    PhyPhoxSensor* module;

	void draw(const DrawArgs& args) override {
        std::string fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
        std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
        if (module->debug) {
            cout << "SensorTypeWidget:draw() sensor = " << module->sensor << endl;
        }

	    if (font) {
		    nvgFontFaceId(args.vg, font->handle);
            nvgFontSize(args.vg, 13.0);
            nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
            std::string text = getText(module);
            nvgText(args.vg, 0.0, 10.0, text.c_str(), NULL);
        } else {
            cerr << "failed to load font " << fontPath << endl;
        }
    }

    void setModule(PhyPhoxSensor* moduleParam) {
        module = moduleParam;
    }

    std::string getText(PhyPhoxSensor* module) {
        switch (module->sensor)
        {
            case PhyPhoxSensor::Sensor::SENSOR_MAG:
                return "MAG";
            case PhyPhoxSensor::Sensor::SENSOR_ACC:
                return "ACC";
            default:
                return "ERR";
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
        module->setWidget(this);

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
        sensorTypeWidget->setModule(module);
        frameBufferWidget->addChild(sensorTypeWidget);
	}

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Sensor settings (PhyPhox app)"));

        menu->addChild(createIndexPtrSubmenuItem("Sensor type",
            {
                "Magnetic",
                "Acceleration"
            },
            &module->modeParam
        ));

        IpAddressMenuItem* ipItem = new IpAddressMenuItem;
        ipItem->text = "IP & port";
        ipItem->rightText = module->ip + " " + RIGHT_ARROW;
        ipItem->module = module;
        menu->addChild(ipItem);

        menu->addChild(rack::createBoolPtrMenuItem("Debug Mode", "", &module->debug));
    }

    void setDirty();
};

void PhyPhoxSensor::initSensor() {
    initUrl();
    initLimits();
    widget->setDirty();
}

void PhyPhoxSensor::process(const ProcessArgs& args) {
		timeSinceLastRequest += args.sampleTime;
		if (!isFetching && timeSinceLastRequest >= 0.01f) {
            if (modeParam != sensor) {
                initSensor();
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
		
        if (debug) {
            // cout << "sending voltages outX=" << outX << " outY=" << outY << " outZ" << outZ << endl;
        }
		outputs[OUT_X].setVoltage(outX);
		outputs[OUT_Y].setVoltage(outY);
		outputs[OUT_Z].setVoltage(outZ);
}

void PhyPhoxWidget::setDirty() {
    frameBufferWidget->setDirty();
}

Model* modelPhyPhoxSensor = createModel<PhyPhoxSensor, PhyPhoxWidget>("PhyPhoxSensor");
