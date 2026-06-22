#pragma once

#include "Singleton.h"

#include <filesystem>
#include <memory>

enum class AudioBus
{
	BGM,
	SFX,
	UI
};

class AudioGlobal : public Singleton<AudioGlobal>
{
private:
	friend class Singleton<AudioGlobal>;

	AudioGlobal();
	~AudioGlobal() override;

public:
	void Initialize() override;
	void Release() override;

	bool Play2D(const std::filesystem::path& filePath, AudioBus bus = AudioBus::SFX, bool loop = false);
	void StopAll();
	void StopBus(AudioBus bus);
	void SetBusVolume(AudioBus bus, float volume);

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};
