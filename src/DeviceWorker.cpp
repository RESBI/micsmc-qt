/* DeviceWorker.cpp — 工作线程：libmicmgmt 轮询与控制动作 */
#include "DeviceWorker.h"

#include <QThread>

#include <cstdio>
#include <cstring>
#include <ctime>

#include "I18n.h"

/* ------------------------------------------------------------------ */
/* 工具                                                                */

static double nowSec()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return double(ts.tv_sec) + double(ts.tv_nsec) / 1e9;
}

static void readDriverVersion(QString &out)
{
    FILE *f = fopen("/sys/class/mic/ctrl/version", "r");
    if (!f)
        return;
    char buf[128];
    if (fgets(buf, sizeof(buf), f))
        out = QString::fromUtf8(buf).trimmed();
    fclose(f);
}

static QString micErr()
{
    const char *s = mic_get_error_string();
    return QString::fromUtf8(s ? s : "unknown micmgmt error");
}

/* 读 sysfs 属性（两段式，同 CLI get_device_attrib） */
static QString readAttr(mic_device *mdh, const char *attr)
{
    size_t sz = 0;
    if (mic_get_sysfs_attribute(mdh, attr, nullptr, &sz) != E_MIC_SUCCESS
        || sz == 0)
        return QString();
    QByteArray buf(int(sz) + 1, '\0');
    if (mic_get_sysfs_attribute(mdh, attr, buf.data(), &sz) != E_MIC_SUCCESS)
        return QString();
    return QString::fromUtf8(buf.constData());
}

/* 在线判定：sysfs state == "online"（mic_in_ready_state 是待刷写态，别用） */
static bool devOnline(mic_device *mdh, QString *stateOut = nullptr)
{
    const QString st = readAttr(mdh, "state");
    if (stateOut)
        *stateOut = st;
    return st == QStringLiteral("online");
}

/* ------------------------------------------------------------------ */

DeviceWorker::DeviceWorker(QObject *parent)
    : QObject(parent)
{
}

DeviceWorker::~DeviceWorker()
{
    for (auto &d : m_devs) {
        if (d.cutil)
            mic_free_core_util(d.cutil);
        if (d.mdh)
            mic_close_device(d.mdh);
    }
}

void DeviceWorker::start()
{
    refreshDevices();
    if (m_devs.isEmpty()) {
        emit fatal(L("fatal_nodev"));
        return;
    }
    if (m_cur < 0 || m_cur >= m_devs.size())
        m_cur = 0;
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &DeviceWorker::tick);
    m_timer->start();
    tick();
}

void DeviceWorker::refreshDevices()
{
    mic_devices_list *list = nullptr;
    if (mic_get_devices(&list) != E_MIC_SUCCESS || !list)
        return;
    int n = 0;
    mic_get_ndevices(list, &n);

    /* 数量变化：释放重建（简单起见；热插拔少见） */
    if (n != m_devs.size()) {
        for (auto &d : m_devs) {
            if (d.cutil)
                mic_free_core_util(d.cutil);
            if (d.mdh)
                mic_close_device(d.mdh);
        }
        m_devs.clear();
        for (int i = 0; i < n; i++) {
            DevState d;
            mic_get_device_at_index(list, i, &d.number);
            mic_alloc_core_util(&d.cutil);
            m_devs.append(d);
        }
    }
    mic_free_devices(list);

    m_names.clear();
    QVector<int> ready;
    for (auto &d : m_devs) {
        QString name = QStringLiteral("mic%1").arg(d.number);
        if (ensureOpen(d)) {
            const char *nm = mic_get_device_name(d.mdh);
            if (nm && *nm)
                name = QString::fromUtf8(nm);
            ready.append(devOnline(d.mdh) ? 1 : 0);
        } else {
            ready.append(0);
        }
        m_names << name;
    }
    emit devicesReady(m_names, ready);
}

bool DeviceWorker::ensureOpen(DevState &d)
{
    if (d.mdh)
        return true;
    if (mic_open_device(&d.mdh, uint32_t(d.number)) != E_MIC_SUCCESS) {
        d.mdh = nullptr;
        return false;
    }
    return true;
}

