#include "Common.h"
#include "Logger.h"
#include "OS.h"
#include "CoreProfilerFactory.h"

extern "C" BOOL __stdcall DllMain(HINSTANCE hInstDll, DWORD reason, PVOID) {
#ifdef _WINDOWS
	//MessageBoxA(NULL, "attach", "attach", MB_OK);
#endif // _WINDOWS

	switch (reason) {
		case DLL_PROCESS_ATTACH:
			Logger::Info("Profiler DLL loaded into PID %d", OS::GetPid());
			break;

		case DLL_PROCESS_DETACH:
			Logger::Info("Profiler DLL unloaded from PID %d", OS::GetPid());
			Logger::Shutdown();
			break;
	}
	return TRUE;
}

class __declspec(uuid("805A308B-061C-47F3-9B30-F785C3186E81")) CoreProfiler;

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
	Logger::Debug(__FUNCTION__);

	if (TRUE) {
		static CoreProfilerFactory factory;
		return factory.QueryInterface(riid, ppv);
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}
