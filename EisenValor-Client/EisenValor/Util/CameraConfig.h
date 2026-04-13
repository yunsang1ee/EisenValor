#pragma once

namespace CameraConfig
{
	// Camera Y height used across different camera modes (lock-on, shoulder, default)
	static constexpr float kCameraHeight = 2.5f;

	// Default local offset components for restoring camera on unlock
	static constexpr float kDefaultLocalOffsetX = 1.0f;
	static constexpr float kDefaultLocalOffsetZ = -5.0f;

	static constexpr float kShoulderViewOffsetX = 1.5f;
	static constexpr float kShoulderViewOffsetY = 1.5f;
	static constexpr float kShoulderViewOffsetZ = -2.0f;
	} // namespace CameraConfig
