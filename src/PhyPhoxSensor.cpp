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

struct RGB { int r, g, b; };

struct SensorValues {
    int x, y, z;
};

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
        STATUS_LIGHT_RED,
        STATUS_LIGHT_GREEN,
        STATUS_LIGHT_BLUE,
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

    enum Status {
        INIT,
        MEASURING,
        NOT_MEASURING,
        ERROR
    };

    enum Sensor {
        SENSOR_MAG = 0,
        SENSOR_ACC = 1,
        SENSOR_LIGHT = 2,
        SENSOR_TILT = 3,
        SENSOR_SOUND = 4,
        SENSOR_COLOR = 5
    };

    enum Coord {
        COORD_X,
        COORD_Y,
        COORD_Z
    };

    Status status = INIT;
    Sensor sensor = SENSOR_MAG;
    int sensorModeParam = SENSOR_MAG;

    enum VoltageMode {
        UNIPOLAR = 0,
        BIPOLAR = 1
    };

    VoltageMode voltageMode = UNIPOLAR;

    std::string sensorModeParamJsonKey = "sensorModeParam";
    std::string voltageModeJsonKey = "voltageMode";
    std::string ipJsonKey = "ip";

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

    const float DEFAULT_MIN_X_LIGHT = 000.f;
    const float DEFAULT_MAX_X_LIGHT = 500.f;
    const float DEFAULT_MIN_Y_LIGHT = 000.f;
    const float DEFAULT_MAX_Y_LIGHT = 500.f;
    const float DEFAULT_MIN_Z_LIGHT = 000.f;
    const float DEFAULT_MAX_Z_LIGHT = 500.f;

    const float DEFAULT_MIN_X_TILT = -180.f;
    const float DEFAULT_MAX_X_TILT = 180.f;
    const float DEFAULT_MIN_Y_TILT = -180.f;
    const float DEFAULT_MAX_Y_TILT = 180.f;
    const float DEFAULT_MIN_Z_TILT = -180.f;
    const float DEFAULT_MAX_Z_TILT = 180.f;

    const float DEFAULT_MIN_X_SOUND = -50.f;
    const float DEFAULT_MAX_X_SOUND = -10.f;
    const float DEFAULT_MIN_Y_SOUND = -50.f;
    const float DEFAULT_MAX_Y_SOUND = -10.f;
    const float DEFAULT_MIN_Z_SOUND = -50.f;
    const float DEFAULT_MAX_Z_SOUND = -10.f;

    const float DEFAULT_MIN_X_COLOR = 0;
    const float DEFAULT_MAX_X_COLOR = 255;
    const float DEFAULT_MIN_Y_COLOR = 0;
    const float DEFAULT_MAX_Y_COLOR = 255;
    const float DEFAULT_MIN_Z_COLOR = 0;
    const float DEFAULT_MAX_Z_COLOR = 255;

    float sensorMinX = DEFAULT_MIN_X_MAG;
    float sensorMaxX = DEFAULT_MAX_X_MAG;
    float sensorMinY = DEFAULT_MIN_Y_MAG;
    float sensorMaxY = DEFAULT_MAX_Y_MAG;
    float sensorMinZ = DEFAULT_MIN_Z_MAG;
    float sensorMaxZ = DEFAULT_MAX_Z_MAG;

	float timeSinceLastRequest = 0.f;
	bool isFetching = false;

    bool debug = false;

	std::atomic<int> nextRequestId;
	int nextExpectedId = 0;
	
	struct OrderedResult {
		int requestId;
        bool isMeasuring;
        bool hasError;
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

        configLight(STATUS_LIGHT_RED, "Status");
        configLight(STATUS_LIGHT_GREEN, "");
        configLight(STATUS_LIGHT_BLUE, "");

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

    float calculateOutputVoltage(float rawValue, float rawMin, float rawMax);

    void process(const ProcessArgs& args) override;

    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJson) override;
    Status getStatus(bool hasError, bool isMeasuring);
    void updateLedColor();
    void setStatusLedColor(float red, float green, float blue);
};

