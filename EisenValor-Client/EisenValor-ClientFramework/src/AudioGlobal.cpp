#include "stdafxClientFramework.h"
#include "AudioGlobal.h"

#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

class AudioGlobal::Impl
{
public:
	IXAudio2* engine = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
};

AudioGlobal::AudioGlobal() : m_impl(std::make_unique<Impl>()) {}

AudioGlobal::~AudioGlobal()
{
	Release();
}

void AudioGlobal::Initialize()
{
	if (m_impl->engine)
		return;

	if (FAILED(XAudio2Create(&m_impl->engine)))
		return;

	if (FAILED(m_impl->engine->CreateMasteringVoice(&m_impl->masteringVoice)))
	{
		m_impl->engine->Release();
		m_impl->engine = nullptr;
	}
}

void AudioGlobal::Release()
{
	if (m_impl->masteringVoice)
	{
		m_impl->masteringVoice->DestroyVoice();
		m_impl->masteringVoice = nullptr;
	}

	if (m_impl->engine)
	{
		m_impl->engine->Release();
		m_impl->engine = nullptr;
	}
}

bool AudioGlobal::Play2D(const std::filesystem::path&, AudioBus)
{
	return false;
}

void AudioGlobal::StopAll() {}

void AudioGlobal::SetBusVolume(AudioBus, float) {}
