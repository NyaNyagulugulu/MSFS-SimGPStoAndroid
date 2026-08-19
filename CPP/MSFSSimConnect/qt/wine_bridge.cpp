// Windows-only companion for Linux builds.  Run with Wine; it reads the local
// MSFS SimConnect API and sends data to 127.0.0.1:<port>, never to the network.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef MSFS_USE_MINIMAL_SIMCONNECT
#include "simconnect_minimal.h"
#else
#include <SimConnect.h>
#endif

#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>

struct SimData { double latitude, longitude, altitude, heading, pitch, roll, groundSpeed, airSpeed; };
SOCKET outputSocket = INVALID_SOCKET;
sockaddr_in outputAddress = {};

void sendLine(const char *line) { sendto(outputSocket, line, static_cast<int>(std::strlen(line)), 0, reinterpret_cast<const sockaddr *>(&outputAddress), sizeof(outputAddress)); }
void CALLBACK dispatch(SIMCONNECT_RECV *message, DWORD, void *) {
    if (!message || message->dwID != SIMCONNECT_RECV_ID_SIMOBJECT_DATA) return;
    const auto *received = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA *>(message);
    const auto *d = reinterpret_cast<const SimData *>(&received->dwData);
    char line[256];
    std::snprintf(line, sizeof(line), "DATA:%.6f,%.6f,%.1f,%.2f,%.2f,%.2f,%.2f,%.2f", d->latitude, d->longitude, d->altitude, d->heading, d->pitch, d->roll, d->groundSpeed, d->airSpeed);
    sendLine(line);
}
int main(int argc, char **argv) {
    if (argc < 3 || std::strcmp(argv[1], "--port") != 0) return 2;
    const int port = std::atoi(argv[2]); if (port < 1 || port > 65535) return 2;
    for (int index = 3; index + 1 < argc; ++index) {
        if (std::strcmp(argv[index], "--simconnect-dir") == 0) {
            if (!SetDllDirectoryA(argv[index + 1])) return 4;
            ++index;
        } else return 2;
    }
    WSADATA wsa{}; if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 3;
    outputSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); if (outputSocket == INVALID_SOCKET) return 3;
    outputAddress.sin_family = AF_INET; outputAddress.sin_port = htons(static_cast<u_short>(port)); InetPtonA(AF_INET, "127.0.0.1", &outputAddress.sin_addr);
    HANDLE handle = nullptr; bool connected = false;
    while (true) {
        if (!handle) {
            if (SUCCEEDED(SimConnect_Open(&handle, "MSFS Wine Bridge", nullptr, 0, 0, 0))) {
                SimConnect_AddToDataDefinition(handle, 0, "PLANE LATITUDE", "degrees"); SimConnect_AddToDataDefinition(handle, 0, "PLANE LONGITUDE", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE ALTITUDE", "feet"); SimConnect_AddToDataDefinition(handle, 0, "PLANE HEADING DEGREES TRUE", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE PITCH DEGREES", "degrees"); SimConnect_AddToDataDefinition(handle, 0, "PLANE BANK DEGREES", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "GPS GROUND SPEED", "meters per second"); SimConnect_AddToDataDefinition(handle, 0, "AIRSPEED INDICATED", "meters per second");
                SimConnect_RequestDataOnSimObject(handle, 0, 0, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME); sendLine("STATUS:CONNECTED"); connected = true;
            }
        } else if (FAILED(SimConnect_CallDispatch(handle, dispatch, nullptr))) { SimConnect_Close(handle); handle = nullptr; if (connected) { sendLine("STATUS:DISCONNECTED"); connected = false; } }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
