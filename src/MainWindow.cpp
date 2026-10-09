/* MainWindow.cpp — 主窗口（原版排版严格复刻，规格书 v3） */
#include "MainWindow.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLocale>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QThread>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "DeviceWorker.h"
#include "I18n.h"
#include "SettingsDialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    /* 语言：优先读用户上次选择；否则跟随系统区域设置（zh* → 中文，其余英文） */
    QSettings cfg;
    const QString defLang =
        QLocale::system().name().startsWith(QLatin1String("zh"))
            ? QStringLiteral("zh") : QStringLiteral("en");
    I18n::setLang(cfg.value(QStringLiteral("lang"), defLang).toString());

    setWindowTitle(QStringLiteral(
        "Intel(R) Xeon Phi(TM) Coprocessor Platform Control Panel"));
    /* 默认窗口比例参照控制面板；内容区按 604×312 基准设计 */
    resize(1024, 330);
    setMinimumSize(640, 260);

    /* ---- 工作线程 ---- */
    m_thread = new QThread(this);
    m_worker = new DeviceWorker;
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::started, m_worker, &DeviceWorker::start);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &DeviceWorker::devicesReady, this, &MainWindow::onDevices);
    connect(m_worker, &DeviceWorker::snapshotReady, this, &MainWindow::onSnapshot);
    connect(m_worker, &DeviceWorker::actionDone, this, &MainWindow::onActionDone);
    connect(m_worker, &DeviceWorker::fatal, this, &MainWindow::onFatal);
    m_thread->start();

    /* ---- 纵向结构：NavBar 灰条（rgb235）+ 上下窗口底色 ----
       左右 6px 边距在此统一施加：NavBar 横幅与下方主框同宽 */
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(6, 11, 6, 0);
    root->setSpacing(12);                    /* NavBar 与面板间留白 12px */
    root->addWidget(buildNavBar(), 0);
    m_systemPanel = new SystemPanel(central);
    root->addWidget(m_systemPanel, 1);
    m_detailPanel = new DetailPanel(central);
    m_detailPanel->hide();
    root->addWidget(m_detailPanel, 42);
    setCentralWidget(central);
    statusBar()->showMessage(QStringLiteral(" "));
}

/* NavBar：满宽 rgb(235,235,235) 灰条
 * 左 [Cards ▾][Advanced ▾] ；右 "Cards: N  Memory: xx GB  Cores: N" */
QWidget *MainWindow::buildNavBar()
{
    auto *bar = new QWidget(this);
    bar->setObjectName(QStringLiteral("NavBar"));
    bar->setFixedHeight(26);
    auto *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(12, 3, 12, 3);
    lay->setSpacing(8);

    auto *btnCards = new QToolButton(bar);
    m_btnCards = btnCards;
    btnCards->setText(L("tool_cards"));
    btnCards->setPopupMode(QToolButton::InstantPopup);
    m_cardMenu = new QMenu(btnCards);
    m_cardMenu->setObjectName(QStringLiteral("CardMenu"));
    m_cardMenu->addSeparator();
    m_actShowAll = m_cardMenu->addAction(L("menu_show_all"), this, [this]() {
        showDetailView(true);
    });
    m_actHideAll = m_cardMenu->addAction(L("menu_hide_all"), this, [this]() {
        showDetailView(false);
    });
    btnCards->setMenu(m_cardMenu);
    lay->addWidget(btnCards);

    auto *btnAdv = new QToolButton(bar);
    m_btnAdv = btnAdv;
    btnAdv->setText(L("tool_advanced"));
    btnAdv->setPopupMode(QToolButton::InstantPopup);
    auto *mAdv = new QMenu(btnAdv);
    m_actErrLog = mAdv->addAction(L("menu_error_log"), this, []() {});
    m_actSettings = mAdv->addAction(L("menu_settings"), this,
                                    &MainWindow::onSettings);
    m_actUpdate = mAdv->addAction(L("menu_update_app"), this, []() {});
    m_actInfo = mAdv->addAction(L("menu_info"), this, []() {});
    m_actAbout = mAdv->addAction(L("menu_about"), this, [this]() {
        QMessageBox::about(this, QStringLiteral("micsmc-qt"), L("about_text"));
    });
    mAdv->addSeparator();
    m_actLang = mAdv->addAction(L("btn_lang"), this, &MainWindow::onLangToggle);
    btnAdv->setMenu(mAdv);
    lay->addWidget(btnAdv);

    lay->addStretch(1);

    auto addField = [&](const char *key, QLabel **out) {
        auto *l = new QLabel(bar);
        l->setObjectName(QStringLiteral("ToolbarField"));
        l->setText(L(key));
        lay->addWidget(l);
        *out = l;
    };
    addField("nav_cards", &m_navCards);
    lay->addSpacing(18);
    addField("nav_memory", &m_navMem);
    lay->addSpacing(18);
    addField("nav_cores", &m_navCores);

    /* 隐藏设备选择器（菜单驱动 worker） */
    m_devSel = new QComboBox(bar);
    m_devSel->hide();
    connect(m_devSel, &QComboBox::currentIndexChanged,
            m_worker, &DeviceWorker::selectDevice);
    connect(m_devSel, &QComboBox::currentIndexChanged, this, [this](int) {
        refreshDetailCard();
    });
    lay->addWidget(m_devSel);
    return bar;
}

