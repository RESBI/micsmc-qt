/* CoreGridWidget.cpp — 每核利用率柱状图（复刻原版 AvgCoreUtilization）
 * 柱数由数据决定（= 设备核心数），不假定具体型号。 */
#include "CoreGridWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

CoreGridWidget::CoreGridWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(48);
}

void CoreGridWidget::setValues(const QVector<double> &perCore)
{
    m_vals = perCore;
    update();
}

void CoreGridWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    const int n = m_vals.size();
    /* Y 轴标题（原版 "Utilization [%]"，竖排左侧） */
    p.save();
    p.translate(10, height() / 2.0);
    p.rotate(-90);
    p.setPen(QColor(0x08, 0x60, 0xA8));
    QFont f = p.font();
    f.setPointSizeF(7.5);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRect(-height() / 2, -28, height(), 14),
               Qt::AlignCenter, QStringLiteral("Utilization [%]"));
    p.restore();

    /* 画布区（Y 轴标题右侧） */
    const int cx0 = 26, cx1 = width() - 4;
    const int cy0 = 4, cy1 = height() - 14;
    const double cw = cx1 - cx0, ch = cy1 - cy0;

    /* 横网格 0/25/50/75/100% */
    p.setPen(QPen(QColor(0xB0, 0xD0, 0xE8), 1));
    for (int i = 0; i <= 4; i++) {
        int y = int(cy1 - ch * i / 4.0);
        p.drawLine(cx0, y, cx1, y);
    }
    /* 刻度值 */
    p.setFont(f);
    p.setPen(QColor(0x08, 0x60, 0xA8));
    for (int i = 0; i <= 4; i += 2) {
        int y = int(cy1 - ch * i / 4.0);
        p.drawText(QRect(cx0 - 24, y - 6, 22, 12), Qt::AlignRight,
                   QString::number(i * 25));
    }

    if (n == 0) {
        p.drawText(QRect(cx0, cy0, int(cw), int(ch)), Qt::AlignCenter,
                   QStringLiteral("…"));
        m_bars.clear();
        return;
    }

    /* 每核一根柱：柱宽 = 画布宽 / 核心数，隙 1px */
    m_bars.clear();
    m_bars.reserve(n);
    const double bw = cw / n;
    for (int i = 0; i < n; i++) {
        double v = qBound(0.0, m_vals[i], 100.0);
        int h = int(ch * v / 100.0);
        QRect bar(int(cx0 + i * bw), cy1 - h, qMax(1, int(bw) - 1), h);
        /* 柱色：Intel 蓝基色，高负载加深 */
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x08, 0x60, 0xA8));
        p.drawRect(bar);
        m_bars.append(bar);
    }
    /* 底轴 */
    p.setPen(QPen(QColor(0x08, 0x60, 0xA8), 1));
    p.drawLine(cx0, cy1, cx1, cy1);
}

void CoreGridWidget::mouseMoveEvent(QMouseEvent *ev)
{
    for (int i = 0; i < m_bars.size() && i < m_vals.size(); i++) {
        if (m_bars[i].contains(ev->pos())) {
            QToolTip::showText(ev->globalPosition().toPoint(),
                               QStringLiteral("Core %1: %2%")
                                   .arg(i + 1)
                                   .arg(m_vals[i], 0, 'f', 1),
                               this);
            return;
        }
    }
    QWidget::mouseMoveEvent(ev);
}
