#pragma once
#include "../plugin.hpp"
#include "KinectSensorWidget.hpp"
#include "HandTracker.hpp"
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

using namespace std;

using namespace rack;

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

    KinectSensor();
    ~KinectSensor();

    void startKinectThread();
    void process(const ProcessArgs& args) override;
    int toDegrees(TiltDegrees tilt);
};