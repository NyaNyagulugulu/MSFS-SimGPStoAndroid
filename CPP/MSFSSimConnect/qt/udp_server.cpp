#include "udp_server.h"

#include <QDateTime>
#include <QNetworkDatagram>
#include <QNetworkProxy>
#include <QTimer>

namespace {
constexpr int kMaxClients = 3;
constexpr qint64 kClientTimeoutMs = 15'000;
}

UdpServer::UdpServer(QObject *parent) : QObject(parent), m_heartbeat(new QTimer(this)) {
    // UDP 服务端必须直连。本机/桌面环境若配置了 HTTP/SOCKS 代理，Qt 默认
    // 代理会导致 bind() 报 "The proxy type is invalid for this operation"。
    m_ipv4.setProxy(QNetworkProxy::NoProxy);
    m_ipv6.setProxy(QNetworkProxy::NoProxy);
    connect(&m_ipv4, &QUdpSocket::readyRead, this, &UdpServer::readPendingDatagrams);
    connect(&m_ipv6, &QUdpSocket::readyRead, this, &UdpServer::readPendingDatagrams);
    connect(m_heartbeat, &QTimer::timeout, this, &UdpServer::sendHeartbeat);
    m_heartbeat->setInterval(2'000);
}

bool UdpServer::listen(quint16 port, QString *error) {
    stop();
    const auto options = QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint;
    const bool v4 = m_ipv4.bind(QHostAddress::AnyIPv4, port, options);
    const bool v6 = m_ipv6.bind(QHostAddress::AnyIPv6, port, options);
    if (!v4 && !v6) {
        if (error) *error = tr("无法监听 UDP %1：%2").arg(port).arg(m_ipv4.errorString());
        return false;
    }
    m_heartbeat->start();
    emit message(tr("正在监听 UDP %1").arg(port));
    return true;
}

void UdpServer::stop() {
    m_heartbeat->stop();
    m_ipv4.close();
    m_ipv6.close();
    if (!m_clients.isEmpty()) {
        m_clients.clear();
        emit clientCountChanged(0);
    }
}

void UdpServer::sendFlightData(const QString &data) {
    const QByteArray bytes = data.toUtf8();
    for (const Client &client : m_clients) writeDatagram(bytes, client);
}

int UdpServer::clientCount() const { return m_clients.size(); }

QString UdpServer::clientKey(const QHostAddress &address, quint16 port) {
    return address.toString() + QLatin1Char(':') + QString::number(port);
}

QList<QUdpSocket *> UdpServer::sockets() { return {&m_ipv4, &m_ipv6}; }

void UdpServer::writeDatagram(const QByteArray &data, const Client &client) {
    QUdpSocket *socket = client.address.protocol() == QAbstractSocket::IPv6Protocol ? &m_ipv6 : &m_ipv4;
    if (socket->state() == QAbstractSocket::BoundState) socket->writeDatagram(data, client.address, client.port);
}

void UdpServer::readPendingDatagrams() {
    for (QUdpSocket *socket : sockets()) while (socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = socket->receiveDatagram();
        const QByteArray payload = datagram.data();
        if (!payload.startsWith("HELLO")) continue;
        const QString key = clientKey(datagram.senderAddress(), datagram.senderPort());
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        auto it = m_clients.find(key);
        if (it == m_clients.end()) {
            if (m_clients.size() >= kMaxClients) {
                socket->writeDatagram("SERVER_FULL", datagram.senderAddress(), datagram.senderPort());
                continue;
            }
            it = m_clients.insert(key, {datagram.senderAddress(), static_cast<quint16>(datagram.senderPort()), now});
            emit clientCountChanged(m_clients.size());
            emit message(tr("客户端已连接：%1").arg(key));
        } else it->lastHelloMs = now;
        socket->writeDatagram("PONG" + payload.mid(5), datagram.senderAddress(), datagram.senderPort());
    }
}

void UdpServer::sendHeartbeat() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool changed = false;
    for (auto it = m_clients.begin(); it != m_clients.end();) {
        if (now - it->lastHelloMs > kClientTimeoutMs) { it = m_clients.erase(it); changed = true; }
        else { writeDatagram("HEARTBEAT", *it); ++it; }
    }
    if (changed) emit clientCountChanged(m_clients.size());
}
