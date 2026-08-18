#include "main_window.h"
#include "port_mapper.h"
#include "qr_code_widget.h"
#include "udp_server.h"

#include <QCheckBox>
#include <QDateTime>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString localIpv4() {
    for (const auto &iface : QNetworkInterface::allInterfaces()) if (iface.flags().testFlag(QNetworkInterface::IsUp) && !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        for (const auto &entry : iface.addressEntries()) if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) return entry.ip().toString();
    return QStringLiteral("127.0.0.1");
}
QString publicIpv6() {
    for (const auto &iface : QNetworkInterface::allInterfaces()) if (iface.flags().testFlag(QNetworkInterface::IsUp))
        for (const auto &entry : iface.addressEntries()) { const auto ip = entry.ip(); if (ip.protocol() == QAbstractSocket::IPv6Protocol && !ip.isLoopback() && !ip.isLinkLocal() && !ip.isUniqueLocalUnicast()) return ip.toString(); }
    return {};
}
}

MainWindow::MainWindow() : m_server(new UdpServer(this)), m_simconnect(new SimConnectSource(this)), m_mapper(new PortMapper(this)) {
    setWindowTitle(tr("MSFS-SimConnect 电脑端")); resize(880, 610);
    auto *central = new QWidget(this); auto *root = new QVBoxLayout(central);
    auto *content = new QGridLayout; m_qr = new QrCodeWidget(central); content->addWidget(m_qr, 0, 0, 5, 1);
    auto *info = new QGroupBox(tr("飞行状态"), central); auto *form = new QFormLayout(info);
    m_clientStatus = new QLabel(tr("未连接手机"), info); m_simStatus = new QLabel(tr("等待 MSFS 启动"), info); m_position = new QLabel(tr("尚未收到飞行数据"), info);
    m_position->setWordWrap(true); form->addRow(tr("手机连接"), m_clientStatus); form->addRow(tr("SimConnect"), m_simStatus); form->addRow(tr("实时数据"), m_position); content->addWidget(info, 0, 1, 4, 1);
    m_addressNote = new QLabel(central); m_addressNote->setAlignment(Qt::AlignCenter); m_addressNote->setWordWrap(true); content->addWidget(m_addressNote, 5, 0, 1, 2);
    root->addLayout(content, 1);
    auto *settings = new QGroupBox(tr("服务设置"), central); auto *grid = new QGridLayout(settings);
    m_interval = new QSpinBox(settings); m_interval->setRange(5, 5000); m_interval->setValue(50); m_interval->setSuffix(tr(" ms"));
    m_port = new QSpinBox(settings); m_port->setRange(1, 65535); m_port->setValue(36666); auto *apply = new QPushButton(tr("应用设置"), settings);
    m_publicMode = new QCheckBox(tr("公网使用（IPv6）"), settings); m_publicStatus = new QLabel(settings); m_publicStatus->setWordWrap(true);
    grid->addWidget(new QLabel(tr("发送间隔"), settings), 0, 0); grid->addWidget(m_interval, 0, 1); grid->addWidget(new QLabel(tr("UDP 端口"), settings), 0, 2); grid->addWidget(m_port, 0, 3); grid->addWidget(apply, 0, 4); grid->addWidget(m_publicMode, 1, 0, 1, 3); grid->addWidget(m_publicStatus, 1, 3, 1, 2); root->addWidget(settings);
    m_log = new QTextEdit(central); m_log->setReadOnly(true); m_log->setMaximumHeight(80); root->addWidget(m_log); setCentralWidget(central);
    connect(apply, &QPushButton::clicked, this, &MainWindow::restartServer);
    connect(m_publicMode, &QCheckBox::toggled, this, [this](bool on) { if (on) m_mapper->start(localIpv4(), m_port->value()); else m_mapper->stop(); updateConnectionInfo(); });
    connect(m_server, &UdpServer::clientCountChanged, this, [this](int n) { m_clientStatus->setText(n ? tr("已连接 %1 台手机").arg(n) : tr("未连接手机")); }); connect(m_server, &UdpServer::message, m_log, &QTextEdit::append);
    connect(m_mapper, &PortMapper::statusChanged, m_publicStatus, &QLabel::setText);
    connect(m_simconnect, &SimConnectSource::connectionChanged, this, [this](bool ok, const QString &text) { m_simStatus->setText(text); m_simStatus->setStyleSheet(ok ? "color:#087f23" : "color:#b00020"); }); connect(m_simconnect, &SimConnectSource::dataReceived, this, &MainWindow::updateData);
    restartServer(); m_simconnect->start();
}
MainWindow::~MainWindow() { m_mapper->stop(); m_server->stop(); }
void MainWindow::restartServer() {
    QString error; if (!m_server->listen(m_port->value(), &error)) { QMessageBox::critical(this, tr("监听失败"), error); return; }
    if (m_publicMode->isChecked()) { m_mapper->stop(); m_mapper->start(localIpv4(), m_port->value()); } updateConnectionInfo();
}
void MainWindow::updateConnectionInfo() {
    const QString ipv6 = publicIpv6(); const bool pub = m_publicMode->isChecked() && !ipv6.isEmpty(); const QString endpoint = pub ? QStringLiteral("[%1]:%2").arg(ipv6).arg(m_port->value()) : QStringLiteral("%1:%2").arg(localIpv4()).arg(m_port->value());
    m_qr->setText(endpoint); m_addressNote->setText(pub ? tr("公网 IPv6：%1\n端口：%2").arg(ipv6).arg(m_port->value()) : tr("局域网：%1\n端口：%2").arg(localIpv4()).arg(m_port->value()));
}
void MainWindow::updateData(const FlightData &data) {
    m_data = data; m_position->setText(tr("纬度 %1  经度 %2\n高度 %3 ft / %4 m\n航向 %5°  俯仰 %6°  横滚 %7°\n地速 %8 m/s  空速 %9 m/s").arg(data.latitude, 0, 'f', 6).arg(data.longitude, 0, 'f', 6).arg(data.altitude, 0, 'f', 1).arg(data.altitude * .3048, 0, 'f', 1).arg(data.heading, 0, 'f', 2).arg(data.pitch, 0, 'f', 2).arg(data.roll, 0, 'f', 2).arg(data.groundSpeed, 0, 'f', 2).arg(data.airSpeed, 0, 'f', 2));
    static qint64 last = 0; const qint64 now = QDateTime::currentMSecsSinceEpoch(); if (now - last >= m_interval->value()) { last = now; m_server->sendFlightData(data.toWireFormat()); }
}
