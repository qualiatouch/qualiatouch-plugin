#pragma once
#include "../plugin.hpp"
//#include "PhyPhoxWidget.hpp"
#include <curl/curl.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>

#include "../util/json.hpp"

using namespace std;

using namespace rack;

struct PhyPhoxWidget;

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
        SENSOR_COLOR = 5,
        SENSOR_GYR = 6
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
    std::string xMinJsonKey = "xMin";
    std::string xMaxJsonKey = "xMax";
    std::string yMinJsonKey = "yMin";
    std::string yMaxJsonKey = "yMax";
    std::string zMinJsonKey = "zMin";
    std::string zMaxJsonKey = "zMax";

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

    const float DEFAULT_MIN_X_GYR = -20;
    const float DEFAULT_MAX_X_GYR = 20;
    const float DEFAULT_MIN_Y_GYR = -20;
    const float DEFAULT_MAX_Y_GYR = 20;
    const float DEFAULT_MIN_Z_GYR = -20;
    const float DEFAULT_MAX_Z_GYR = 20;

    float sensorMinX = DEFAULT_MIN_X_MAG;
    float sensorMaxX = DEFAULT_MAX_X_MAG;
    float sensorMinY = DEFAULT_MIN_Y_MAG;
    float sensorMaxY = DEFAULT_MAX_Y_MAG;
    float sensorMinZ = DEFAULT_MIN_Z_MAG;
    float sensorMaxZ = DEFAULT_MAX_Z_MAG;

    float minXParam;
    float maxXParam;
    float minYParam;
    float maxYParam;
    float minZParam;
    float maxZParam;

    bool loadedFromJson = false;

    float sampleRate = 0.01f;
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

    PhyPhoxSensor();

    void setWidget(PhyPhoxWidget* widgetParam);

    void initUrl();
    void initLimitsFromDefaults();
    void initLimitsFromJson();
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
