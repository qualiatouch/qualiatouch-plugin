// HandTracker.cpp
#include <libfreenect.hpp>
#include <vector>
#include <mutex>
#include <thread>
#include <iostream>

#include "HandTracker.hpp"

using namespace std;

HandTracker::HandTracker(freenect_context* ctx, int index)
    : Freenect::FreenectDevice(ctx, index), depth_buffer(FREENECT_DEPTH_11BIT_PACKED), new_frame(false) {
    depth_data.resize(640 * 480);
}

void HandTracker::DepthCallback(void* depth, uint32_t timestamp) {
    std::lock_guard<std::mutex> lock(mutex);
    uint16_t* depth_ptr = static_cast<uint16_t*>(depth);
    std::copy(depth_ptr, depth_ptr + 640 * 480, depth_data.begin());
    new_frame = true;
}

bool HandTracker::getHandPosition(int& out_x, int& out_y, int& out_depth_mm) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!new_frame) return false;

    int min_depth = std::numeric_limits<int>::max();
    int best_x = -1, best_y = -1;

    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            int idx = y * 640 + x;
            int d = depth_data[idx];
            if (d > 0 && d < min_depth && d < 1000) {
                min_depth = d;
                best_x = x;
                best_y = y;
            }
        }
    }

    if (best_x == -1) return false;

    out_x = best_x;
    out_y = best_y;
    out_depth_mm = min_depth;
    new_frame = false;
    return true;
}

void HandTracker::start() {
    startDepth();
}
