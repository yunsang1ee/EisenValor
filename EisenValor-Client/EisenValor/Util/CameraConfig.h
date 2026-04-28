#pragma once

namespace CameraConfig
{
	// Camera Y height used across different camera modes (lock-on, shoulder, default)
	static constexpr float kCameraHeight = 1.5f;

	// Default local offset components for restoring camera on unlock
	static constexpr float kDefaultLocalOffsetX = 1.0f;
	static constexpr float kDefaultLocalOffsetZ = -3.5f;

	////////////////////////////////////////////////////////

	static constexpr float kShoulderViewOffsetX = 1.5f;
	static constexpr float kShoulderViewOffsetY = 1.3f;
	static constexpr float kShoulderViewOffsetZ = -1.8f;

	static constexpr float kLockOnViewOffsetY = 1.0f;
	} // namespace CameraConfig
