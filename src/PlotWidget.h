/* PlotWidget.h — 双 Y 轴时间序列折线
 *
 * 外观：
 *   左右双 Y 轴各带 100/80/60/40/20/0 标签与主次刻度
 *   X 轴主刻度带时间戳(HH:MM:SS)，下方居中 X 轴标题
 *   曲线 2px：System=红 #E16762、User=紫 #7D81E4
 *   画布底色与面板一致（rgb(225,225,225)），无边框，仅画坐标轴
 */
#ifndef MICSMC_QT_PLOTWIDGET_H
#define MICSMC_QT_PLOTWIDGET_H

#include <QColor>
#include <QStringList>
#include <QWidget>

class PlotWidget : public QWidget {
    Q_OBJECT
public:
    PlotWidget(const QStringList &seriesNames,
               const QList<QColor> &colors,
               const QString &unit = QString(),
               QWidget *parent = nullptr);

    void push(double tSec, const QVector<double> &values);
    void setFixedRange(double minV, double maxV);
    void setXTitle(const QString &title);      /* X 轴标题（居中，下方） */
    void setSeriesNames(const QStringList &names);  /* 语言切换时更新系列名 */
    void setTimeLabels(bool on) { m_timeLabels = on; }
    void clear();

    QSize sizeHint() const override { return QSize(380, 150); }

protected:
    void paintEvent(QPaintEvent *ev) override;

private:
    struct Sample { double t; QVector<double> v; };
    QStringList  m_names;
    QList<QColor> m_colors;
    QString      m_unit;
    QString      m_xTitle;
    QVector<Sample> m_samples;
    double  m_windowSec = 60.0;
    double  m_fixedMin = 0, m_fixedMax = 0;
    bool    m_fixed = false;
    bool    m_timeLabels = true;
};

#endif /* MICSMC_QT_PLOTWIDGET_H */
