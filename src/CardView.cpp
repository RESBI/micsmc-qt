/* CardView.cpp — 总览面板 / 卡面板 / 右侧字段控件 / 图例（复刻控制面板排版） */
#include "CardView.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QPainter>
#include <QPalette>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

#include "I18n.h"

/* 白画布 + 1px 蓝边 */
static QFrame *canvas(QWidget *parent, QLayout **out)
{
    auto *f = new QFrame(parent);
    f->setObjectName(QStringLiteral("DataPanel"));
    auto *v = new QVBoxLayout(f);
    v->setContentsMargins(4, 4, 4, 4);
    v->setSpacing(2);
    if (out)
        *out = v;
    return f;
}

/* ================= FieldWidget ================= */
/* ---- 告警门限与 overlay 配色 ----
 *
 * 温度三级告警按"黄 → 橙 → 红"递进，直接改数码管字色；
 * 门限值见 tempWarnLimit()/tempCautionLimit()/tempCritLimit()，
 * 三者均可用 QSettings 覆盖（键名与默认值见 README §6）。
 * 首级键沿用 MPSS 控制面板既有设置键名与默认 85 °C，便于沿用既有配置。
 */
static int cfgInt(const char *key, int def)
{
    QSettings s;
    bool ok = false;
    const int v = s.value(QString::fromLatin1(key), def).toInt(&ok);
    return ok ? v : def;
}

static int tempWarnLimit()
{
    return cfgInt("MicSettingsCpuStat/coreTempLimit", 85);
}
static int tempCautionLimit()
{
    return cfgInt("MicSettingsCpuStat/coreTempCaution", tempWarnLimit() + 10);
}
static int tempCritLimit()
{
    return cfgInt("MicSettingsCpuStat/coreTempCritical", tempWarnLimit() + 20);
}

/* overlay 配色：黄 / 橙 / 红 */
static const QColor kOverlayWarn(0xFF, 0xFF, 0x00);
static const QColor kOverlayCaution(0xFF, 0xAA, 0x7F);
static const QColor kOverlayCrit(0xFF, 0x00, 0x00);

static void applyLcdOverlay(QLCDNumber *lcd, int level)
{
    QPalette pal = lcd->palette();
    switch (level) {
    case 3:  pal.setColor(QPalette::WindowText, kOverlayCrit); break;
    case 2:  pal.setColor(QPalette::WindowText, kOverlayCaution); break;
    case 1:  pal.setColor(QPalette::WindowText, kOverlayWarn); break;
    default: pal.setColor(QPalette::WindowText, QColor(10, 10, 10)); break;
    }
    lcd->setPalette(pal);
}

/* ================= FieldWidget ================= */

FieldWidget::FieldWidget(Kind kind, QWidget *parent)
    : QWidget(parent), m_kind(kind)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(2, 3, 2, 3);
    v->setSpacing(2);

    QFont lf = font();
    lf.setPointSizeF(8);
    lf.setBold(true);
    lf.setFamily(QStringLiteral("Courier New"));

    if (m_kind == Temp) {
        /* 7 段数码管（黑字，约占栏高一半，上下居中） */
        v->addStretch(1);
        m_lcd = new QLCDNumber(this);
        m_lcd->setDigitCount(3);
        m_lcd->setSegmentStyle(QLCDNumber::Filled);
        m_lcd->setFrameShape(QFrame::NoFrame);
        m_lcd->setMinimumSize(64, 86);       /* 放大数码管 */
        m_lcd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QPalette pal = m_lcd->palette();
        pal.setColor(QPalette::Window, QColor(225, 225, 225));
        pal.setColor(QPalette::WindowText, QColor(10, 10, 10));
        m_lcd->setPalette(pal);
        m_lcd->display(QStringLiteral("--"));
        v->addWidget(m_lcd);
        v->addStretch(1);
    } else {
        m_value = new QLabel(this);
        m_value->setObjectName(QStringLiteral("FieldValue"));
        m_value->setAlignment(Qt::AlignHCenter);
        m_value->setFont(lf);
        v->addWidget(m_value);
        /* 竖条绘制区（自绘：圆角轮廓 + 底部蓝色填充） */
        m_barArea = new QWidget(this);
        m_barArea->setMinimumHeight(56);
        m_barArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        v->addWidget(m_barArea, 1);
    }

    m_l1 = new QLabel(this);
    m_l2 = new QLabel(this);
    m_l1->setAlignment(Qt::AlignHCenter);
    m_l2->setAlignment(Qt::AlignHCenter);
    m_l1->setFont(lf);
    m_l2->setFont(lf);
    v->addWidget(m_l1);
    v->addWidget(m_l2);

    retranslate();
}

