#pragma once

#include "RestirDebugGlobal.h"

#include <DxFrameResource.h>
#include <DxCommandContext.h>
#include <DxTexture.h>
#include <DxUtils.h>
#include <CameraRenderData.h>
#include <DirectXPackedVector.h>
#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

class RestirHdrCapture
{
	struct Slot
	{
		ComPtr<ID3D12Resource> buffer;
		ComPtr<ID3D12Fence> fence;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		uint64_t fenceValue = 0;
		uint64_t session = 0;
		uint32_t ordinal = 0;
		bool pending = false;
	};

public:
	void PollCompleted() { Poll(); }
	void Cancel()
	{
		m_active = false;
		GLOBAL(RestirDebugGlobal).SetCaptureStatus(L"FAILED", m_received, m_targetFrames);
	}

	void Execute(
		DxFrameResource*		frame,
		DxTexture*				texture,
		const CameraRenderData* camera,
		uint64_t				request,
		uint64_t				revision,
		uint64_t				signature,
		const char*				source,
		uint32_t				spp,
		bool					allowed,
		bool					rrSnapshot = false
	)
	{
		if (m_active && (!allowed || !camera || revision != m_revision || signature != m_signature ||
			texture->GetWidth() != m_width || texture->GetHeight() != m_height ||
			std::memcmp(&camera->viewMatrix, &m_view, sizeof(m_view)) != 0 ||
			std::memcmp(&camera->projectionMatrix, &m_projection, sizeof(m_projection)) != 0))
		{
			DEBUG_LOG_FMT("[ReSTIR.Capture] cancelled: camera, mode, scene, or settings changed.\n");
			m_active = false;
			GLOBAL(RestirDebugGlobal).SetCaptureStatus(L"CANCELLED: SCENE CHANGED", m_received, m_targetFrames);
		}
		Poll();
		if (request != m_request)
		{
			m_request = request;
			m_active = false;
			if (allowed && camera && texture->GetFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT)
			{
				m_targetFrames = rrSnapshot ? 1u : 512u;
				++m_session;
				m_revision = revision;
				m_signature = signature;
				m_width = texture->GetWidth();
				m_height = texture->GetHeight();
				m_view = camera->viewMatrix;
				m_projection = camera->projectionMatrix;
				m_submitted = m_received = 0;
				m_invalid = 0;
				m_peak = 0.0;
				m_sum.assign(size_t(m_width) * m_height * 3, 0.0);
				const auto stamp = std::chrono::system_clock::now().time_since_epoch().count();
				m_directory = std::filesystem::path("Captures") / (rrSnapshot ? "RR" : "RestirHDR") /
							  (std::to_string(stamp) + "_" + source);
				std::filesystem::create_directories(m_directory);
				std::ofstream metadata(m_directory / "capture.txt");
				metadata.exceptions(std::ios::failbit | std::ios::badbit);
				metadata << "source=" << source
						 << (rrSnapshot ? "\nformat=linear RGB after RR, before tonemap; requested preset (driver "
										  "override not detected)"
										: "\nformat=linear RGB from R16G16B16A16_FLOAT, before RR/tonemap")
						 << "\nwidth=" << m_width << "\nheight=" << m_height << "\nhistory_signature=" << signature
						 << "\nspp=" << spp << "\ncamera=" << camera->cameraPosition.x << ','
						 << camera->cameraPosition.y << ',' << camera->cameraPosition.z
						 << "\ndirection=" << camera->cameraDirection.x << ',' << camera->cameraDirection.y << ','
						 << camera->cameraDirection.z << "\nfov=" << camera->fov << "\naspect=" << camera->aspectRatio
						 << "\nAnimated objects are NOT frozen. Frame variance includes animation and jitter.\n";
				metadata.close();
				m_active = true;
				GLOBAL(RestirDebugGlobal).SetCaptureStatus(L"CAPTURING", 0, m_targetFrames);
				DEBUG_LOG_FMT("[ReSTIR.Capture] started: {}\n", m_directory.string());
			}
			else
			{
				GLOBAL(RestirDebugGlobal).SetCaptureStatus(L"UNAVAILABLE", 0);
			}
		}
		if (!m_active || m_submitted == m_targetFrames)
		{
			return;
		}
		for (auto& slot : m_slots)
		{
			if (slot.pending)
			{
				continue;
			}
			ComPtr<ID3D12Device> device;
			ThrowIfFailed(texture->GetResource()->GetDevice(IID_PPV_ARGS(&device)));
			auto desc = texture->GetResource()->GetDesc();
			uint64_t bytes = 0;
			device->GetCopyableFootprints(&desc, 0, 1, 0, &slot.footprint, nullptr, nullptr, &bytes);
			if (!slot.buffer || slot.buffer->GetDesc().Width < bytes)
			{
				slot.buffer.Reset();
				D3D12_HEAP_PROPERTIES heap{};
				heap.Type = D3D12_HEAP_TYPE_READBACK;
				heap.CreationNodeMask = heap.VisibleNodeMask = 1;
				D3D12_RESOURCE_DESC buffer{};
				buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				buffer.Width = bytes;
				buffer.Height = buffer.DepthOrArraySize = buffer.MipLevels = 1;
				buffer.SampleDesc.Count = 1;
				buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				ThrowIfFailed(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
					D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&slot.buffer)));
			}
			auto* list = frame->GetMainContext()->CommandList();
			DxUtils::TransitionResourceIfNeeded(list, texture, D3D12_RESOURCE_STATE_COPY_SOURCE);
			D3D12_TEXTURE_COPY_LOCATION src{};
			src.pResource = texture->GetResource();
			src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			D3D12_TEXTURE_COPY_LOCATION dst{};
			dst.pResource = slot.buffer.Get();
			dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			dst.PlacedFootprint = slot.footprint;
			list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
			DxUtils::TransitionResourceIfNeeded(list, texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			slot.fence = frame->GetFence();
			slot.fenceValue = frame->GetFenceValue() + 1;
			slot.session = m_session;
			slot.ordinal = ++m_submitted;
			slot.pending = true;
			return;
		}
		m_active = false;
		GLOBAL(RestirDebugGlobal).SetCaptureStatus(L"CANCELLED: READBACK FULL", m_received, m_targetFrames);
		DEBUG_LOG_FMT("[ReSTIR.Capture] cancelled: readback queue full.\n");
	}

