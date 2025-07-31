#pragma once

#include "Global.h"
#include "IInputGlobal.h"
#include "ITimerGlobal.h"

namespace Globals
{
    void InitializeGlobalRegistry();

    inline auto& Input() noexcept
    {
        return GlobalRegistry::Get<IInputGlobal>();
    }

    inline auto& Timer() noexcept
    {
        return GlobalRegistry::Get<ITimerGlobal>();
    }
}