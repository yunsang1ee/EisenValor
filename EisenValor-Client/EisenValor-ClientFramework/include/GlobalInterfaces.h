#pragma once

class InputGlobal;
class TimerGlobal;
class DxDeviceGlobal;
class DxGfxCommandQueueGlobal;

#ifdef _DEBUG
class DxDebugGlobal;
#endif

namespace Globals
{

void Initialize();
void Shutdown();

} // namespace Globals