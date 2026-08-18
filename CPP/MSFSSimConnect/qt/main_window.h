#pragma once

#include <QMainWindow>
#include "simconnect_source.h"

class QLabel;
class QSpinBox;
class QTextEdit;
class QCheckBox;
class QrCodeWidget;
class UdpServer;
class PortMapper;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow() override;
private:
    void restartServer();
    void updateData(const FlightData &data);
    void updateConnectionInfo();
    QLabel *m_simStatus, *m_clientStatus, *m_position, *m_publicStatus, *m_addressNote;
    QSpinBox *m_port, *m_interval;
    QCheckBox *m_publicMode;
    QTextEdit *m_log;
    QrCodeWidget *m_qr;
    UdpServer *m_server;
    SimConnectSource *m_simconnect;
    PortMapper *m_mapper;
    FlightData m_data;
};
