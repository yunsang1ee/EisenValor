#pragma once

#include <IComponent.h>
#include <array>
#include <string>

class GameObject;
class Scene;
class TextUIComponent;

class OptionsMenuComponent final : public ComponentBase<OptionsMenuComponent>
{
public:
	static constexpr int		 kPriority = -1000;
	static constexpr const char* GetStaticTypeName() { return "OptionsMenuComponent"; }

	void OnStart() override;
	void OnUpdate(float deltaTime);

	void SetEscapeOpenEnabled(bool enabled) { m_escapeOpenEnabled = enabled; }
	void SetOpen(bool open);
	bool IsOpen() const { return m_isOpen; }

	static bool OpenActive();
	static bool IsOpenInActiveScene();

private:
	enum class Action : uint8_t
	{
		DisplayMode,
		PathTracing,
		RestirPT,
		Environment,
		MouseCapture,
		Close,
		Quit,
		Count
	};

	static constexpr size_t kActionCount = static_cast<size_t>(Action::Count);
	static constexpr size_t ToIndex(Action action) { return static_cast<size_t>(action); }

	void CreateMenuUI(Scene& scene);
	void ReserveText(
		Scene&				 scene,
		HandleOf<GameObject> parentHandle,
		const char*			 name,
		float				 minX,
		float				 minY,
		float				 maxX,
		float				 maxY,
		const wchar_t*		 value,
		float				 fontSize,
		int32_t				 order
	);
	void ReserveButton(
		Scene&				 scene,
		HandleOf<GameObject> parentHandle,
		const char*			 name,
		Action				 action,
		float				 minX,
		float				 minY,
		float				 maxX,
		float				 maxY,
		bool				 danger = false
	);
	void ExecuteAction(Action action);
	void RefreshLabels();
	void SetActionLabel(Action action, std::wstring value);
	void UpdateInteractability();

	HandleOf<GameObject>								m_uiRootHandle;
	std::array<HandleOf<GameObject>, kActionCount>		m_actionObjectHandles{};
	std::array<HandleOf<TextUIComponent>, kActionCount> m_actionTextHandles{};
	bool												m_isOpen = false;
	bool												m_escapeOpenEnabled = true;
	bool												m_hasGameplayControls = false;
	bool												m_restoreMouseLock = false;
};
