#include "port_mapper.h"
PortMapper::PortMapper(QObject *parent) : QObject(parent) {}
void PortMapper::start(const QString &localAddress, quint16 port) {
    m_active = true;
    Q_UNUSED(localAddress); Q_UNUSED(port); emit statusChanged(tr("请在路由器或防火墙中手动放行此 UDP 端口"));
}
void PortMapper::stop() {
    if (!m_active) return; m_active = false;
    emit statusChanged({});
}
