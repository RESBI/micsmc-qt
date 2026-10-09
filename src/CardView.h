/* CardView.h — 原版排版严格复刻（规格书 v3）
 *
 * FieldWidget   右侧竖长条字段（55px 级）：两行标题 + 值 + 进度条
 * SystemPanel   上带：标题 + 每核柱状图(60%) + 温度|内存|功耗 三字段横排
 * DetailPanel   下带：曲线图(58%) + 温度|内存|功耗 三字段横排
 */
#ifndef MICSMC_QT_CARDVIEW_H
#define MICSMC_QT_CARDVIEW_H

#include <QLabel>
#include <QProgressBar>
#include <QWidget>

#include "CoreGridWidget.h"
#include "PlotWidget.h"
#include "Snapshot.h"

/* 右侧字段控件
 * 温度 = 7 段数码管(QLCDNumber) + 两行标签；
 * 内存/功耗 = 值文本(上) + 竖直圆角条(中，蓝填充) + 两行标签(下) */
class FieldWidget : public QWidget {
    Q_OBJECT
public:
    enum Kind { Temp, Mem, Power };
    FieldWidget(Kind kind, QWidget *parent = nullptr);
    void setSnapshot(const Snapshot &s);
    void setAverageMode(bool avg);   /* 上带(平均/总量) vs 下带(当前) */
    void retranslate();

protected:
    void paintEvent(QPaintEvent *ev) override;

private:
    Kind m_kind;
    bool m_avg = false;
    class QLCDNumber *m_lcd = nullptr;   /* 温度数码管 */
    QLabel *m_value = nullptr;           /* "0.31 GB" / "98 Watts" */
    QLabel *m_l1 = nullptr, *m_l2 = nullptr;  /* 下方两行标签 */
    QWidget *m_barArea = nullptr;        /* 竖条绘制区（占位定尺寸） */
    double m_pct = 0;                    /* 竖条填充比例 0..1 */
    int m_level = 0;                     /* 0=正常 1=黄Warning 2=橙Caution 3=红Critical */
    double m_pctWarn = -1, m_pctCrit = -1;  /* 功耗分区的门限（占满量程比例） */
};

/* 图例框（带立体边框，两行系列项 + 中间横线）
 * System [%] = 红 #E16762 ；User [%] = 紫 #7D81E4 */
class LegendWidget : public QWidget {
    Q_OBJECT
public:
    explicit LegendWidget(QWidget *parent = nullptr);
    void setValues(double sysPct, double userPct);
    void retranslate();

protected:
    void paintEvent(QPaintEvent *ev) override;

private:
    double m_sys = -1, m_user = -1;
    QString m_name1, m_name2;
};

/* 总览面板（上带） */
class SystemPanel : public QWidget {
    Q_OBJECT
public:
    explicit SystemPanel(QWidget *parent = nullptr);
    void setSnapshot(const Snapshot &s);
    void retranslate();                   /* 语言切换时重设标题/图例/轴标题/字段标签 */

private:
    QLabel *m_title = nullptr;            /* "Utilization View (All Devices)" */
    LegendWidget *m_legend = nullptr;     /* 左列图例框 */
    PlotWidget *m_plot = nullptr;         /* 折线（System/User 双曲线） */
    FieldWidget *m_temp = nullptr, *m_mem = nullptr, *m_power = nullptr;
};

/* 卡视图面板：标题在面板内，右上角两个视图切换小按钮
 * （原版 :/utilization.png 与 :/histo.png，tooltip "Go to ... View"） */
class DetailPanel : public QWidget {
    Q_OBJECT
public:
    enum ViewType { UtilizationView, HistogramView, HistoricalView };
    explicit DetailPanel(QWidget *parent = nullptr);
    void setSnapshot(const Snapshot &s);
    void setViewType(ViewType t);
    ViewType viewType() const { return m_view; }
    void setCardName(const QString &name);   /* "mic0" */
    void retranslate();                      /* 语言切换时重设标题/图例/轴标题/标签/按钮 */

private:
    void refreshTitle();
    ViewType m_view = UtilizationView;
    QString m_cardName = QStringLiteral("mic0");
    QLabel *m_title = nullptr;             /* "mic0: Utilization View" */
    class QToolButton *m_btnUtil = nullptr, *m_btnHist = nullptr;
    PlotWidget *m_plot = nullptr;          /* 曲线（X 轴 Core Utilization） */
    CoreGridWidget *m_hist = nullptr;      /* Histogram 视图复用 */
    FieldWidget *m_temp = nullptr, *m_mem = nullptr, *m_power = nullptr;
};

#endif /* MICSMC_QT_CARDVIEW_H */
