 #pragma once

namespace CameraConfig
{
    // Camera Y height used across different camera modes (lock-on, shoulder, default)
    static constexpr float kCameraHeight = 3.0f;

    // Default local offset components for restoring camera on unlock
    static constexpr float kDefaultLocalOffsetZ = -5.0f;
}