void DeviceWorker::tick()
{
    if (++m_ticks % 15 == 0)
        refreshDevices();
    if (m_cur < 0 || m_cur >= m_devs.size())
        return;
    emit snapshotReady(collect(m_devs[m_cur]));
}

/* ------------------------------------------------------------------ */
/* 采集                                                                  */

void DeviceWorker::collectCoreUtil(DevState &d, Snapshot &s)
{
    if (!d.cutil || mic_update_core_util(d.mdh, d.cutil) != E_MIC_SUCCESS)
        return;

    uint16_t ncores = 0, thr = 0;
    uint64_t jiffy = 0, usum = 0, ssum = 0;
    mic_get_num_cores(d.cutil, &ncores);
    mic_get_threads_core(d.cutil, &thr);
    mic_get_jiffy_counter(d.cutil, &jiffy);
    mic_get_user_sum(d.cutil, &usum);
    mic_get_sys_sum(d.cutil, &ssum);
    s.ncores = ncores;
    s.threadsCore = thr;

    if (ncores == 0)
        return;

    std::vector<uint64_t> un(ncores), sn(ncores);
    if (mic_get_user_counters(d.cutil, un.data()) != E_MIC_SUCCESS ||
        mic_get_sys_counters(d.cutil, sn.data()) != E_MIC_SUCCESS)
        return;

    const double jiffyD = double(jiffy - d.jiffyPrev);
    if (d.havePrev && thr > 0 && jiffyD > 0) {
        /* 设备级：micsmc_helper::compute_dev_utilization 同式 */
        double userD = double(usum - d.userSumPrev);
        double sysD  = double(ssum  - d.sysSumPrev);
        double u = (userD * 100.0 / (double(thr) * jiffyD)) / double(ncores);
        double y = (sysD  * 100.0 / (double(thr) * jiffyD)) / double(ncores);
        if (u > 100.0) u = 100.0;
        if (y > 100.0) y = 100.0;
        s.userPct = u;
        s.sysPct  = y;
        s.idlePct = qMax(0.0, 100.0 - u - y);
        s.perCore.resize(ncores);
        for (int i = 0; i < ncores; i++) {
            double du = double(un[i] - d.userPrev[i]);
            double ds = double(sn[i] - d.sysPrev[i]);
            double p = (du + ds) * 100.0 / (double(thr) * jiffyD);
            s.perCore[i] = qBound(0.0, p, 100.0);
        }
        s.utilOk = true;
    }
    d.userPrev = std::move(un);
    d.sysPrev  = std::move(sn);
    d.ncores   = ncores;
    d.havePrev = true;
    d.jiffyPrev = jiffy;
    d.userSumPrev = usum;
    d.sysSumPrev = ssum;
}

