/* Snapshot.h — 跨线程传递的设备快照（micsmc-qt）
 *
 * DeviceWorker（工作线程）采集 → queued signal → MainWindow（UI 线程）渲染。
 * 采集线程按固定周期填充该结构，经 queued signal 交给 UI 线程渲染。
 */
#ifndef MICSMC_QT_SNAPSHOT_H
#define MICSMC_QT_SNAPSHOT_H

#include <QMetaType>
#include <QString>
#include <QVector>

struct Snapshot {
    /* 身份 */
    QString device;
    int     index = 0;
    bool    ready = false;
    QString postCode;
    QString uos, flash, driver;

    /* 温度（°C） */
    struct Temp { double c = 0; bool ok = false; };
    Temp die, gddr, fanin, fanout, vccp, vddg, vddq;
    quint32 fanRpm = 0;
    bool    fanOk = false;

    /* 功耗（W） */
    double totalW = 0, instW = 0, maxInstW = 0;
    double lmrkW = 0, hmrkW = 0, physW = 0;
    bool   powerOk = false, limitOk = false;

    /* 频率（GHz） */
    double freqGhz = 0;
    bool   freqOk = false;

    /* 内存（MB，KiB/1024，与上游 CLI 一致） */
    double totalMb = 0, freeMb = 0, usedMb = 0, usedPct = 0;
    bool   memOk = false;

    /* ECC */
    bool ecc = false, eccOk = false;

    /* 核心与利用率 */
    int  ncores = 0, threadsCore = 0;
    bool utilOk = false;              /* 第二个采集周期起有效 */
    double userPct = 0, sysPct = 0, idlePct = 0;
    QVector<double> perCore;          /* 每核 user+sys（%），长度 ncores */

    /* turbo / LED / 维护模式 */
    bool turboAvail = false, turboActive = false, turboEnabled = false;
    bool turboOk = false;
    bool led = false, ledOk = false;
    bool maint = false, maintOk = false;

    QString error;
    double  ts = 0;
};

Q_DECLARE_METATYPE(Snapshot)

#endif /* MICSMC_QT_SNAPSHOT_H */
