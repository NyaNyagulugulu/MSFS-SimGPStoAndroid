#include "qr_code_widget.h"
#include "../ThirdParty/qrcodegen/qrcodegen.hpp"

#include <QPainter>

QrCodeWidget::QrCodeWidget(QWidget *parent) : QWidget(parent) { setMinimumSize(200, 200); }
void QrCodeWidget::setText(const QString &text) {
    const auto qr = qrcodegen::QrCode::encodeText(text.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
    m_modules.resize(qr.getSize());
    for (int y = 0; y < qr.getSize(); ++y) { m_modules[y].resize(qr.getSize()); for (int x = 0; x < qr.getSize(); ++x) m_modules[y][x] = qr.getModule(x, y); }
    update();
}
QSize QrCodeWidget::sizeHint() const { return {280, 280}; }
void QrCodeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this); p.fillRect(rect(), Qt::white); if (m_modules.isEmpty()) return;
    const int quiet = 2, n = m_modules.size() + quiet * 2, unit = qMax(1, qMin(width(), height()) / n);
    const int x0 = (width() - n * unit) / 2 + quiet * unit, y0 = (height() - n * unit) / 2 + quiet * unit;
    p.setPen(Qt::NoPen); p.setBrush(Qt::black);
    for (int y = 0; y < m_modules.size(); ++y) for (int x = 0; x < m_modules.size(); ++x) if (m_modules[y][x]) p.drawRect(x0 + x * unit, y0 + y * unit, unit, unit);
}