Snapshot DeviceWorker::collect(DevState &d)
{
    Snapshot s;
    s.ts = nowSec();
    s.index = m_cur;

    if (!ensureOpen(d)) {
        s.error = QStringLiteral("open mic%1 failed: %2")
                      .arg(d.number).arg(micErr());
        return s;
    }
    const char *nm = mic_get_device_name(d.mdh);
    s.device = QString::fromUtf8(nm && *nm ? nm : "mic?");

    s.ready = devOnline(d.mdh, &s.postCode);     /* state 即状态串 */
    {
        char pc[64] = "";
        size_t sz = sizeof(pc);
        if (mic_get_post_code(d.mdh, pc, &sz) == E_MIC_SUCCESS && pc[0])
            s.postCode = QStringLiteral("POST %1")
                             .arg(QString::fromUtf8(pc));
    }

    mic_version_info *vi = nullptr;
    if (mic_get_version_info(d.mdh, &vi) == E_MIC_SUCCESS && vi) {
        char buf[128];
        size_t sz = sizeof(buf);
        if (mic_get_uos_version(vi, buf, &sz) == E_MIC_SUCCESS)
            s.uos = QString::fromUtf8(buf);
        sz = sizeof(buf);
        if (mic_get_flash_version(vi, buf, &sz) == E_MIC_SUCCESS)
            s.flash = QString::fromUtf8(buf);
        mic_free_version_info(vi);
    }
    readDriverVersion(s.driver);

    if (!s.ready)
        return s;

    mic_thermal_info *ti = nullptr;
    if (mic_get_thermal_info(d.mdh, &ti) == E_MIC_SUCCESS && ti) {
        uint32_t die = 0; int ok = 0;
        if (mic_get_die_temp(ti, &die) == E_MIC_SUCCESS) {
            s.die.c = double(die);
            s.die.ok = (mic_is_die_temp_valid(ti, &ok) == E_MIC_SUCCESS)
                           ? ok : true;
        }
        auto t16 = [&](Snapshot::Temp &t,
                       int (*gv)(mic_thermal_info *, uint16_t *),
                       int (*gvok)(mic_thermal_info *, int *)) {
            uint16_t v = 0; int vok = 0;
            if (gv(ti, &v) == E_MIC_SUCCESS) {
                t.c = double(v);
                t.ok = (gvok(ti, &vok) == E_MIC_SUCCESS) ? vok : true;
            }
        };
        t16(s.gddr,   mic_get_gddr_temp,   mic_is_gddr_temp_valid);
        t16(s.fanin,  mic_get_fanin_temp,  mic_is_fanin_temp_valid);
        t16(s.fanout, mic_get_fanout_temp, mic_is_fanout_temp_valid);
        t16(s.vccp,   mic_get_vccp_temp,   mic_is_vccp_temp_valid);
        t16(s.vddg,   mic_get_vddg_temp,   mic_is_vddg_temp_valid);
        t16(s.vddq,   mic_get_vddq_temp,   mic_is_vddq_temp_valid);
        if (mic_get_fan_rpm(ti, &s.fanRpm) == E_MIC_SUCCESS)
            s.fanOk = true;
        mic_free_thermal_info(ti);
    }

    mic_power_util_info *pi = nullptr;
    if (mic_get_power_utilization_info(d.mdh, &pi) == E_MIC_SUCCESS && pi) {
        uint32_t w0 = 0, inst = 0, mx = 0;
        mic_get_total_power_readings_w0(pi, &w0);   /* µW */
        mic_get_inst_power_readings(pi, &inst);
        mic_get_max_inst_power_readings(pi, &mx);
        s.totalW = double(w0) / 1e6;
        s.instW  = double(inst) / 1e6;
        s.maxInstW = double(mx) / 1e6;
        s.powerOk = true;
        mic_free_power_utilization_info(pi);
    }
    mic_power_limit *pl = nullptr;
    if (mic_get_power_limit(d.mdh, &pl) == E_MIC_SUCCESS && pl) {
        uint32_t lm = 0, hm = 0, ph = 0;
        mic_get_power_lmrk(pl, &lm);
        mic_get_power_hmrk(pl, &hm);
        mic_get_power_phys_limit(pl, &ph);
        s.lmrkW = double(lm); s.hmrkW = double(hm); s.physW = double(ph);
        s.limitOk = true;
        mic_free_power_limit(pl);
    }

    mic_cores_info *ci = nullptr;
    if (mic_get_cores_info(d.mdh, &ci) == E_MIC_SUCCESS && ci) {
        uint32_t khz = 0;
        if (mic_get_cores_frequency(ci, &khz) == E_MIC_SUCCESS) {
            s.freqGhz = double(khz) / 1e6;
            s.freqOk = true;
        }
        mic_free_cores_info(ci);
    }

    mic_memory_util_info *mi = nullptr;
    if (mic_get_memory_utilization_info(d.mdh, &mi) == E_MIC_SUCCESS && mi) {
        uint32_t tot = 0, avail = 0;
        mic_get_total_memory_size(mi, &tot);        /* KiB */
        mic_get_available_memory_size(mi, &avail);
        s.totalMb = double(tot) / 1024.0;
        s.freeMb  = double(avail) / 1024.0;
        s.usedMb  = s.totalMb - s.freeMb;
        s.usedPct = s.totalMb > 0 ? s.usedMb * 100.0 / s.totalMb : 0.0;
        s.memOk = true;
        mic_free_memory_utilization_info(mi);
    }

    mic_device_mem *mem = nullptr;
    if (mic_get_memory_info(d.mdh, &mem) == E_MIC_SUCCESS && mem) {
        uint16_t ecc = 0;
        if (mic_get_ecc_mode(mem, &ecc) == E_MIC_SUCCESS) {
            s.ecc = (ecc != 0);
            s.eccOk = true;
        }
        mic_free_memory_info(mem);
    }

    collectCoreUtil(d, s);

    mic_turbo_info *tbi = nullptr;
    if (mic_get_turbo_state_info(d.mdh, &tbi) == E_MIC_SUCCESS && tbi) {
        uint32_t valid = 0, state = 0, mode = 0;
        mic_get_turbo_state_valid(tbi, &valid);
        mic_get_turbo_state(tbi, &state);
        mic_get_turbo_mode(tbi, &mode);
        s.turboAvail = valid; s.turboActive = state; s.turboEnabled = mode;
        s.turboOk = true;
        mic_free_turbo_info(tbi);
    }
    uint32_t led = 0;
    if (mic_get_led_alert(d.mdh, &led) == E_MIC_SUCCESS) {
        s.led = led; s.ledOk = true;
    }
    int mm = 0;
    if (mic_in_maint_mode(d.mdh, &mm) == E_MIC_SUCCESS) {
        s.maint = mm; s.maintOk = true;
    }
    return s;
}