void FieldWidget::retranslate()
{
    if (m_kind == Temp) {
        /* 系统态："Average Core"；卡片态："Processor Core" */
        m_l1->setText(m_avg ? L("sys_temp1") : L("cv_temp1"));
        m_l2->setText(L("cv_temp2b"));       /* "Temperature   °C" */
        m_l1->setVisible(true);
    } else if (m_kind == Mem) {
        /* 系统态两行 "Total"/"Memory Usage"；卡片态单行 "Memory Usage" */
        m_l1->setText(m_avg ? L("sys_mem1") : QString());
        m_l1->setVisible(m_avg);
        m_l2->setText(L("sys_mem2"));
    } else {
        m_l1->setText(m_avg ? L("sys_pow1") : QString());
        m_l1->setVisible(m_avg);
        m_l2->setText(L("sys_pow2"));
    }
}

void FieldWidget::setAverageMode(bool avg)
{
    m_avg = avg;
    retranslate();
}

void FieldWidget::paintEvent(QPaintEvent *)
{
    if (m_kind == Temp || !m_barArea)
        return;

    /* 竖直圆角条：灰轮廓 #ABA9AD + 蓝色填充 #2B60A4 */
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect g = m_barArea->geometry();
    const int bw = qMin(46, qMax(20, g.width() - 16));
    QRectF bar(g.center().x() - bw / 2.0, g.top(), bw, g.height());
    if (bar.height() < 8)
        return;

    p.setPen(QPen(QColor(0xAB, 0xA9, 0xAD), 1));
    p.setBrush(QColor(225, 225, 225));
    p.drawRoundedRect(bar.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);

    const double f = qBound(0.0, m_pct, 1.0);
    if (f > 0.001) {
        const double fh = (bar.height() - 3) * f;
        QRectF fill(bar.left() + 1.5, bar.bottom() - 1.5 - fh,
                    bar.width() - 3, fh);
        p.setPen(Qt::NoPen);

        /* 功耗条分区着色（进入限值区间的部分变黄 / 橙 / 红）
         * 用 clip 把同一填充按门限分段涂色 */
        const QColor kBase(0x2B, 0x60, 0xA4);
        p.save();
        p.setClipRect(fill);
        p.setBrush(kBase);
        p.drawRect(fill);
        auto paintZone = [&](double from, double to, const QColor &c) {
            if (m_pctWarn < 0 || to <= from)
                return;
            const double y0 = bar.bottom() - 1.5 - (bar.height() - 3) * from;
            const double y1 = bar.bottom() - 1.5 - (bar.height() - 3) * to;
            if (y1 >= y0)
                return;
            p.setBrush(c);
            p.drawRect(QRectF(fill.left(), y1, fill.width(), y0 - y1));
        };
        if (m_pctCrit > 0)
            paintZone(m_pctCrit, 1.0, kOverlayCrit);
        if (m_pctWarn > 0)
            paintZone(m_pctWarn, m_pctCrit > 0 ? m_pctCrit : 1.0,
                      kOverlayCaution);
        p.restore();

        /* 圆角裁边一次（保持圆角外观） */
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0xAB, 0xA9, 0xAD), 1));
        p.drawRoundedRect(bar.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
    }
}

