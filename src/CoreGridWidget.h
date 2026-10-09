/* CoreGridWidget.h — 每核利用率柱状图（复刻原版 AvgCoreUtilization）
 *
 * 每核一根竖柱（柱数随设备核心数变化），蓝系配色，Y 轴 Utilization [%]。
 * 悬停提示核心号与数值。
 */
#ifndef MICSMC_QT_COREGRIDWIDGET_H
#define MICSMC_QT_COREGRIDWIDGET_H

#include <QRect>
#include <QVector>
#include <QWidget>

class CoreGridWidget : public QWidget {
    Q_OBJECT
public:
    explicit CoreGridWidget(QWidget *parent = nullptr);
    void setValues(const QVector<double> &perCore);  /* 0..100，% */

    QSize sizeHint() const override { return QSize(360, 100); }

protected:
    void paintEvent(QPaintEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private:
    QVector<double> m_vals;
    QVector<QRect>  m_bars;   /* 每柱矩形，悬停用 */
};

#endif /* MICSMC_QT_COREGRIDWIDGET_H */
