#pragma once

#include "Global.h"
#include "IInputGlobal.h"
#include "ITimerGlobal.h"

namespace Globals
{
    struct RegisterAllGlobal
    {
        void operator()();
    };

    inline IInputGlobal& Input() noexcept {
        return GlobalRegistry::Get<IInputGlobal>();
    }

    inline ITimerGlobal& Timer() noexcept {
        return GlobalRegistry::Get<ITimerGlobal>();
    }
}