void PhyPhoxSensor::setWidget(PhyPhoxWidget* widgetParam) {
    widget = widgetParam;
}

void PhyPhoxSensor::setIpAddress(std::string newIp) {
    ip = newIp;
    initUrl();
}

void PhyPhoxSensor::initUrl() {
    sensor = (Sensor) sensorModeParam;
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
        case Sensor::SENSOR_LIGHT:
            sensorMinX = DEFAULT_MIN_X_LIGHT;
            sensorMaxX = DEFAULT_MAX_X_LIGHT;
            sensorMinY = DEFAULT_MIN_Y_LIGHT;
            sensorMaxY = DEFAULT_MAX_Y_LIGHT;
            sensorMinZ = DEFAULT_MIN_Z_LIGHT;
            sensorMaxZ = DEFAULT_MAX_Z_LIGHT;
            break;
        case Sensor::SENSOR_TILT:
            sensorMinX = DEFAULT_MIN_X_TILT;
            sensorMaxX = DEFAULT_MAX_X_TILT;
            sensorMinY = DEFAULT_MIN_Y_TILT;
            sensorMaxY = DEFAULT_MAX_Y_TILT;
            sensorMinZ = DEFAULT_MIN_Z_TILT;
            sensorMaxZ = DEFAULT_MAX_Z_TILT;
            break;
        case Sensor::SENSOR_SOUND:
            sensorMinX = DEFAULT_MIN_X_SOUND;
            sensorMaxX = DEFAULT_MAX_X_SOUND;
            sensorMinY = DEFAULT_MIN_Y_SOUND;
            sensorMaxY = DEFAULT_MAX_Y_SOUND;
            sensorMinZ = DEFAULT_MIN_Z_SOUND;
            sensorMaxZ = DEFAULT_MAX_Z_SOUND;
            break;
        case Sensor::SENSOR_COLOR:
            sensorMinX = DEFAULT_MIN_X_COLOR;
            sensorMaxX = DEFAULT_MAX_X_COLOR;
            sensorMinY = DEFAULT_MIN_Y_COLOR;
            sensorMaxY = DEFAULT_MAX_Y_COLOR;
            sensorMinZ = DEFAULT_MIN_Z_COLOR;
            sensorMaxZ = DEFAULT_MAX_Z_COLOR;
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
        case PhyPhoxSensor::SENSOR_LIGHT:
            return "illum";
        case PhyPhoxSensor::SENSOR_TILT:
            return "tiltFlatUD&tiltFlatLR";
        case PhyPhoxSensor::SENSOR_SOUND:
            return "dB";
        case PhyPhoxSensor::SENSOR_COLOR:
            return "h&s&v";
        default:
            return "";
    }
}

static std::string getType(const PhyPhoxSensor::Sensor sensor, PhyPhoxSensor::Coord coord) {
    std::string type = "";
    std::string arg = "";
    switch (sensor)
    {
        case PhyPhoxSensor::SENSOR_MAG:
            arg = "X";
            if (coord == PhyPhoxSensor::COORD_Y) {
                arg = "Y";
            } else if (coord == PhyPhoxSensor::COORD_Z) {
                arg = "Z";
            }
            type.append("mag").append(arg);
            return type;
        case PhyPhoxSensor::SENSOR_ACC:
            arg = "X";
            if (coord == PhyPhoxSensor::COORD_Y) {
                arg = "Y";
            } else if (coord == PhyPhoxSensor::COORD_Z) {
                arg = "Z";
            }
            type.append("acc").append(arg);
            return type;
        case PhyPhoxSensor::SENSOR_LIGHT:
            return "illum";
        case PhyPhoxSensor::SENSOR_TILT:
            arg = coord == PhyPhoxSensor::COORD_X ? "UD" : "LR";
            return type.append("tiltFlat").append(arg);
        case PhyPhoxSensor::SENSOR_SOUND:
            return "dB";
        case PhyPhoxSensor::SENSOR_COLOR:
            arg = "h";
            if (coord == PhyPhoxSensor::COORD_Y) {
                arg = "s";
            } else if (coord == PhyPhoxSensor::COORD_Z) {
                arg = "v";
            }
            return arg;
        default:
            return "";
    }
}