/* 工具栏：左侧两个下拉按钮，右侧 Cards:/Memory:/Cores: 计数 */
void MainWindow::onSettings()
{
    QVector<QPair<bool, bool>> cur;
    for (int i = 0; i < m_names.size(); i++) {
        if (m_haveSnapshot && m_lastSnapshot.index == i)
            cur.append({ m_lastSnapshot.turboEnabled, m_lastSnapshot.led });
        else
            cur.append({ false, false });
    }
    SettingsDialog dlg(m_names, cur, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    for (const auto &it : dlg.intents()) {
        if (it.turbo) {
            const bool want = !(it.devIndex < cur.size() ? cur[it.devIndex].first : false);
            QMetaObject::invokeMethod(m_worker, [this, it, want]() {
                m_worker->setTurboAt(it.devIndex, want);
            }, Qt::QueuedConnection);
        }
        if (it.led) {
            const bool want = !(it.devIndex < cur.size() ? cur[it.devIndex].second : false);
            QMetaObject::invokeMethod(m_worker, [this, it, want]() {
                m_worker->setLedAt(it.devIndex, want);
            }, Qt::QueuedConnection);
        }
    }
}

void MainWindow::onLangToggle()
{
    I18n::setLang(I18n::isEn() ? QStringLiteral("zh")
                               : QStringLiteral("en"));
    QSettings cfg;
    cfg.setValue(QStringLiteral("lang"), I18n::langStorage());
    retranslate();
}

void MainWindow::retranslate()
{
    /* 1. NavBar 两个下拉按钮 */
    if (m_btnCards)
        m_btnCards->setText(L("tool_cards"));
    if (m_btnAdv)
        m_btnAdv->setText(L("tool_advanced"));

    /* 2. 下拉菜单项（此前遗漏，导致菜单语言不跟随） */
    if (m_actShowAll)  m_actShowAll->setText(L("menu_show_all"));
    if (m_actHideAll)  m_actHideAll->setText(L("menu_hide_all"));
    if (m_actErrLog)   m_actErrLog->setText(L("menu_error_log"));
    if (m_actSettings) m_actSettings->setText(L("menu_settings"));
    if (m_actUpdate)   m_actUpdate->setText(L("menu_update_app"));
    if (m_actInfo)     m_actInfo->setText(L("menu_info"));
    if (m_actAbout)    m_actAbout->setText(L("menu_about"));
    if (m_actLang)     m_actLang->setText(L("btn_lang"));

    /* 3. NavBar 计数字段 + 详情面板卡名 */
    refreshNavFields();
    refreshDetailCard();

    /* 4. 两个面板内部文本（标题 / 图例 / X 轴标题 / 字段标签 / 视图按钮） */
    m_systemPanel->retranslate();
    m_detailPanel->retranslate();

    /* 5. 用最近快照重刷一次数值文本（GB/Watts 等单位前缀也随语言变） */
    if (m_haveSnapshot)
        onSnapshot(m_lastSnapshot);
}

/* 原版行为：从 Cards 菜单选卡后 "enlarges the application frame"
 * 显示 Card View（下带）；Hide All 收起 */
void MainWindow::showDetailView(bool on)
{
    m_detailPanel->setVisible(on);
    refreshDetailCard();
    if (on) {
        const int dh = 250;
        if (height() < dh + 140)
            resize(width(), dh + 140);
    } else {
        resize(width(), 330);
    }
}

/* 详情面板标题使用当前卡名（原版 "mic0: Utilization View"） */
void MainWindow::refreshDetailCard()
{
    const int idx = m_devSel->currentIndex();
    if (idx >= 0 && idx < m_names.size())
        m_detailPanel->setCardName(m_names[idx]);
}

/* ------------------------------------------------------------------ */

void MainWindow::onDevices(const QStringList &names, const QVector<int> &ready)
{
    m_names = names;
    m_ready = ready;
    int cur = m_devSel->currentIndex();
    if (cur < 0)
        cur = 0;
    {
        QSignalBlocker blk(m_devSel);
        m_devSel->clear();
        for (const QString &n : names)
            m_devSel->addItem(n);
        m_devSel->setCurrentIndex(qBound(0, cur, qMax(0, names.size() - 1)));
    }
    rebuildCardMenu();
    refreshDetailCard();
}

void MainWindow::rebuildCardMenu()
{
    auto *mCard = m_cardMenu;
    if (!mCard)
        return;
    QList<QAction *> acts = mCard->actions();
    while (acts.size() > 3) {   /* 保留分隔符 + Show All + Hide All */
        QAction *a = acts.takeFirst();
        mCard->removeAction(a);
        a->deleteLater();
    }
    QAction *sep = acts.value(0);
    for (int i = 0; i < m_names.size(); i++) {
        auto *a = new QAction(m_names[i], this);   /* 原版 "mic%1" */
        a->setCheckable(true);
        a->setChecked(m_devSel->currentIndex() == i);
        const int idx = i;
        connect(a, &QAction::triggered, this, [this, idx]() {
            m_devSel->setCurrentIndex(idx);
            showDetailView(true);          /* 原版：选卡 → 展开下带 */
        });
        if (sep)
            mCard->insertAction(sep, a);
        else
            mCard->addAction(a);
    }
}

/* NavBar 计数字段 */
void MainWindow::refreshNavFields()
{
    m_navCards->setText(L("nav_cards").arg(m_names.size()));
    m_navMem->setText(L("nav_memory").arg(m_totalMemGb, 0, 'f', 0));
    m_navCores->setText(L("nav_cores").arg(m_nCores));
}

/* 标题行（原版 "mic%1: %2" + 视图名） */
void MainWindow::onSnapshot(const Snapshot &s)
{
    m_haveSnapshot = true;
    m_lastSnapshot = s;

    /* NavBar 计数 */
    if (s.ncores > 0)
        m_nCores = s.ncores;
    if (s.memOk && s.totalMb > 0)
        m_totalMemGb = s.totalMb / 1024.0;
    refreshNavFields();

    refreshDetailCard();

    /* 上带 + 下带 */
    m_systemPanel->setSnapshot(s);
    m_detailPanel->setSnapshot(s);

    if (!s.error.isEmpty())
        statusBar()->showMessage(s.error);
}

void MainWindow::onActionDone(const QString &what, bool ok, const QString &err)
{
    const QString disp = what == QStringLiteral("turbo") ? L("a_turbo")
                         : what == QStringLiteral("led")  ? L("a_led")
                                                          : L("a_maint");
    statusBar()->showMessage(ok
        ? L("act_done").arg(disp)
        : L("act_failed").arg(disp, err), 5000);
}

MainWindow::~MainWindow()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->wait(3000);
    }
}

void MainWindow::onFatal(const QString &why)
{
    QMessageBox::critical(this, QStringLiteral("micsmc-qt"), why);
    close();
}
