#include "simconnect_source.h"

#include <QTimer>

#ifdef HAVE_SIMCONNECT
#include <SimConnect.h>

namespace {
struct NativeSimData {
    double latitude, longitude, altitude, heading;
    double pitch, roll, groundSpeed, airSpeed;
};

void CALLBACK simDispatch(SIMCONNECT_RECV *message, DWORD, void *context) {
    if (!message || message->dwID != SIMCONNECT_RECV_ID_SIMOBJECT_DATA) return;
    auto *source = static_cast<SimConnectSource *>(context);
    const auto *received = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA *>(message);
    const auto *native = reinterpret_cast<const NativeSimData *>(&received->dwData);
    FlightData data;
    data.latitude = native->latitude; data.longitude = native->longitude;
    data.altitude = native->altitude; data.heading = native->heading;
    data.pitch = native->pitch; data.roll = native->roll;
    data.groundSpeed = native->groundSpeed; data.airSpeed = native->airSpeed;
    emit source->dataReceived(data);
}
}
#endif

QString FlightData::toWireFormat() const {
    return QString::asprintf("%.6f,%.6f,%.1f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
        latitude, longitude, altitude, heading, pitch, roll, groundSpeed, airSpeed);
}

SimConnectSource::SimConnectSource(QObject *parent) : QObject(parent), m_timer(new QTimer(this)) {
    m_timer->setInterval(5);
    connect(m_timer, &QTimer::timeout, this, [this] {
#ifdef HAVE_SIMCONNECT
        if (!m_handle) {
            HANDLE handle = nullptr;
            if (SUCCEEDED(SimConnect_Open(&handle, "MSFS Sender", nullptr, 0, 0, 0))) {
                m_handle = handle;
                SimConnect_AddToDataDefinition(handle, 0, "PLANE LATITUDE", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE LONGITUDE", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE ALTITUDE", "feet");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE HEADING DEGREES TRUE", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE PITCH DEGREES", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE BANK DEGREES", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "GPS GROUND SPEED", "meters per second");
                SimConnect_AddToDataDefinition(handle, 0, "AIRSPEED INDICATED", "meters per second");
                SimConnect_RequestDataOnSimObject(handle, 0, 0, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME);
                emit connectionChanged(true, tr("已连接到 MSFS SimConnect"));
            }
        } else if (FAILED(SimConnect_CallDispatch(static_cast<HANDLE>(m_handle), simDispatch, this))) {
            SimConnect_Close(static_cast<HANDLE>(m_handle));
            m_handle = nullptr;
            emit connectionChanged(false, tr("与 MSFS SimConnect 的连接已断开"));
        }
#else
        emit connectionChanged(false, tr("此平台未启用 Windows SimConnect SDK"));
        m_timer->stop();
#endif
    });
}

void SimConnectSource::start() { m_timer->start(); }

SimConnectSource::~SimConnectSource() {
#ifdef HAVE_SIMCONNECT
    if (m_handle) SimConnect_Close(static_cast<HANDLE>(m_handle));
#endif
}
