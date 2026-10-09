/* PlotWidget.cpp — 双 Y 轴时间序列折线 */
#include "PlotWidget.h"

#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

PlotWidget::PlotWidget(const QStringList &seriesNames,
                       const QList<QColor> &colors,
                       const QString &unit,
                       QWidget *parent)
    : QWidget(parent), m_names(seriesNames), m_colors(colors), m_unit(unit)
{
    setMinimumHeight(70);
}

void PlotWidget::setFixedRange(double minV, double maxV)
{
    m_fixed = true;
    m_fixedMin = minV;
    m_fixedMax = maxV;
    update();
}

void PlotWidget::setXTitle(const QString &title)
{
    m_xTitle = title;
    update();
}

void PlotWidget::setSeriesNames(const QStringList &names)
{
    m_names = names;
    update();
}

void PlotWidget::clear()
{
    m_samples.clear();
    update();
}

void PlotWidget::push(double tSec, const QVector<double> &values)
{
    Sample s;
    s.t = tSec;
    s.v = values;
    m_samples.append(s);
    while (m_samples.size() > 2 && m_samples.first().t < tSec - m_windowSec - 2.0)
        m_samples.removeFirst();
    update();
}

void PlotWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    /* 画布底色与面板一致（无白底、无边框） */
    p.fillRect(rect(), QColor(225, 225, 225));

    const QColor axis(0x08, 0x60, 0xA8);
    QFont f = p.font();
    f.setPointSizeF(8);
    f.setBold(true);
    f.setFamily(QStringLiteral("Courier New"));
    p.setFont(f);
    const QFontMetrics fm(f);

    /* 画布区：左右各留 Y 轴标签+刻度，下留 X 标签 + X 标题 */
    const int lm = fm.horizontalAdvance(QStringLiteral("100")) + 12;
    const int rm = lm;
    const int bm = fm.height() + (m_xTitle.isEmpty() ? 4 : fm.height() + 4);
    const QRectF r(lm, 4, width() - lm - rm - 2, height() - bm - 4);

    /* Y 范围 */
    double lo = m_fixed ? m_fixedMin : 0;
    double hi = m_fixed ? m_fixedMax : 1;
    if (!m_fixed && !m_samples.isEmpty()) {
        lo = std::numeric_limits<double>::max();
        hi = std::numeric_limits<double>::lowest();
        for (const auto &s : m_samples)
            for (double v : s.v) {
                if (v != v) continue;
                if (v < lo) lo = v;
                if (v > hi) hi = v;
            }
        if (lo > hi) { lo = 0; hi = 1; }
        if (hi - lo < 1e-9) hi = lo + 1;
        double pad = (hi - lo) * 0.08;
        lo -= pad;
        hi += pad;
    }
    if (hi <= lo) hi = lo + 1;

    /* ---- Y 轴：左右各 5 档标签 + 主次刻度（各 5 等分，主刻度带标签）---- */
    const int ydiv = 5;
    for (int side = 0; side < 2; side++) {
        const bool left = (side == 0);
        const double ax = left ? r.left() : r.right();
        p.setPen(QPen(axis, 1));
        p.drawLine(QPointF(ax, r.top()), QPointF(ax, r.bottom()));
        for (int i = 0; i <= ydiv; i++) {
            double y = r.bottom() - r.height() * i / ydiv;
            double v = lo + (hi - lo) * i / ydiv;
            p.setPen(QPen(axis, 1));
            /* 主刻度 6px（向内） */
            p.drawLine(QPointF(ax, y),
                       QPointF(ax + (left ? 6 : -6), y));
            /* 次刻度：每档 4 等分，3px */
            if (i < ydiv)
                for (int k = 1; k <= 4; k++) {
                    double yy = y - r.height() / ydiv * k / 5.0;
                    p.drawLine(QPointF(ax, yy),
                               QPointF(ax + (left ? 3 : -3), yy));
                }
            /* 标签 */
            p.setPen(axis);
            const QString t = QString::number(qRound(v));
            if (left)
                p.drawText(QRectF(ax - lm, y - fm.height() / 2.0,
                                  lm - 10, fm.height()),
                           Qt::AlignRight | Qt::AlignVCenter, t);
            else
                p.drawText(QRectF(ax + 10, y - fm.height() / 2.0,
                                  rm - 10, fm.height()),
                           Qt::AlignLeft | Qt::AlignVCenter, t);
        }
    }

    /* ---- X 轴 ---- */
    p.setPen(QPen(axis, 1));
    p.drawLine(QPointF(r.left(), r.bottom()), QPointF(r.right(), r.bottom()));
    const double tNow = m_samples.isEmpty() ? 0 : m_samples.last().t;
    const double t0 = tNow - m_windowSec;
    /* X 轴主刻度：5 等分 */
    const int xdiv = 5;
    for (int i = 0; i <= xdiv; i++) {
        double x = r.left() + r.width() * i / xdiv;
        p.setPen(QPen(axis, 1));
        p.drawLine(QPointF(x, r.bottom()), QPointF(x, r.bottom() + 6));
        if (i < xdiv)
            for (int k = 1; k <= 4; k++) {
                double xx = x + r.width() / xdiv * k / 5.0;
                p.drawLine(QPointF(xx, r.bottom()),
                           QPointF(xx, r.bottom() + 3));
            }
        /* 时间戳标签 */
        p.setPen(axis);
        double tt = t0 + m_windowSec * i / xdiv;
        QString lab;
        if (m_timeLabels)
            lab = QDateTime::fromSecsSinceEpoch(qint64(tt))
                      .toString(QStringLiteral("HH:mm:ss"));
        else
            lab = QString::number(tt, 'f', 0);
        QRectF box(x - 40, r.bottom() + 7, 80, fm.height());
        p.drawText(box, (i == 0) ? Qt::AlignLeft
                                 : (i == xdiv ? Qt::AlignRight : Qt::AlignHCenter)
                       | Qt::AlignVCenter, lab);
    }

    /* ---- 曲线 ---- */
    for (int si = 0; si < m_names.size() && si < m_colors.size(); si++) {
        QPainterPath path;
        bool started = false;
        for (const auto &s : m_samples) {
            if (si >= s.v.size() || s.v[si] != s.v[si])
                continue;
            double x = r.left() + r.width() * (s.t - t0) / m_windowSec;
            double y = r.bottom() - r.height() * (s.v[si] - lo) / (hi - lo);
            if (!started) { path.moveTo(x, y); started = true; }
            else path.lineTo(x, y);
        }
        p.setPen(QPen(m_colors.value(si), 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }

    /* ---- X 轴标题（居中，下方）---- */
    if (!m_xTitle.isEmpty()) {
        p.setPen(axis);
        p.drawText(QRectF(r.left(), r.bottom() + 7 + fm.height(),
                          r.width(), fm.height()),
                   Qt::AlignCenter, m_xTitle);
    }
}