void FieldWidget::setSnapshot(const Snapshot &s)
{
    if (m_kind == Temp) {
        if (s.die.ok) {
            m_lcd->display(int(s.die.c + 0.5));
            /* 三级 overlay（黄/橙/红）——门限来自原版配置键，默认 85/95/105 */
            int lv = 0;
            if (s.die.c >= tempCritLimit())         lv = 3;
            else if (s.die.c >= tempCautionLimit()) lv = 2;
            else if (s.die.c >= tempWarnLimit())    lv = 1;
            if (lv != m_level) {
                m_level = lv;
                applyLcdOverlay(m_lcd, lv);
            }
        } else {
            m_lcd->display(QStringLiteral("--"));
            if (m_level != 0) {
                m_level = 0;
                applyLcdOverlay(m_lcd, 0);
            }
        }
        return;
    }

    if (m_kind == Mem) {
        /* 值在上方（GB），条 = 已用占比（内存无门限告警） */
        if (s.memOk) {
            m_value->setText(L("unit_gb").arg(s.usedMb / 1024.0, 0, 'f', 2));
            m_pct = s.totalMb > 0 ? s.usedMb / s.totalMb : 0.0;
            update();
        }
    } else {
        /* 值 "98 Watts" 在上，条 = 功率/物理上限；门限用设备上报的 lmrk/hmrk
         * （Low setpoint=黄 / High setpoint=橙 / Shutdown=红，
         *  功耗进入 shutdown 上限 10% 内即显示 Critical） */
        if (s.powerOk) {
            m_value->setText(L("unit_watts").arg(s.instW, 0, 'f', 0));
            const double cap = s.limitOk && s.physW > 0 ? s.physW
                             : (s.limitOk && s.hmrkW > 0 ? s.hmrkW * 1.1 : 245.0);
            m_pct = s.instW / cap;
            if (s.limitOk && s.lmrkW > 0)
                m_pctWarn = s.lmrkW / cap;
            if (s.limitOk && s.hmrkW > 0)
                m_pctCrit = qMax(0.9, s.hmrkW / cap);   /* 10% 内视为临界 */
            update();
        }
    }
}

/* ================= LegendWidget（图例框） ================= */

LegendWidget::LegendWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(96);
    setFixedHeight(44);          /* 高 44px（两行 ×21px） */
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    retranslate();
}

void LegendWidget::retranslate()
{
    /* 图例文字："System [%]" / "User [%]" */
    m_name1 = L("leg_sys");
    m_name2 = L("leg_user");
    update();
}

void LegendWidget::setValues(double sysPct, double userPct)
{
    m_sys = sysPct;
    m_user = userPct;
    update();
}

void LegendWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(225, 225, 225));

    const QColor axis(0x08, 0x60, 0xA8);
    QRect r = rect().adjusted(0, 0, -1, -1);
    /* 立体边框（凸起 3D 框：外浅内深） */
    p.setPen(QColor(0x6B, 0x6B, 0x6B));
    p.drawLine(r.topLeft(), r.topRight());
    p.drawLine(r.topLeft(), r.bottomLeft());
    p.setPen(QColor(0xFF, 0xFF, 0xFF));
    p.drawLine(r.bottomLeft(), r.bottomRight());
    p.drawLine(r.topRight(), r.bottomRight());

    QFont f = p.font();
    f.setPointSizeF(8);
    f.setBold(true);
    f.setFamily(QStringLiteral("Courier New"));
    p.setFont(f);
    const QFontMetrics fm(f);

    const int rowH = (r.height() - 6) / 2;
    const QColor cSys(0xE1, 0x67, 0x62);    /* System = 红 */
    const QColor cUser(0x7D, 0x81, 0xE4);   /* User   = 紫 */
    auto row = [&](int idx, const QString &name, const QColor &c, double v) {
        Q_UNUSED(v);   /* 图例只显示系列名 */
        int y = r.top() + 3 + idx * rowH;
        p.fillRect(QRect(r.left() + 8, y + rowH / 2 - 4, 8, 8), c);
        p.setPen(axis);
        p.drawText(QRect(r.left() + 21, y, r.width() - 24, rowH),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   fm.elidedText(name, Qt::ElideRight, r.width() - 24));
    };
    row(0, m_name1, cSys, m_sys);     /* System [%] */
    row(1, m_name2, cUser, m_user);   /* User [%] */
    /* 中间分隔线 */
    p.setPen(QColor(0x9E, 0xA7, 0xB4));
    p.drawLine(r.left() + 1, r.top() + 3 + rowH,
               r.right() - 1, r.top() + 3 + rowH);
}

