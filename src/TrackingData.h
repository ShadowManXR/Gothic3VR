#pragma once
#include <cstdint>
namespace diagnostic
{
struct HeadPose
{
    int64_t qpc = 0;
    uint64_t session = 0; // Assigned by the publishing worker on each successful connection.
    bool runtimeConnected = false, deviceConnected = false, valid = false;
    int trackingResult = 0;
    char origin[16] = "standing";
    float transform[12]{}, position[3]{}, quaternion[4]{};
};
struct HeadsetInfo
{
    char model[128]{}, manufacturer[128]{}, trackingSystem[128]{};
    uint32_t recommendedWidth = 0, recommendedHeight = 0;
    bool opticsValid = false;
    float eyeToHead[24]{}, projectionRaw[8]{};
};
}
