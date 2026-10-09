/* MainWindow.h — 主窗口（控制面板排版复刻）
 *
 * 窗口结构：
 *   蓝色工具栏：左 [Cards ▾][Advanced ▾]，右 "Cards: N  Memory: xx GB  Cores: N"
 *   中央面板（rgb(225,225,225) + 蓝边）：SystemPanel
 *   蓝色状态栏
 *   选卡后追加：标题行 "mic0: Utilization View" + ◀▶ 与 DetailPanel
 */
#ifndef MICSMC_QT_MAINWINDOW_H
#define MICSMC_QT_MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>

#include "CardView.h"
#include "Snapshot.h"

class QComboBox;
class QMenu;
class QPushButton;
class QThread;
class DeviceWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onDevices(const QStringList &names, const QVector<int> &ready);
    void onSnapshot(const Snapshot &s);
    void onActionDone(const QString &what, bool ok, const QString &err);
    void onFatal(const QString &why);
    void onLangToggle();
    void onSettings();

private:
    QWidget *buildNavBar();
    void retranslate();
    void rebuildCardMenu();
    void refreshNavFields();
    void refreshDetailCard();
    void showDetailView(bool on);

    /* NavBar 计数字段 */
    QLabel *m_navCards = nullptr, *m_navMem = nullptr, *m_navCores = nullptr;
    QMenu *m_cardMenu = nullptr;
    class QToolButton *m_btnCards = nullptr, *m_btnAdv = nullptr;
    /* 菜单项（语言切换时需重设文本，故保存指针） */
    class QAction *m_actShowAll = nullptr, *m_actHideAll = nullptr;
    class QAction *m_actErrLog = nullptr, *m_actSettings = nullptr;
    class QAction *m_actUpdate = nullptr, *m_actInfo = nullptr;
    class QAction *m_actAbout = nullptr, *m_actLang = nullptr;

    SystemPanel *m_systemPanel = nullptr;
    DetailPanel *m_detailPanel = nullptr;

    QComboBox *m_devSel = nullptr;
    QThread *m_thread = nullptr;
    DeviceWorker *m_worker = nullptr;
    bool m_haveSnapshot = false;
    Snapshot m_lastSnapshot;
    QStringList m_names;
    QVector<int> m_ready;
    int m_nCores = 0;
    double m_totalMemGb = 0;
};

#endif /* MICSMC_QT_MAINWINDOW_H */