static float getBufferValue(const json j, const PhyPhoxSensor::Sensor sensor, PhyPhoxSensor::Coord coord) {
    if (j["buffer"].empty()) {
        return 0.f;
    }

    std::string p = getType(sensor, coord);
    if (false == j["buffer"].contains(p)) {
        return 0.f;
    }

    return j["buffer"][p]["buffer"][0];
}

static bool getMeasuringValue(const json j) {
    if (j["status"].empty()) {
        return false;
    }

    return j["status"]["measuring"];
}

static RGB hsv_to_rgb(float h, float s, float v) {
    float c = v * s; // chroma
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;

    float r1, g1, b1;

    if (h < 60)       { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
    else              { r1 = c; g1 = 0; b1 = x; }

    RGB rgb;
    rgb.r = static_cast<int>((r1 + m) * 255);
    rgb.g = static_cast<int>((g1 + m) * 255);
    rgb.b = static_cast<int>((b1 + m) * 255);
    return rgb;
}

static SensorValues getValue(const json j, const PhyPhoxSensor::Sensor sensor) {
    float valueX = getBufferValue(j, sensor, PhyPhoxSensor::COORD_X);
    float valueY = getBufferValue(j, sensor, PhyPhoxSensor::COORD_Y);
    float valueZ = getBufferValue(j, sensor, PhyPhoxSensor::COORD_Z);
    SensorValues v;
    switch (sensor) {
        case PhyPhoxSensor::SENSOR_MAG:
        case PhyPhoxSensor::SENSOR_ACC:
        case PhyPhoxSensor::SENSOR_LIGHT:
        case PhyPhoxSensor::SENSOR_TILT:
        case PhyPhoxSensor::SENSOR_SOUND:
            v.x = valueX;
            v.y = valueY;
            v.z = valueZ;
            return v;
        case PhyPhoxSensor::SENSOR_COLOR:
            RGB rgb = hsv_to_rgb(valueX, valueY, valueZ);
            v.x = rgb.r;
            v.y = rgb.g;
            v.z = rgb.b;
            return v;
    };

    return v;
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

float PhyPhoxSensor::calculateOutputVoltage(float rawValue, float rawMin, float rawMax) {
    switch (voltageMode) {
        case VoltageMode::BIPOLAR:
            return scaleAndClamp(rawValue, rawMin, rawMax, -5.0f, 5.0f);
        case VoltageMode::UNIPOLAR:
        default:
            return scaleAndClamp(rawValue, rawMin, rawMax, 0.f, 10.f);
    }
}

PhyPhoxSensor::Status PhyPhoxSensor::getStatus(bool hasError, bool isMeasuring) {
    if (hasError) {
        return ERROR;
    }

    if (isMeasuring) {
        return MEASURING;
    }

    return NOT_MEASURING;
}

void PhyPhoxSensor::setStatusLedColor(float red, float green, float blue) {
    lights[STATUS_LIGHT_RED].setBrightness(red);
    lights[STATUS_LIGHT_GREEN].setBrightness(green);
    lights[STATUS_LIGHT_BLUE].setBrightness(blue);
}

void PhyPhoxSensor::updateLedColor() {
    switch (status)
    {
        case INIT:
            setStatusLedColor(0.f, 0.f, 1.f);
            break;
        case MEASURING:
            setStatusLedColor(0.f, 1.f, 0.f);
            break;
        case NOT_MEASURING:
            setStatusLedColor(1.f, 0.5f, 0.f);
            break;
        case ERROR:
            setStatusLedColor(1.f, 0.f, 0.f);
            break;
        default:
            setStatusLedColor(0.f, 0.f, 0.f);
            break;
    }
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

    bool isMeasuring = false;
    bool hasError = false;
    float voltageX = 0.f;
    float voltageY = 0.f;
    float voltageZ = 0.f;

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

            isMeasuring = getMeasuringValue(j);
            if (isMeasuring) {
                SensorValues values = getValue(j, module->sensor);
                float sensorX = values.x;
                float sensorY = values.y;
                float sensorZ = values.z;

                if (module->debug) {
                    cout << "sensorX = " << sensorX << endl;
                    cout << "sensorY = " << sensorY << endl;
                    cout << "sensorZ = " << sensorZ << endl;
                }

                voltageX = module->calculateOutputVoltage(sensorX, module->sensorMinX, module->sensorMaxX);
                voltageY = module->calculateOutputVoltage(sensorY, module->sensorMinY, module->sensorMaxY);
                voltageZ = module->calculateOutputVoltage(sensorZ, module->sensorMinZ, module->sensorMaxZ);

                if (module->debug) {
                    cout << "voltageX = " << voltageX << endl;
                    cout << "voltageY = " << voltageY << endl;
                    cout << "voltageZ = " << voltageZ << endl;
                }
            }
		} catch (const std::exception& e) {
            hasError = true;
            if (module->debug) {
                cout << "error " << "" << " : " << e.what() << endl;
            }
			module->dataReady = true;
		}
    } else {
        hasError = true;
        if (module->debug) {
            cout << "error with curl : " << res << " - " << curl_easy_strerror(res) << endl;
        }
    }

    std::lock_guard<std::mutex> lock(module->resultMutex);
    module->resultQueue.push({requestId, isMeasuring, hasError, voltageX, voltageY, voltageZ});

    curl_easy_cleanup(curl);
    module->isFetching = false;
}