/* ================= SystemPanel（上带） ================= */

SystemPanel::SystemPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);   /* 左右边距由中央布局统一施加 */
    outer->setSpacing(0);
    auto *frame = canvas(this, nullptr);

    m_title = new QLabel(L("sys_title"), frame);   /* 标题（面板内左上角） */
    QFont tf = m_title->font();
    tf.setPointSizeF(9);
    tf.setBold(true);
    tf.setFamily(QStringLiteral("Courier New"));
    m_title->setFont(tf);

    auto *row = new QHBoxLayout();
    row->setSpacing(10);

    m_legend = new LegendWidget(frame);            /* 左列图例框 */
    {
        /* 图例靠上，下方留白 */
        auto *lcol = new QWidget(frame);
        auto *lv = new QVBoxLayout(lcol);
        lv->setContentsMargins(0, 14, 0, 0);
        lv->setSpacing(0);
        lv->addWidget(m_legend);
        lv->addStretch(1);
        row->addWidget(lcol, 11);
    }

    /* 主图：折线（System/User 双曲线 + 时间戳 + X 轴标题） */
    m_plot = new PlotWidget(
        { L("leg_sys"), L("leg_user") },
        { QColor(0xE1, 0x67, 0x62), QColor(0x7D, 0x81, 0xE4) },
        QStringLiteral("%"), frame);
    m_plot->setFixedRange(0, 100);
    m_plot->setXTitle(L("axis_avg_core_util"));
    row->addWidget(m_plot, 52);

    m_temp = new FieldWidget(FieldWidget::Temp, frame);
    m_temp->setAverageMode(true);
    m_mem = new FieldWidget(FieldWidget::Mem, frame);
    m_mem->setAverageMode(true);
    m_power = new FieldWidget(FieldWidget::Power, frame);
    m_power->setAverageMode(true);
    row->addWidget(m_temp, 10);
    row->addWidget(m_mem, 9);
    row->addWidget(m_power, 8);

    auto *v = static_cast<QVBoxLayout *>(frame->layout());
    v->addWidget(m_title);
    v->addLayout(row, 1);
    outer->addWidget(frame);
}

void SystemPanel::setSnapshot(const Snapshot &s)
{
    if (s.utilOk) {
        m_plot->push(s.ts, { s.sysPct, s.userPct });
        m_legend->setValues(s.sysPct, s.userPct);
    }
    m_temp->setSnapshot(s);
    m_mem->setSnapshot(s);
    m_power->setSnapshot(s);
}

/* 语言切换：标题 / 图例 / X 轴标题 / 三个字段标签 */
void SystemPanel::retranslate()
{
    m_title->setText(L("sys_title"));
    m_legend->retranslate();
    m_plot->setXTitle(L("axis_avg_core_util"));
    m_temp->retranslate();
    m_mem->retranslate();
    m_power->retranslate();
}

/* ================= DetailPanel（下带） ================= */

