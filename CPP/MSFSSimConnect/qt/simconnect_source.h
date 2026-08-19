#pragma once

#include <QObject>
#include <QString>

class QTimer;
class QUdpSocket;
class QProcess;

struct FlightData {
    double latitude = 0, longitude = 0, altitude = 0, heading = 0;
    double pitch = 0, roll = 0, groundSpeed = 0, airSpeed = 0;
    QString toWireFormat() const;
};

class SimConnectSource final : public QObject {
    Q_OBJECT
public:
    explicit SimConnectSource(QObject *parent = nullptr);
    ~SimConnectSource() override;
    void start();
signals:
    void connectionChanged(bool connected, const QString &detail);
    void dataReceived(const FlightData &data);
private:
    QTimer *m_timer;
#ifdef HAVE_SIMCONNECT
    void *m_handle = nullptr;
#else
    QUdpSocket *m_wineSocket = nullptr;
    QProcess *m_wineProcess = nullptr;
#endif
};