json_t* PhyPhoxSensor::dataToJson() {
    json_t* rootJson = json_object();
    json_object_set_new(rootJson, ipJsonKey.c_str(), json_string(ip.c_str()));
    json_object_set_new(rootJson, sensorModeParamJsonKey.c_str(), json_integer(sensorModeParam));
    json_object_set_new(rootJson, voltageModeJsonKey.c_str(), json_integer(voltageMode));

    return rootJson;
}

void PhyPhoxSensor::dataFromJson(json_t* rootJson)  {
    if (debug) {
        char* jsonStr = json_dumps(rootJson, JSON_INDENT(2));
        cout << "Loading JSON: " << jsonStr << endl;
        free(jsonStr);
    }

    json_t* sensorModeParamJson = json_object_get(rootJson, sensorModeParamJsonKey.c_str());
    if (sensorModeParamJson) {
        sensorModeParam = json_integer_value(sensorModeParamJson);
    }

    json_t* voltageModeJson = json_object_get(rootJson, voltageModeJsonKey.c_str());
    if (voltageModeJson) {
        voltageMode = (VoltageMode) json_integer_value(voltageModeJson);
    }

    json_t* ipJson = json_object_get(rootJson, ipJsonKey.c_str());
    if (ipJson) {
        const char* ipValue = json_string_value(ipJson);
        if (ipValue) {
            ip = ipValue;
        }
    }
}

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
	void draw(const DrawArgs& args) override {
        std::string fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
        std::shared_ptr<Font> font = APP->window->loadFont(fontPath);

	    if (font) {
		    nvgFontFaceId(args.vg, font->handle);
            nvgFontSize(args.vg, 13.0);
            nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
            std::string text = "";
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

        addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(7.625, 42.5)), module, PhyPhoxSensor::STATUS_LIGHT_RED));

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
        menu->addChild(createMenuLabel("Sensor settings (PhyPhox app)"));

        menu->addChild(createIndexPtrSubmenuItem("Sensor type",
            {
                "Magnetic",
                "Acceleration",
                "Light",
                "Tilt",
                "Sound intensity",
                "Color (HSV converted to RGB)"
            },
            &module->sensorModeParam
        ));

        menu->addChild(createIndexPtrSubmenuItem("Output voltage mode",
            {
                "Unipolar (0V => +10V)",
                "Bipolar (-5V => +5V)",
            },
            &module->voltageMode
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
}

void PhyPhoxSensor::process(const ProcessArgs& args) {
		timeSinceLastRequest += args.sampleTime;
		if (!isFetching && timeSinceLastRequest >= 0.01f) {
            if (sensorModeParam != sensor) {
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

                status = getStatus(result.hasError, result.isMeasuring);
                updateLedColor();
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
