#pragma once
#include "Singleton.h"
#include "UIComponent.h"
#include "DxCommon.h"
#include "DxFrameResource.h"
#include "DenseList.h"
#include <vector>
#include <memory>

// 전방 선언
class UIRenderPass;

class UIGlobal : public Singleton<UIGlobal>
{
private:
	friend class Singleton<UIGlobal>;
	UIGlobal() = default;
	~UIGlobal() override = default;

public:
	void Initialize() override {}
	void Release() override {}
	 using UIHandle = DenseList<std::unique_ptr<UIComponent>>::Handle;

	template <typename T, typename... Args>
	UIHandle AddUI(Args&&... args)
	{
		 auto ui = std::make_unique<T>(std::forward<Args>(args)...);
		 T*	  ptr = ui.get();
		 ui->Initialize();
		 return m_uiComponents.Emplace(std::move(ui));
	}

	UIComponent* GetUI(UIHandle handle)
	{
		if (m_uiComponents.IsValid(handle))
		{
			return m_uiComponents.Get(handle).get();
		}
		return nullptr;
	}

	// UI 컴포넌트 접근을 위한 getter
	const DenseList<std::unique_ptr<UIComponent>>& GetUIComponents() const { return m_uiComponents; }

private:
	DenseList<std::unique_ptr<UIComponent>>	  m_uiComponents;
};