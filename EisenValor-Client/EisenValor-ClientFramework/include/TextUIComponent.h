#pragma once

#include "UIComponent.h"
#include <memory>
#include <string>

class TextureResource;

enum class TextHorizontalAlign
{
	Left,
	Center,
	Right
};

enum class TextVerticalAlign
{
	Top,
	Center,
	Bottom
};

class TextUIComponent : public UIComponent<TextUIComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "TextUIComponent"; }

	TextUIComponent() = default;
	virtual ~TextUIComponent() = default;

	void SetText(std::wstring text)
	{
		m_text = std::move(text);
		m_isDirty = true;
	}
	const std::wstring& GetText() const { return m_text; }

	void SetFontPath(std::wstring fontPath)
	{
		m_fontPath = std::move(fontPath);
		m_isDirty = true;
	}
	const std::wstring& GetFontPath() const { return m_fontPath; }

	void SetFontSize(float fontSize)
	{
		m_fontSize = fontSize;
		m_isDirty = true;
	}
	float GetFontSize() const { return m_fontSize; }

	void SetHorizontalAlign(TextHorizontalAlign align) { m_horizontalAlign = align; }
	TextHorizontalAlign GetHorizontalAlign() const { return m_horizontalAlign; }

	void SetVerticalAlign(TextVerticalAlign align) { m_verticalAlign = align; }
	TextVerticalAlign GetVerticalAlign() const { return m_verticalAlign; }

	void SetTextTextureResource(std::shared_ptr<TextureResource> res)
	{
		m_textTextureResource = std::move(res);
		m_isDirty = false;
	}
	std::shared_ptr<TextureResource> GetTextTextureResource() const { return m_textTextureResource; }

	bool IsDirty() const { return m_isDirty; }
	void MarkDirty() { m_isDirty = true; }

	virtual void GetRenderData(std::vector<UIRenderData>& outData) override;

private:
	std::wstring m_text;
	std::wstring m_fontPath = L"Resource/Font/NotoSansKR-Medium.ttf";
	float m_fontSize = 24.0f;
	TextHorizontalAlign m_horizontalAlign = TextHorizontalAlign::Left;
	TextVerticalAlign m_verticalAlign = TextVerticalAlign::Top;
	std::shared_ptr<TextureResource> m_textTextureResource;
	bool m_isDirty = true;
};
