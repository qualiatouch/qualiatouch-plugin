#include <libfreenect.hpp>
#include <mutex>
#include <vector>

class HandTracker : public Freenect::FreenectDevice {
    public:
        HandTracker(freenect_context* ctx, int index);
        void DepthCallback(void* depth, uint32_t timestamp) override;
        bool getHandPosition(int& out_x, int& out_y, int& out_depth_mm);
        void start();

    private:
        std::vector<uint16_t> depth_data;
        std::vector<uint8_t> depth_buffer;
        std::mutex mutex;
        bool new_frame;
};
