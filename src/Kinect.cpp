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

    enum TiltDegrees {
        TILT_0,
        TILT_5,
        TILT_10,
        TILT_15,
        TILT_20,
        TILT_25,
        TILT_30
    };

    bool debug = false;

	float timeSinceLastLoop = 0.f;

    bool hasDevice = false;

    std::atomic<float> handX{0.f};
    std::atomic<float> handY{0.f};
    std::atomic<float> handZ{0.f};
    std::thread kinectThread;
    std::atomic<bool> running{true};

    TiltDegrees tiltRequest = TILT_0;
    TiltDegrees currentTilt = TILT_0;

    KinectSensor() {
        config(NUM_PARAMS, 0, NUM_OUTPUTS);

        configParam(THRESHOLD_PARAM, 100.f, 1000.f, 500.f, "Depth threshold", " mm");

        configOutput(OUT_HAND_X, "Hand x");
        configOutput(OUT_HAND_Y, "Hand y");
        configOutput(OUT_HAND_DEPTH, "Hand depth");
        configOutput(OUT_HAND_DEPTH_THRESHOLD, "Hand depth threshold attained");

        Freenect::Freenect freenect;
        int deviceCount = freenect.deviceCount();
        if (debug) {
            cout << "device count : " << freenect.deviceCount() << endl;
        }
        if (deviceCount < 1) {
            return;
        }

        if (debug) {
            cout << "creating device" << endl;
        }

        startKinectThread();
    }

    ~KinectSensor() {
        running = false;
        if (kinectThread.joinable()) {
            kinectThread.join();
        }
    }

    void startKinectThread();
    void process(const ProcessArgs& args) override;
    int toDegrees(TiltDegrees tilt);
};

int KinectSensor::toDegrees(TiltDegrees tilt) {
    return tilt * 5;
}

void KinectSensor::startKinectThread() {
    kinectThread = std::thread([this]() {
        if (debug) {
            cout << "starting device" << endl;
        }

        Freenect::Freenect freenect;
        HandTracker& device = freenect.createDevice<HandTracker>(0);

        hasDevice = true;

        device.startDepth();

        while (running) {
            //cout << "tiltRequest " << tiltRequest << " currentTilt " << currentTilt << endl;
            if (tiltRequest != currentTilt) {
                currentTilt = tiltRequest;
                if (debug) {
                    cout << "setTiltDegrees " << tiltRequest << endl;
                }

                device.setTiltDegrees(toDegrees(currentTilt));
            }

            int x, y, z;
            if (device.getHandPosition(x, y, z)) {
                if (debug) {
                    cout << "got hand position " << x << " " << y << " " << z << endl;
                }
                handX = static_cast<float>(x);
                handY = static_cast<float>(y);
                handZ = static_cast<float>(z);
            } else {
                // if (debug) {
                    cerr << "did not get hand position" << endl;
                // }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // todo param
        }
    });
    if (debug) {
        cout << "end startKinectThread" << endl;
    }
}

void KinectSensor::process(const ProcessArgs& args) {
    if (!hasDevice) {
        return;
    }

    timeSinceLastLoop += args.sampleTime;
    if (timeSinceLastLoop < 0.05f) {
        return;
    }

    float threshold_mm = params[THRESHOLD_PARAM].getValue();
    if (debug) {
        //cout << "threshold_mm " << threshold_mm << endl;
    }

    lowerThreshold = threshold_mm;
    upperThreshold = lowerThreshold + 50;

    if (debug) {
        //cout << "before getting data" << endl;
    }

    float rawX = handX;
    float rawY = handY;
    float rawDepth = handZ;

    float normX = scaleAndClamp(rawX, 0, 640, 0, 10);
    float normY = scaleAndClamp(rawY, 0, 480, 0, 10, true);
    float normD = scaleAndClamp(rawDepth, 0, 1000, 0, 10); // adjust

    smoothedX = alpha * normX + (1 - alpha) * smoothedX;
    smoothedY = alpha * normY + (1 - alpha) * smoothedY;
    smoothedD = alpha * normD + (1 - alpha) * smoothedD;

    if (debug) {
        //cout << "threshold param = " << threshold_mm << " ; lower = " << lowerThreshold << " ; upper = " << upperThreshold << "rawDepth = " << rawDepth << " rawDepth < lowerThreshold = " << (rawDepth < lowerThreshold ? "1" : "0") << "rawDepth > upperThreshold = " << (rawDepth > upperThreshold ? "1" : "0") << endl;
    }

    if (rawDepth < lowerThreshold) {
        thresholdPassed = true;
    } else if (rawDepth > upperThreshold) {
        thresholdPassed = false;
    }

    //cout << "got voltages " << normX << " " << normY << " " << normD << endl;
    outputs[OUT_HAND_X].setVoltage(normX);
    outputs[OUT_HAND_Y].setVoltage(normY);
    outputs[OUT_HAND_DEPTH].setVoltage(normD);
    outputs[OUT_HAND_DEPTH_THRESHOLD].setVoltage(thresholdPassed ? 5.f : 0.f);

    timeSinceLastLoop = 0.0f;
}

struct KinectSensorWidget : ModuleWidget {
    KinectSensor* module;
    KinectSensorWidget(KinectSensor* moduleParam) {
        module = moduleParam;
        setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/kinect.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 2, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(7.625, 110)), module, KinectSensor::THRESHOLD_PARAM));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 30)), module, KinectSensor::OUT_HAND_X));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 50)), module, KinectSensor::OUT_HAND_Y));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 70)), module, KinectSensor::OUT_HAND_DEPTH));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 90)), module, KinectSensor::OUT_HAND_DEPTH_THRESHOLD));
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createIndexPtrSubmenuItem("Sensor tilt",
            {
                "0°",
                "5°",
                "10°",
                "15°",
                "20°",
                "25°",
                "30°"
            },
            &module->tiltRequest));

        menu->addChild(rack::createBoolPtrMenuItem("Debug", "", &module->debug));

        if (module->debug) {
            menu->addChild(new MenuSeparator);
            menu->addChild(createMenuLabel("Debug info"));
            menu->addChild(createMenuLabel("HasDevice " + to_string(module->hasDevice)));
            menu->addChild(createMenuLabel("Current tilt " + to_string(module->currentTilt)));
            menu->addChild(createMenuLabel("Requested tilt " + to_string(module->tiltRequest)));
        }
    }
};

Model* modelKinectSensor = createModel<KinectSensor, KinectSensorWidget>("KinectSensor");
