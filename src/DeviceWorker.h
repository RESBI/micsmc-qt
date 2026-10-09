/* DeviceWorker.h — 工作线程：libmicmgmt 轮询与控制动作（micsmc-qt）
 *
 * 所有 mic_* 调用都发生在本对象所在的 QThread（moveToThread 之后），
 * UI 线程只通过 queued slot/signal 交互。
 * 利用率按 MPSS 范式差分计算：user/sys 计数器差分 / (threads_per_core ×
 * jiffy 差分 × 核心数) × 100，与上游 CLI 的口径一致。
 */
#ifndef MICSMC_QT_DEVICEWORKER_H
#define MICSMC_QT_DEVICEWORKER_H

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVector>

#include <cstdint>
#include <vector>
#include <miclib.h>

#include "Snapshot.h"

class DeviceWorker : public QObject {
    Q_OBJECT
public:
    explicit DeviceWorker(QObject *parent = nullptr);
    ~DeviceWorker() override;

public slots:
    void start();               /* 在工作线程内调用：建定时器、首次枚举 */
    void selectDevice(int idx); /* UI 选择设备（-1 = 自动取第一个） */
    void setTurboAt(int dev, bool enable);   /* Advanced Settings 批量应用 */
    void setLedAt(int dev, bool enable);
    void maintEnter();
    void maintLeave();

signals:
    void devicesReady(const QStringList &names, const QVector<int> &ready);
    void snapshotReady(const Snapshot &s);
    void actionDone(const QString &what, bool ok, const QString &err);
    void fatal(const QString &why);   /* 枚举不到设备等致命错误 */

private slots:
    void tick();

private:
    struct DevState {
        int  number = 0;
        mic_device *mdh = nullptr;
        mic_core_util *cutil = nullptr;
        /* 利用率差分状态 */
        bool havePrev = false;
        int  ncores = 0;
        std::vector<uint64_t> userPrev, sysPrev;
        uint64_t jiffyPrev = 0, userSumPrev = 0, sysSumPrev = 0;
    };

    void refreshDevices();
    bool ensureOpen(DevState &d);
    Snapshot collect(DevState &d);
    void collectCoreUtil(DevState &d, Snapshot &s);

    QVector<DevState> m_devs;
    QStringList m_names;
    int  m_cur = -1;
    int  m_ticks = 0;
    QTimer *m_timer = nullptr;
};

#endif /* MICSMC_QT_DEVICEWORKER_H */
