#include "stdafxClientFramework.h"
#include "AudioGlobal.h"

#include <fstream>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

class AudioGlobal::Impl
{
public:
	struct PlayingVoice
	{
		IXAudio2SourceVoice* voice = nullptr;
		std::vector<BYTE> audioData;
		AudioBus bus = AudioBus::SFX;
	};

	IXAudio2* engine = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
	std::vector<PlayingVoice> playingVoices;
	std::array<float, 3> busVolumes{1.0f, 1.0f, 1.0f};
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
	StopAll();

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

bool AudioGlobal::Play2D(const std::filesystem::path& filePath, AudioBus bus)
{
	if (!m_impl->engine || !m_impl->masteringVoice)
		return false;

	for (auto it = m_impl->playingVoices.begin(); it != m_impl->playingVoices.end();)
	{
		XAUDIO2_VOICE_STATE state{};
		it->voice->GetState(&state);
		if (state.BuffersQueued == 0)
		{
			it->voice->DestroyVoice();
			it = m_impl->playingVoices.erase(it);
		}
		else
		{
			++it;
		}
	}

	std::ifstream file(filePath, std::ios::binary);
	if (!file)
		return false;

	auto readFourCC = [&file](char (&value)[4]) { return static_cast<bool>(file.read(value, sizeof(value))); };
	auto readUInt32 = [&file](uint32_t& value)
	{
		return static_cast<bool>(file.read(reinterpret_cast<char*>(&value), sizeof(value)));
	};

	char riff[4]{};
	char wave[4]{};
	uint32_t riffSize = 0;
	if (!readFourCC(riff) || !readUInt32(riffSize) || !readFourCC(wave) || std::memcmp(riff, "RIFF", 4) != 0 ||
		std::memcmp(wave, "WAVE", 4) != 0)
	{
		return false;
	}

	std::vector<BYTE> formatData;
	std::vector<BYTE> audioData;
	while (file && (formatData.empty() || audioData.empty()))
	{
		char chunkId[4]{};
		uint32_t chunkSize = 0;
		if (!readFourCC(chunkId) || !readUInt32(chunkSize))
			break;

		if (std::memcmp(chunkId, "fmt ", 4) == 0)
		{
			formatData.resize(std::max<size_t>(chunkSize, sizeof(WAVEFORMATEX)));
			if (!file.read(reinterpret_cast<char*>(formatData.data()), chunkSize))
				return false;
			if (chunkSize == 16)
				reinterpret_cast<WAVEFORMATEX*>(formatData.data())->cbSize = 0;
		}
		else if (std::memcmp(chunkId, "data", 4) == 0)
		{
			audioData.resize(chunkSize);
			if (!file.read(reinterpret_cast<char*>(audioData.data()), chunkSize))
				return false;
		}
		else
		{
			file.seekg(chunkSize, std::ios::cur);
		}

		if ((chunkSize & 1u) != 0)
			file.seekg(1, std::ios::cur);
	}

	if (formatData.size() < sizeof(WAVEFORMATEX) || audioData.empty())
		return false;

	IXAudio2SourceVoice* voice = nullptr;
	if (FAILED(m_impl->engine->CreateSourceVoice(
			&voice, reinterpret_cast<const WAVEFORMATEX*>(formatData.data())
		)))
	{
		return false;
	}

	const size_t busIndex = static_cast<size_t>(bus);
	voice->SetVolume(m_impl->busVolumes[busIndex]);

	XAUDIO2_BUFFER buffer{};
	buffer.AudioBytes = static_cast<UINT32>(audioData.size());
	buffer.pAudioData = audioData.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	if (FAILED(voice->SubmitSourceBuffer(&buffer)) || FAILED(voice->Start()))
	{
		voice->DestroyVoice();
		return false;
	}

	m_impl->playingVoices.push_back({voice, std::move(audioData), bus});
	return true;
}

void AudioGlobal::StopAll()
{
	for (auto& playing : m_impl->playingVoices)
	{
		playing.voice->Stop();
		playing.voice->DestroyVoice();
	}
	m_impl->playingVoices.clear();
}

void AudioGlobal::SetBusVolume(AudioBus bus, float volume)
{
	const float clampedVolume = std::clamp(volume, 0.0f, 1.0f);
	m_impl->busVolumes[static_cast<size_t>(bus)] = clampedVolume;
	for (auto& playing : m_impl->playingVoices)
	{
		if (playing.bus == bus)
			playing.voice->SetVolume(clampedVolume);
	}
}
