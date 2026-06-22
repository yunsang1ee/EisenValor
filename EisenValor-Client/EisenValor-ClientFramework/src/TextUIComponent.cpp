#include "stdafxClientFramework.h"
#include "TextUIComponent.h"
#include "DxCommandQueueGlobal.h"
#include "DxDeviceGlobal.h"
#include "DxTexture.h"
#include "RectTransformComponent.h"
#include "TextureResource.h"
#include <algorithm>
#include <cmath>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace
{
	ComPtr<IDWriteFactory> GetDWriteFactory()
	{
		static ComPtr<IDWriteFactory> factory;
		if (factory)
		{
			return factory;
		}

		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(factory.GetAddressOf())
		);
		return factory;
	}

	ComPtr<ID2D1Factory> GetD2DFactory()
	{
		static ComPtr<ID2D1Factory> factory;
		if (factory)
		{
			return factory;
		}

		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
		return factory;
	}

	ComPtr<IWICImagingFactory> GetWICFactory()
	{
		static ComPtr<IWICImagingFactory> factory = []
		{
			ComPtr<IWICImagingFactory> createdFactory;
			CoInitializeEx(nullptr, COINIT_MULTITHREADED);

			const HRESULT hr = CoCreateInstance(
				CLSID_WICImagingFactory,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(createdFactory.GetAddressOf())
			);
			if (FAILED(hr))
			{
				createdFactory.Reset();
			}

			return createdFactory;
		}();
		return factory;
	}

	std::shared_ptr<TextureResource> CreateTextureResourceFromBGRA(
		const std::vector<uint8_t>& pixels,
		uint32_t width,
		uint32_t height
	)
	{
		if (pixels.empty() || width == 0 || height == 0)
		{
			return nullptr;
		}

		ID3D12Device* device = GLOBAL(DxDeviceGlobal).GetDevice();
		if (!device)
		{
			return nullptr;
		}

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_HEAP_PROPERTIES defaultHeap = {};
		defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

		ComPtr<ID3D12Resource> texture;
		HRESULT hr = device->CreateCommittedResource(
			&defaultHeap,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(texture.GetAddressOf())
		);
		if (FAILED(hr))
		{
			return nullptr;
		}

		const uint64_t sourceRowPitch = static_cast<uint64_t>(width) * 4;
		const uint64_t uploadRowPitch = (sourceRowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) &
										~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
		const uint64_t uploadSize = uploadRowPitch * height;

		D3D12_RESOURCE_DESC uploadDesc = {};
		uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadDesc.Width = uploadSize;
		uploadDesc.Height = 1;
		uploadDesc.DepthOrArraySize = 1;
		uploadDesc.MipLevels = 1;
		uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
		uploadDesc.SampleDesc.Count = 1;
		uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES uploadHeap = {};
		uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

		ComPtr<ID3D12Resource> upload;
		hr = device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&uploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(upload.GetAddressOf())
		);
		if (FAILED(hr))
		{
			return nullptr;
		}

		uint8_t* mapped = nullptr;
		hr = upload->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
		if (FAILED(hr))
		{
			return nullptr;
		}

		for (uint32_t y = 0; y < height; ++y)
		{
			memcpy(mapped + uploadRowPitch * y, pixels.data() + sourceRowPitch * y, sourceRowPitch);
		}
		upload->Unmap(0, nullptr);

		ComPtr<ID3D12CommandAllocator> allocator;
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(allocator.GetAddressOf()));
		if (FAILED(hr))
		{
			return nullptr;
		}

		ComPtr<ID3D12GraphicsCommandList> commandList;
		hr = device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			allocator.Get(),
			nullptr,
			IID_PPV_ARGS(commandList.GetAddressOf())
		);
		if (FAILED(hr))
		{
			return nullptr;
		}

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = texture.Get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = upload.Get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		src.PlacedFootprint.Footprint.Width = width;
		src.PlacedFootprint.Footprint.Height = height;
		src.PlacedFootprint.Footprint.Depth = 1;
		src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(uploadRowPitch);

		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = texture.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);
		commandList->Close();

		auto& queue = GLOBAL(DxGfxCommandQueueGlobal);
		ID3D12CommandList* commandLists[] = {commandList.Get()};
		queue.GetQueue()->ExecuteCommandLists(1, commandLists);
		queue.WaitForIdle();

		auto dxTexture = std::make_unique<DxTexture>();
		dxTexture->InitializeFromResource(
			device,
			std::move(texture),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			"TextUITexture"
		);

		auto resource = std::make_shared<TextureResource>();
		resource->SetMeta(false, EvAsset::TextureUsage::Albedo);
		resource->SetTexture(std::move(dxTexture));
		return resource;
	}
}

