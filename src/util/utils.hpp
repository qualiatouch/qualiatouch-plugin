 #pragma once
 #include <algorithm>

namespace utils {
    inline float scaleAndClamp(
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
}