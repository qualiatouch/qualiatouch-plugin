#include "PhoneSensor.hpp"
#include "PhoneSensorWidget.hpp"
#include "../util/utils.hpp"

using json = nlohmann::json;
using namespace std;
using namespace utils;

struct RGB { int r, g, b; };

struct SensorValues {
    int x, y, z;
};

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

PhoneSensor::PhoneSensor() {
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

void PhoneSensor::setWidget(PhoneSensorWidget* widgetParam) {
    widget = widgetParam;
}

void PhoneSensor::setIpAddress(std::string newIp) {
    ip = newIp;
    initUrl();
}

void PhoneSensor::initUrl() {
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

void PhoneSensor::initLimitsFromDefaults() {
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
        case Sensor::SENSOR_GYR:
            sensorMinX = DEFAULT_MIN_X_GYR;
            sensorMaxX = DEFAULT_MAX_X_GYR;
            sensorMinY = DEFAULT_MIN_Y_GYR;
            sensorMaxY = DEFAULT_MAX_Y_GYR;
            sensorMinZ = DEFAULT_MIN_Z_GYR;
            sensorMaxZ = DEFAULT_MAX_Z_GYR;
            break;
    }
}

void PhoneSensor::initLimitsFromJson() {
    switch (sensor) {
        case Sensor::SENSOR_MAG:
            sensorMinX = minXParam;
            sensorMaxX = maxXParam;
            sensorMinY = minYParam;
            sensorMaxY = maxYParam;
            sensorMinZ = minZParam;
            sensorMaxZ = maxZParam;
            break;
        case Sensor::SENSOR_ACC:
            sensorMinX = minXParam;
            sensorMaxX = maxXParam;
            sensorMinY = minYParam;
            sensorMaxY = maxYParam;
            sensorMinZ = minZParam;
            sensorMaxZ = maxZParam;
            break;
        case Sensor::SENSOR_LIGHT:
            sensorMinX = minXParam;
            sensorMaxX = maxXParam;
            sensorMinY = minYParam;
            sensorMaxY = maxYParam;
            sensorMinZ = minZParam;
            sensorMaxZ = maxZParam;
            break;
        case Sensor::SENSOR_TILT:
            sensorMinX = minXParam;
            sensorMaxX = maxXParam;
            sensorMinY = minYParam;
            sensorMaxY = maxYParam;
            sensorMinZ = minZParam;
            sensorMaxZ = maxZParam;
            break;
        case Sensor::SENSOR_SOUND:
            sensorMinX = minXParam;
            sensorMaxX = maxXParam;
            sensorMinY = minYParam;
            sensorMaxY = maxYParam;
            sensorMinZ = minZParam;
            sensorMaxZ = maxZParam;
            break;
        case Sensor::SENSOR_COLOR:
            sensorMinX = minXParam;
            sensorMaxX = maxXParam;
            sensorMinY = minYParam;
            sensorMaxY = maxYParam;
            sensorMinZ = minZParam;
            sensorMaxZ = maxZParam;
            break;
        case Sensor::SENSOR_GYR:
            sensorMinX = minXParam;
            sensorMaxX = maxXParam;
            sensorMinY = minYParam;
            sensorMaxY = maxYParam;
            sensorMinZ = minZParam;
            sensorMaxZ = maxZParam;
            break;
    }
}

std::string PhoneSensor::getQueryParams(Sensor sensor) {
    switch (sensor)
    {
        case PhoneSensor::SENSOR_MAG:
            return "magX&magY&magZ";
        case PhoneSensor::SENSOR_ACC:
            return "accX&accY&accZ";
        case PhoneSensor::SENSOR_LIGHT:
            return "illum";
        case PhoneSensor::SENSOR_TILT:
            return "tiltFlatUD&tiltFlatLR";
        case PhoneSensor::SENSOR_SOUND:
            return "dB";
        case PhoneSensor::SENSOR_COLOR:
            return "h&s&v";
        case PhoneSensor::SENSOR_GYR:
            return "gyrX&gyrY&gyrZ";
        default:
            return "";
    }
}

static std::string getType(const PhoneSensor::Sensor sensor, PhoneSensor::Coord coord) {
    std::string type = "";
    std::string arg = "";
    switch (sensor)
    {
        case PhoneSensor::SENSOR_MAG:
            arg = "X";
            if (coord == PhoneSensor::COORD_Y) {
                arg = "Y";
            } else if (coord == PhoneSensor::COORD_Z) {
                arg = "Z";
            }
            type.append("mag").append(arg);
            return type;
        case PhoneSensor::SENSOR_ACC:
            arg = "X";
            if (coord == PhoneSensor::COORD_Y) {
                arg = "Y";
            } else if (coord == PhoneSensor::COORD_Z) {
                arg = "Z";
            }
            type.append("acc").append(arg);
            return type;
        case PhoneSensor::SENSOR_LIGHT:
            return "illum";
        case PhoneSensor::SENSOR_TILT:
            arg = coord == PhoneSensor::COORD_X ? "UD" : "LR";
            return type.append("tiltFlat").append(arg);
        case PhoneSensor::SENSOR_SOUND:
            return "dB";
        case PhoneSensor::SENSOR_COLOR:
            arg = "h";
            if (coord == PhoneSensor::COORD_Y) {
                arg = "s";
            } else if (coord == PhoneSensor::COORD_Z) {
                arg = "v";
            }
            return arg;
        case PhoneSensor::SENSOR_GYR:
            arg = "X";
            if (coord == PhoneSensor::COORD_Y) {
                arg = "Y";
            } else if (coord == PhoneSensor::COORD_Z) {
                arg = "Z";
            }
            type.append("gyr").append(arg);
            return type;
        default:
            return "";
    }
}

static float getBufferValue(const json j, const PhoneSensor::Sensor sensor, PhoneSensor::Coord coord) {
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

static SensorValues getValue(const json j, const PhoneSensor::Sensor sensor) {
    float valueX = getBufferValue(j, sensor, PhoneSensor::COORD_X);
    float valueY = getBufferValue(j, sensor, PhoneSensor::COORD_Y);
    float valueZ = getBufferValue(j, sensor, PhoneSensor::COORD_Z);
    SensorValues v;
    switch (sensor) {
        case PhoneSensor::SENSOR_MAG:
        case PhoneSensor::SENSOR_ACC:
        case PhoneSensor::SENSOR_LIGHT:
        case PhoneSensor::SENSOR_TILT:
        case PhoneSensor::SENSOR_SOUND:
        case PhoneSensor::SENSOR_GYR:
            v.x = valueX;
            v.y = valueY;
            v.z = valueZ;
            return v;
        case PhoneSensor::SENSOR_COLOR:
            RGB rgb = hsv_to_rgb(valueX, valueY, valueZ);
            v.x = rgb.r;
            v.y = rgb.g;
            v.z = rgb.b;
            return v;
    };

    return v;
}

float PhoneSensor::calculateOutputVoltage(float rawValue, float rawMin, float rawMax) {
    switch (voltageMode) {
        case VoltageMode::BIPOLAR:
            return scaleAndClamp(rawValue, rawMin, rawMax, -5.0f, 5.0f);
        case VoltageMode::UNIPOLAR:
        default:
            return scaleAndClamp(rawValue, rawMin, rawMax, 0.f, 10.f);
    }
}

PhoneSensor::Status PhoneSensor::getStatus(bool hasError, bool isMeasuring) {
    if (hasError) {
        return ERROR;
    }

    if (isMeasuring) {
        return MEASURING;
    }

    return NOT_MEASURING;
}

void PhoneSensor::setStatusLedColor(float red, float green, float blue) {
    lights[STATUS_LIGHT_RED].setBrightness(red);
    lights[STATUS_LIGHT_GREEN].setBrightness(green);
    lights[STATUS_LIGHT_BLUE].setBrightness(blue);
}

void PhoneSensor::updateLedColor() {
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

void fetchHttpAsync(PhoneSensor* module, int requestId) {
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

json_t* PhoneSensor::dataToJson() {
    json_t* rootJson = json_object();
    json_object_set_new(rootJson, ipJsonKey.c_str(), json_string(ip.c_str()));
    json_object_set_new(rootJson, sensorModeParamJsonKey.c_str(), json_integer(sensorModeParam));
    json_object_set_new(rootJson, voltageModeJsonKey.c_str(), json_integer(voltageMode));

    json_object_set_new(rootJson, xMinJsonKey.c_str(), json_real(sensorMinX));
    json_object_set_new(rootJson, xMaxJsonKey.c_str(), json_real(sensorMaxX));
    json_object_set_new(rootJson, yMinJsonKey.c_str(), json_real(sensorMinY));
    json_object_set_new(rootJson, yMaxJsonKey.c_str(), json_real(sensorMaxY));
    json_object_set_new(rootJson, zMinJsonKey.c_str(), json_real(sensorMinZ));
    json_object_set_new(rootJson, zMaxJsonKey.c_str(), json_real(sensorMaxZ));

    if (debug) {
        char* jsonStr = json_dumps(rootJson, JSON_INDENT(2));
        cout << "saving " << jsonStr << endl;
    }

    return rootJson;
}

void PhoneSensor::dataFromJson(json_t* rootJson)  {
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

    json_t* xMinJson = json_object_get(rootJson, xMinJsonKey.c_str());
    if (xMinJson) {
        minXParam = (float) json_real_value(xMinJson);
    }
    json_t* xMaxJson = json_object_get(rootJson, xMaxJsonKey.c_str());
    if (xMaxJson) {
        maxXParam = (float) json_real_value(xMaxJson);
    }
    json_t* yMinJson = json_object_get(rootJson, yMinJsonKey.c_str());
    if (yMinJson) {
        minYParam = (float) json_real_value(yMinJson);
    }
    json_t* yMaxJson = json_object_get(rootJson, yMaxJsonKey.c_str());
    if (yMaxJson) {
        maxYParam = (float) json_real_value(yMaxJson);
    }
    json_t* zMinJson = json_object_get(rootJson, zMinJsonKey.c_str());
    if (zMinJson) {
        minZParam = (float) json_real_value(zMinJson);
    }
    json_t* zMaxJson = json_object_get(rootJson, zMaxJsonKey.c_str());
    if (xMaxJson) {
        maxZParam = (float) json_real_value(zMaxJson);
    }

    loadedFromJson = true;
}

void PhoneSensor::initSensor() {
    initUrl();
    if (loadedFromJson) {
        initLimitsFromJson();
    } else {
        initLimitsFromDefaults();
    }
}

void PhoneSensor::process(const ProcessArgs& args) {
		timeSinceLastRequest += args.sampleTime;
		if (!isFetching && timeSinceLastRequest >= sampleRate) {
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

Model* modelPhoneSensor = createModel<PhoneSensor, PhoneSensorWidget>("PhoneSensor");