private:
	void Poll()
	{
		for (;;)
		{
			Slot* next = nullptr;
			for (auto& slot : m_slots)
			{
				if (slot.pending && slot.fence->GetCompletedValue() >= slot.fenceValue &&
					(!m_active || slot.session != m_session))
				{
					slot.pending = false;
				}
				if (slot.pending && slot.session == m_session && slot.ordinal == m_received + 1)
				{
					next = &slot;
				}
			}
			if (!m_active || !next || next->fence->GetCompletedValue() < next->fenceValue)
			{
				return;
			}
			void* mapped = nullptr;
			D3D12_RANGE range{0, size_t(next->buffer->GetDesc().Width)};
			ThrowIfFailed(next->buffer->Map(0, &range, &mapped));
			for (uint32_t y = 0; y < m_height; ++y)
			{
				const auto* row = reinterpret_cast<const uint16_t*>(static_cast<const char*>(mapped) +
					next->footprint.Offset + size_t(y) * next->footprint.Footprint.RowPitch);
				for (uint32_t x = 0; x < m_width; ++x)
				{
					for (uint32_t c = 0; c < 3; ++c)
					{
						const float value = DirectX::PackedVector::XMConvertHalfToFloat(row[x * 4 + c]);
						if (!std::isfinite(value))
						{
							++m_invalid;
							continue;
						}
						m_sum[(size_t(y) * m_width + x) * 3 + c] += value;
						m_peak = (std::max)(m_peak, double(value));
					}
				}
			}
			D3D12_RANGE written{0, 0};
			next->buffer->Unmap(0, &written);
			next->pending = false;
			++m_received;
			GLOBAL(RestirDebugGlobal).SetCaptureStatus(L"CAPTURING", m_received, m_targetFrames);
			if (m_received == 1 || m_received == 32 || m_received == 128 || m_received == m_targetFrames)
			{
				Save();
			}
			if (m_received == m_targetFrames)
			{
				m_active = false;
				GLOBAL(RestirDebugGlobal)
					.SetCaptureStatus(m_invalid ? L"INVALID HDR VALUES" : L"SAVED", m_received, m_targetFrames);
				DEBUG_LOG_FMT("[ReSTIR.Capture] complete: {}\n", m_directory.string());
			}
		}
	}

	void Save()
	{
		const auto stem = "mean_" + std::to_string(m_received);
		std::ofstream image(m_directory / (stem + ".pfm"), std::ios::binary);
		image.exceptions(std::ios::failbit | std::ios::badbit);
		image << "PF\n" << m_width << ' ' << m_height << "\n-1.0\n";
		double meanLuminance = 0.0;
		for (uint32_t y = m_height; y-- > 0;)
		{
			for (uint32_t x = 0; x < m_width; ++x)
			{
				float rgb[3];
				for (uint32_t c = 0; c < 3; ++c)
				{
					rgb[c] = float(m_sum[(size_t(y) * m_width + x) * 3 + c] / m_received);
				}
				image.write(reinterpret_cast<const char*>(rgb), sizeof(rgb));
				meanLuminance += rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722;
			}
		}
		image.close();
		if (m_targetFrames == 1)
		{
			BITMAPFILEHEADER fileHeader{};
			BITMAPINFOHEADER info{};
			info.biSize = sizeof(info);
			info.biWidth = static_cast<LONG>(m_width);
			info.biHeight = static_cast<LONG>(m_height);
			info.biPlanes = 1;
			info.biBitCount = 32;
			info.biCompression = BI_RGB;
			info.biSizeImage = m_width * m_height * 4;
			fileHeader.bfType = 0x4d42;
			fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(info);
			fileHeader.bfSize = fileHeader.bfOffBits + info.biSizeImage;
			std::ofstream preview(m_directory / "preview.bmp", std::ios::binary);
			preview.exceptions(std::ios::failbit | std::ios::badbit);
			preview.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
			preview.write(reinterpret_cast<const char*>(&info), sizeof(info));
			std::vector<uint8_t> row(size_t(m_width) * 4, 255);
			for (uint32_t y = m_height; y-- > 0;)
			{
				for (uint32_t x = 0; x < m_width; ++x)
				{
					for (uint32_t c = 0; c < 3; ++c)
					{
						const double v = (std::max)(0.0, m_sum[(size_t(y) * m_width + x) * 3 + c]);
						const double aces = std::clamp((v * (2.51 * v + .03)) / (v * (2.43 * v + .59) + .14), 0.0, 1.0);
						row[x * 4 + 2 - c] = static_cast<uint8_t>(std::pow(aces, 1.0 / 2.2) * 255.0 + .5);
					}
				}
				preview.write(reinterpret_cast<const char*>(row.data()), row.size());
			}
			preview.close();
		}
		std::ofstream stats(m_directory / (stem + ".txt"));
		stats.exceptions(std::ios::failbit | std::ios::badbit);
		stats << "frames=" << m_received << "\nmean_luminance=" << meanLuminance / (double(m_width) * m_height)
			<< "\nraw_peak_channel=" << m_peak << "\nnonfinite_channels=" << m_invalid
			<< "\nNonfinite channels are excluded as zero; any nonzero count invalidates numerical comparison.\n";
		stats.close();
		DEBUG_LOG_FMT("[ReSTIR.Capture] saved {} frames, invalid={}\n", m_received, m_invalid);
	}

	std::array<Slot, 4> m_slots;
	std::vector<double> m_sum;
	std::filesystem::path m_directory;
	DirectX::XMMATRIX m_view{}, m_projection{};
	uint64_t m_request = 0, m_session = 0, m_revision = 0, m_signature = 0, m_invalid = 0;
	uint32_t			  m_targetFrames = 512;
	uint32_t m_width = 0, m_height = 0, m_submitted = 0, m_received = 0;
	double m_peak = 0.0;
	bool m_active = false;
};
