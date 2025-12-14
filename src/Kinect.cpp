#include "plugin.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <limits>
#include <libfreenect.hpp>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "HandTracker.hpp"

using namespace std;

const float alpha = 0.9f;

float smoothedX = 0.f;
float smoothedY = 0.f;
float smoothedD = 0.f;

// mm
float lowerThreshold = 40 * 10.f;
float upperThreshold = 45 * 10.f;

bool thresholdPassed = false;

static float scaleAndClamp(
    float rawValue,
    float rawMin, float rawMax,
    float outMin, float outMax,
    bool invert = false
) {
    float normalized = (rawValue - rawMin) / (rawMax - rawMin);

    float scaled = normalized * (outMax - outMin) + outMin;

    if (invert) {
        scaled = outMax - (scaled - outMin);
    }

    return clamp(scaled, outMin, outMax);
}

struct KinectSensor : Module {
    enum ParamIds {
        THRESHOLD_PARAM,
        NUM_PARAMS
    };

    enum OutputIds {
        OUT_HAND_X,
        OUT_HAND_Y,
        OUT_HAND_DEPTH,
        OUT_HAND_DEPTH_THRESHOLD,
        NUM_OUTPUTS
    };

	float phase = 0.f;

	float timeSinceLastLoop = 0.f;

    std::atomic<float> handX{0.f};
    std::atomic<float> handY{0.f};
    std::atomic<float> handZ{0.f};
    std::thread kinectThread;
    std::atomic<bool> running{true};

    KinectSensor() {
        config(NUM_PARAMS, 0, NUM_OUTPUTS);

        configParam(THRESHOLD_PARAM, 100.f, 1000.f, 500.f, "Depth threshold", " mm");

        configOutput(OUT_HAND_X, "Hand x");
        configOutput(OUT_HAND_Y, "Hand y");
        configOutput(OUT_HAND_DEPTH, "Hand depth");
        configOutput(OUT_HAND_DEPTH_THRESHOLD, "Hand depth threshold attained");

        startKinectThread();
    }

    ~KinectSensor() {
        cout << "destroying Kinect" << endl;
        running = false;
        if (kinectThread.joinable())
            kinectThread.join();
    }

    void startKinectThread();
    void process(const ProcessArgs& args) override;
};

void KinectSensor::startKinectThread() {
    cout << "startKinectThread" << endl;
    kinectThread = std::thread([this]() {
        Freenect::Freenect freenect;
        cout << "creating device" << endl;
        HandTracker& device = freenect.createDevice<HandTracker>(0);
        cout << "starting device" << endl;
        device.startDepth();

        cout << "starting loop" << endl;
        while (running) {
            int x, y, z;
            if (device.getHandPosition(x, y, z)) {
                //cout << "got hand position " << x << " " << y << " " << z << endl;
                handX = static_cast<float>(x);
                handY = static_cast<float>(y);
                handZ = static_cast<float>(z);
            } else {
                cout << "did not get hand positin" << endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30)); // todo param
        }
        cout << "end loop" << endl;
    });
    cout << "end startKinectThread" << endl;
}

void KinectSensor::process(const ProcessArgs& args) {
    timeSinceLastLoop += args.sampleTime;
    if (timeSinceLastLoop < 0.03f) {
        return;
    }

    float threshold_mm = params[THRESHOLD_PARAM].getValue();
    cout << "threshold_mm " << threshold_mm << endl;

    lowerThreshold = threshold_mm;
    upperThreshold = lowerThreshold + 50;

    cout << "before getting data" << endl;
    float rawX = handX;
    float rawY = handY;
    float rawDepth = handZ;

    float normX = scaleAndClamp(rawX, 0, 640, 0, 10);
    float normY = scaleAndClamp(rawY, 0, 480, 0, 10, true);
    float normD = scaleAndClamp(rawDepth, 0, 1000, 0, 10); // adjust

    smoothedX = alpha * normX + (1 - alpha) * smoothedX;
    smoothedY = alpha * normY + (1 - alpha) * smoothedY;
    smoothedD = alpha * normD + (1 - alpha) * smoothedD;

    cout << "threshold param = " << threshold_mm << " ; lower = " << lowerThreshold << " ; upper = " << upperThreshold << "rawDepth = " << rawDepth << " rawDepth < lowerThreshold = " << (rawDepth < lowerThreshold ? "1" : "0") << "rawDepth > upperThreshold = " << (rawDepth > upperThreshold ? "1" : "0") << endl;

    if (rawDepth < lowerThreshold) {
        thresholdPassed = true;
    } else if (rawDepth > upperThreshold) {
        thresholdPassed = false;
    }

    cout << "got voltages " << normX << " " << normY << " " << normD << endl;
    outputs[OUT_HAND_X].setVoltage(normX);
    outputs[OUT_HAND_Y].setVoltage(normY);
    outputs[OUT_HAND_DEPTH].setVoltage(normD);
    outputs[OUT_HAND_DEPTH_THRESHOLD].setVoltage(thresholdPassed ? 5.f : 0.f);

    timeSinceLastLoop = 0.0f;
}

struct KinectSensorWidget : ModuleWidget {
    KinectSensorWidget(KinectSensor* module) {
        setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/kinect.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 2, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(7.625, 110)), module, KinectSensor::THRESHOLD_PARAM));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 30)), module, KinectSensor::OUT_HAND_X));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 50)), module, KinectSensor::OUT_HAND_Y));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 70)), module, KinectSensor::OUT_HAND_DEPTH));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 90)), module, KinectSensor::OUT_HAND_DEPTH_THRESHOLD));
    }
};

Model* modelKinectSensor = createModel<KinectSensor, KinectSensorWidget>("KinectSensor");
