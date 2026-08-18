#pragma once

#include <QObject>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QUdpSocket>

class UdpServer final : public QObject {
    Q_OBJECT
public:
    explicit UdpServer(QObject *parent = nullptr);
    bool listen(quint16 port, QString *error = nullptr);
    void stop();
    void sendFlightData(const QString &data);
    int clientCount() const;

signals:
    void clientCountChanged(int count);
    void message(const QString &text);

private slots:
    void readPendingDatagrams();
    void sendHeartbeat();

private:
    struct Client { QHostAddress address; quint16 port; qint64 lastHelloMs; };
    static QString clientKey(const QHostAddress &address, quint16 port);
    void writeDatagram(const QByteArray &data, const Client &client);
    QList<QUdpSocket *> sockets();

    QUdpSocket m_ipv4;
    QUdpSocket m_ipv6;
    QHash<QString, Client> m_clients;
    QTimer *m_heartbeat;
};
