#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum SIMCONNECT_RECV_ID {
    SIMCONNECT_RECV_ID_NULL = 0,
    SIMCONNECT_RECV_ID_SIMOBJECT_DATA = 8,
};

enum SIMCONNECT_DATATYPE {
    SIMCONNECT_DATATYPE_INVALID = 0,
    SIMCONNECT_DATATYPE_FLOAT64 = 4,
};

enum SIMCONNECT_PERIOD {
    SIMCONNECT_PERIOD_NEVER = 0,
    SIMCONNECT_PERIOD_ONCE = 1,
    SIMCONNECT_PERIOD_VISUAL_FRAME = 2,
    SIMCONNECT_PERIOD_SIM_FRAME = 3,
};

using SIMCONNECT_DATA_DEFINITION_ID = DWORD;
using SIMCONNECT_DATA_REQUEST_ID = DWORD;
using SIMCONNECT_OBJECT_ID = DWORD;

constexpr SIMCONNECT_OBJECT_ID SIMCONNECT_OBJECT_ID_USER = 0;

struct SIMCONNECT_RECV {
    DWORD dwSize;
    SIMCONNECT_RECV_ID dwID;
    DWORD dwException;
    DWORD dwSendID;
};

struct SIMCONNECT_RECV_SIMOBJECT_DATA : SIMCONNECT_RECV {
    DWORD dwRequestID;
    DWORD dwObjectID;
    DWORD dwDefineID;
    DWORD dwFlags;
    DWORD dwentrynumber;
    DWORD dwoutof;
    DWORD dwDefineCount;
    DWORD dwData[8192];
};

using SIMCONNECT_DISPATCH_PROC = void (CALLBACK *)(SIMCONNECT_RECV *, DWORD, void *);

namespace simconnect_minimal {
template <typename Function>
Function load(const char *name) {
    static HMODULE module = nullptr;
    static bool attempted = false;
    if (!attempted) {
        attempted = true;
        module = LoadLibraryA("SimConnect_internal.dll");
    }
    return module ? reinterpret_cast<Function>(GetProcAddress(module, name)) : nullptr;
}
}

inline HRESULT SimConnect_Open(
    HANDLE *phSimConnect,
    const char *szName,
    HWND hWnd,
    DWORD UserEventWin32,
    HANDLE hEventHandle,
    DWORD ConfigIndex) {
    using Function = HRESULT(WINAPI *)(HANDLE *, const char *, HWND, DWORD, HANDLE, DWORD);
    const auto function = simconnect_minimal::load<Function>("SimConnect_Open");
    return function ? function(phSimConnect, szName, hWnd, UserEventWin32, hEventHandle, ConfigIndex) : E_FAIL;
}

inline HRESULT SimConnect_Close(HANDLE hSimConnect) {
    using Function = HRESULT(WINAPI *)(HANDLE);
    const auto function = simconnect_minimal::load<Function>("SimConnect_Close");
    return function ? function(hSimConnect) : E_FAIL;
}

inline HRESULT SimConnect_AddToDataDefinition(
    HANDLE hSimConnect,
    SIMCONNECT_DATA_DEFINITION_ID DefineID,
    const char *DatumName,
    const char *UnitsName,
    SIMCONNECT_DATATYPE DatumType = SIMCONNECT_DATATYPE_FLOAT64,
    float fEpsilon = 0.0f,
    DWORD DatumID = static_cast<DWORD>(-1)) {
    using Function = HRESULT(WINAPI *)(HANDLE, SIMCONNECT_DATA_DEFINITION_ID, const char *, const char *, SIMCONNECT_DATATYPE, float, DWORD);
    const auto function = simconnect_minimal::load<Function>("SimConnect_AddToDataDefinition");
    return function ? function(hSimConnect, DefineID, DatumName, UnitsName, DatumType, fEpsilon, DatumID) : E_FAIL;
}

inline HRESULT SimConnect_RequestDataOnSimObject(
    HANDLE hSimConnect,
    SIMCONNECT_DATA_REQUEST_ID RequestID,
    SIMCONNECT_DATA_DEFINITION_ID DefineID,
    SIMCONNECT_OBJECT_ID ObjectID,
    SIMCONNECT_PERIOD Period,
    DWORD Flags = 0,
    DWORD origin = 0,
    DWORD interval = 0,
    DWORD limit = 0) {
    using Function = HRESULT(WINAPI *)(HANDLE, SIMCONNECT_DATA_REQUEST_ID, SIMCONNECT_DATA_DEFINITION_ID, SIMCONNECT_OBJECT_ID, SIMCONNECT_PERIOD, DWORD, DWORD, DWORD, DWORD);
    const auto function = simconnect_minimal::load<Function>("SimConnect_RequestDataOnSimObject");
    return function ? function(hSimConnect, RequestID, DefineID, ObjectID, Period, Flags, origin, interval, limit) : E_FAIL;
}

inline HRESULT SimConnect_CallDispatch(
    HANDLE hSimConnect,
    SIMCONNECT_DISPATCH_PROC pfcnDispatch,
    void *pContext) {
    using Function = HRESULT(WINAPI *)(HANDLE, SIMCONNECT_DISPATCH_PROC, void *);
    const auto function = simconnect_minimal::load<Function>("SimConnect_CallDispatch");
    return function ? function(hSimConnect, pfcnDispatch, pContext) : E_FAIL;
}