DetailPanel::DetailPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);   /* 左右边距由中央布局统一施加 */
    outer->setSpacing(0);
    auto *frame = canvas(this, nullptr);

    /* 标题行（面板内）：左标题，右上角两个视图切换小按钮 */
    auto *head = new QHBoxLayout();
    head->setSpacing(4);
    m_title = new QLabel(frame);
    QFont tf = m_title->font();
    tf.setPointSizeF(9);
    tf.setBold(true);
    tf.setFamily(QStringLiteral("Courier New"));
    m_title->setFont(tf);
    head->addWidget(m_title, 1);

    m_btnUtil = new QToolButton(frame);   /* :/utilization.png 语义 */
    m_btnHist = new QToolButton(frame);   /* :/histo.png 语义 */
    for (auto *b : { m_btnUtil, m_btnHist }) {
        b->setFixedSize(30, 20);
        b->setCheckable(true);
        b->setAutoRaise(true);
    }
    m_btnUtil->setText(L("vbtn_util"));
    m_btnHist->setText(L("vbtn_histo"));
    m_btnUtil->setToolTip(L("tip_go_util"));
    m_btnHist->setToolTip(L("tip_go_histo"));
    connect(m_btnUtil, &QToolButton::clicked, this, [this]() {
        setViewType(UtilizationView);
    });
    connect(m_btnHist, &QToolButton::clicked, this, [this]() {
        setViewType(HistogramView);
    });
    head->addWidget(m_btnUtil);
    head->addWidget(m_btnHist);

    auto *row = new QHBoxLayout();
    row->setSpacing(10);

    m_plot = new PlotWidget(
        { L("leg_sys"), L("leg_user") },
        { QColor(0xE1, 0x67, 0x62), QColor(0x7D, 0x81, 0xE4) },
        QStringLiteral("%"), frame);
    m_plot->setFixedRange(0, 100);
    m_plot->setXTitle(L("axis_core_util"));
    m_hist = new CoreGridWidget(frame);           /* Histogram 视图复用 */
    m_hist->hide();
    row->addWidget(m_plot, 58);
    row->addWidget(m_hist, 58);

    m_temp = new FieldWidget(FieldWidget::Temp, frame);   /* 卡片态：非 avg */
    m_mem = new FieldWidget(FieldWidget::Mem, frame);
    m_power = new FieldWidget(FieldWidget::Power, frame);
    row->addWidget(m_temp, 12);
    row->addWidget(m_mem, 11);
    row->addWidget(m_power, 10);

    auto *v = static_cast<QVBoxLayout *>(frame->layout());
    v->addLayout(head);
    v->addLayout(row, 1);
    outer->addWidget(frame);
    refreshTitle();
}

void DetailPanel::setViewType(ViewType t)
{
    m_view = t;
    m_btnUtil->setChecked(t == UtilizationView);
    m_btnHist->setChecked(t == HistogramView);
    refreshTitle();
    update();
}

void DetailPanel::setCardName(const QString &name)
{
    if (!name.isEmpty())
        m_cardName = name;
    refreshTitle();
}

/* 语言切换：标题 / 图例 / X 轴标题 / 按钮文本与提示 / 三个字段标签 */
void DetailPanel::retranslate()
{
    refreshTitle();
    m_plot->setXTitle(L("axis_core_util"));
    m_plot->setSeriesNames({ L("leg_sys"), L("leg_user") });
    m_btnUtil->setText(L("vbtn_util"));
    m_btnHist->setText(L("vbtn_histo"));
    m_btnUtil->setToolTip(L("tip_go_util"));
    m_btnHist->setToolTip(L("tip_go_histo"));
    m_temp->retranslate();
    m_mem->retranslate();
    m_power->retranslate();
}

void DetailPanel::refreshTitle()
{
    /* 原版模板 "%1: %2"（卡名: 视图名），如 "mic0: Utilization View" */
    static const char *KEYS[3] = { "nav_view_util", "nav_view_histo",
                                   "nav_view_hist" };
    m_title->setText(L("nav_title").arg(m_cardName).arg(L(KEYS[int(m_view)])));
}

void DetailPanel::setSnapshot(const Snapshot &s)
{
    const bool hist = (m_view == HistogramView);
    m_hist->setVisible(hist);
    m_plot->setVisible(!hist);
    if (hist)
        m_hist->setValues(s.perCore);
    else if (s.utilOk)
        m_plot->push(s.ts, { s.sysPct, s.userPct });

    m_temp->setSnapshot(s);
    m_mem->setSnapshot(s);
    m_power->setSnapshot(s);
}