/* ------------------------------------------------------------------ */
/* 控制动作                                                            */

void DeviceWorker::selectDevice(int idx)
{
    if (idx >= 0 && idx < m_devs.size())
        m_cur = idx;
}

void DeviceWorker::setTurboAt(int dev, bool enable)
{
    if (dev < 0 || dev >= m_devs.size())
        return;
    auto &d = m_devs[dev];
    if (!ensureOpen(d)) {
        emit actionDone(QStringLiteral("turbo"), false, micErr());
        return;
    }
    uint32_t v = enable ? 1u : 0u;               /* miclib.h: uint32_t * */
    int r = mic_set_turbo_mode(d.mdh, &v);
    emit actionDone(QStringLiteral("turbo"), r == E_MIC_SUCCESS,
                    r == E_MIC_SUCCESS ? QString() : micErr());
}

void DeviceWorker::setLedAt(int dev, bool enable)
{
    if (dev < 0 || dev >= m_devs.size())
        return;
    auto &d = m_devs[dev];
    if (!ensureOpen(d)) {
        emit actionDone(QStringLiteral("led"), false, micErr());
        return;
    }
    uint32_t v = enable ? 1u : 0u;               /* miclib.h: uint32_t * */
    int r = mic_set_led_alert(d.mdh, &v);
    emit actionDone(QStringLiteral("led"), r == E_MIC_SUCCESS,
                    r == E_MIC_SUCCESS ? QString() : micErr());
}

void DeviceWorker::maintEnter()
{
    if (m_cur < 0 || m_cur >= m_devs.size())
        return;
    auto &d = m_devs[m_cur];
    if (!ensureOpen(d)) {
        emit actionDone(QStringLiteral("maint"), false, micErr());
        return;
    }
    int r = mic_enter_maint_mode(d.mdh);
    emit actionDone(QStringLiteral("maint"), r == E_MIC_SUCCESS,
                    r == E_MIC_SUCCESS ? QString() : micErr());
}

void DeviceWorker::maintLeave()
{
    if (m_cur < 0 || m_cur >= m_devs.size())
        return;
    auto &d = m_devs[m_cur];
    if (!ensureOpen(d)) {
        emit actionDone(QStringLiteral("maint"), false, micErr());
        return;
    }
    int r = mic_leave_maint_mode(d.mdh);
    emit actionDone(QStringLiteral("maint"), r == E_MIC_SUCCESS,
                    r == E_MIC_SUCCESS ? QString() : micErr());
}
