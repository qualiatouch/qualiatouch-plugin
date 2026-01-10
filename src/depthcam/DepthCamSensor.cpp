#include "DepthCamSensor.hpp"
#include "../util/utils.hpp"

using namespace std;

using namespace utils;

const float alpha = 0.9f;

float smoothedX = 0.f;
float smoothedY = 0.f;
float smoothedD = 0.f;

// mm
float lowerThreshold = 40 * 10.f;
float upperThreshold = 45 * 10.f;

bool thresholdPassed = false;

DepthCamSensor::DepthCamSensor() {
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

DepthCamSensor::~DepthCamSensor() {
    running = false;
    if (kinectThread.joinable()) {
        kinectThread.join();
    }
}

int DepthCamSensor::toDegrees(TiltDegrees tilt) {
    return tilt * 5;
}

void DepthCamSensor::startKinectThread() {
    kinectThread = std::thread([this]() {
        if (debug) {
            cout << "starting device" << endl;
        }

        Freenect::Freenect freenect;
        HandTracker& device = freenect.createDevice<HandTracker>(0);

        hasDevice = true;

        device.startDepth();

        while (running) {
            if (tiltRequest != currentTilt) {
                currentTilt = tiltRequest;
                if (debug) {
                    cout << "setTiltDegrees " << tiltRequest << endl;
                }

                try {
                    device.setTiltDegrees(toDegrees(currentTilt));
                } catch (std::runtime_error& e) {
                    cerr << "failed to tilt device : " << e.what() << endl;
                }
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
            std::this_thread::sleep_for(std::chrono::milliseconds(deviceSleep));
        }
        if (debug) {
            cout << "stopping device" << endl;
        }
        device.stopDepth();
    });
    if (debug) {
        cout << "end startKinectThread" << endl;
    }
}

void DepthCamSensor::process(const ProcessArgs& args) {
    if (!hasDevice) {
        return;
    }

    timeSinceLastLoop += args.sampleTime;
    if (timeSinceLastLoop < processPeriod) {
        return;
    }

    float threshold_mm = params[THRESHOLD_PARAM].getValue();
    if (debug) {
        //cout << "threshold_mm " << threshold_mm << endl;
    }

    lowerThreshold = threshold_mm;
    upperThreshold = lowerThreshold + hysteresisRange;

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

Model* modelDepthCamSensor = createModel<DepthCamSensor, DepthCamSensorWidget>("DepthCamSensor");
