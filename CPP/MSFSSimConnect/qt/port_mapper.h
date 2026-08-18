#pragma once
#include <QObject>

class PortMapper final : public QObject {
    Q_OBJECT
public:
    explicit PortMapper(QObject *parent = nullptr);
    void start(const QString &localAddress, quint16 port);
    void stop();
signals:
    void statusChanged(const QString &text);
private:
    bool m_active = false;
};