void TextUIComponent::GetRenderData(std::vector<UIRenderData>& outData)
{
	if (auto* go = GetGameObject())
	{
		if (!go->IsActive())
		{
			return;
		}
	}

	RectTransformComponent* rectTr = GetRectTransform();
	if (!rectTr)
	{
		return;
	}

	UpdateTextTextureIfNeeded();

	if (m_textTextureResource == nullptr || !m_textTextureResource->IsReady())
	{
		return;
	}

	UIRenderData data;
	data.textureResource = m_textTextureResource;
	data.rect = rectTr->GetRect();
	data.uvMin = {0.0f, 0.0f};
	data.uvMax = {1.0f, 1.0f};
	data.rotationDegrees = rectTr->GetRotationDegrees();
	data.color = m_color;

	outData.push_back(data);
}

void TextUIComponent::UpdateTextTextureIfNeeded()
{
	if (!m_isDirty)
	{
		return;
	}

	if (m_text.empty())
	{
		m_textTextureResource.reset();
		m_isDirty = false;
		return;
	}

	RectTransformComponent* rectTr = GetRectTransform();
	if (!rectTr)
	{
		return;
	}

	const auto rect = rectTr->GetRect();
	const uint32_t width = std::max<uint32_t>(1, static_cast<uint32_t>(std::ceil(rect.width)));
	const uint32_t height = std::max<uint32_t>(1, static_cast<uint32_t>(std::ceil(rect.height)));

	auto dwriteFactory = GetDWriteFactory();
	auto d2dFactory = GetD2DFactory();
	auto wicFactory = GetWICFactory();
	if (!dwriteFactory || !d2dFactory || !wicFactory)
	{
		return;
	}

	ComPtr<IDWriteTextFormat> textFormat;
	HRESULT hr = dwriteFactory->CreateTextFormat(
		L"Noto Sans KR",
		nullptr,
		DWRITE_FONT_WEIGHT_MEDIUM,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		m_fontSize,
		L"ko-kr",
		textFormat.GetAddressOf()
	);
	if (FAILED(hr))
	{
		return;
	}

	switch (m_horizontalAlign)
	{
	case TextHorizontalAlign::Center:
		textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		break;
	case TextHorizontalAlign::Right:
		textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
		break;
	default:
		textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		break;
	}

	switch (m_verticalAlign)
	{
	case TextVerticalAlign::Center:
		textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		break;
	case TextVerticalAlign::Bottom:
		textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		break;
	default:
		textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		break;
	}

	ComPtr<IDWriteTextLayout> textLayout;
	hr = dwriteFactory->CreateTextLayout(
		m_text.c_str(),
		static_cast<UINT32>(m_text.size()),
		textFormat.Get(),
		static_cast<float>(width),
		static_cast<float>(height),
		textLayout.GetAddressOf()
	);
	if (FAILED(hr))
	{
		return;
	}

	ComPtr<IWICBitmap> bitmap;
	hr = wicFactory->CreateBitmap(
		width,
		height,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapCacheOnLoad,
		bitmap.GetAddressOf()
	);
	if (FAILED(hr))
	{
		return;
	}

	D2D1_RENDER_TARGET_PROPERTIES renderTargetProps = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	ComPtr<ID2D1RenderTarget> renderTarget;
	hr = d2dFactory->CreateWicBitmapRenderTarget(bitmap.Get(), renderTargetProps, renderTarget.GetAddressOf());
	if (FAILED(hr))
	{
		return;
	}

	ComPtr<ID2D1SolidColorBrush> brush;
	hr = renderTarget->CreateSolidColorBrush(
		D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
		brush.GetAddressOf()
	);
	if (FAILED(hr))
	{
		return;
	}

	renderTarget->BeginDraw();
	renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
	renderTarget->DrawTextLayout(D2D1::Point2F(0.0f, 0.0f), textLayout.Get(), brush.Get());
	hr = renderTarget->EndDraw();
	if (FAILED(hr))
	{
		return;
	}

	std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
	WICRect copyRect = {0, 0, static_cast<INT>(width), static_cast<INT>(height)};
	hr = bitmap->CopyPixels(&copyRect, width * 4, static_cast<UINT>(pixels.size()), pixels.data());
	if (FAILED(hr))
	{
		return;
	}

	auto textTexture = CreateTextureResourceFromBGRA(pixels, width, height);
	if (!textTexture)
	{
		return;
	}

	m_textTextureResource = std::move(textTexture);
	m_isDirty = false;
}
