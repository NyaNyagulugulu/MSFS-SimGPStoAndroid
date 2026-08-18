#pragma once

#include <QWidget>

class QrCodeWidget final : public QWidget {
    Q_OBJECT
public:
    explicit QrCodeWidget(QWidget *parent = nullptr);
    void setText(const QString &text);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *) override;
private:
    QVector<QVector<bool>> m_modules;
};
