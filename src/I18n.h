/* I18n.h — 轻量双语表（micsmc-qt）
 *
 * 不走 Qt Linguist（.ts/.qm 需要构建期工具链）：头文件级 key→{zh,en} 表，
 * 运行时可切换（MainWindow 里的语言按钮），选择存 QSettings。
 * 用法：QString s = L("key");   // 模板串可继续 .arg(...)
 */
#ifndef MICSMC_QT_I18N_H
#define MICSMC_QT_I18N_H

#include <QString>
#include <QStringList>
#include <cstring>

namespace I18n {

struct Entry {
    const char *key;
    const char *zh;
    const char *en;
};

/* 当前语言存储（"zh" / "en"） */
inline QString &langStorage()
{
    static QString s = QStringLiteral("zh");
    return s;
}

inline void setLang(const QString &l) { langStorage() = l; }
inline bool isEn() { return langStorage() == QStringLiteral("en"); }

inline const Entry ENTRIES[] = {
    /* 窗口与分区 */
    { "app_title",      "micsmc-qt — Xeon Phi 控制面板", "micsmc-qt — Xeon Phi Control Panel" },
    { "grp_devinfo",    "设备信息",   "Device" },
    { "grp_power",      "功耗 (W)",   "Power (W)" },
    { "grp_temp",       "核心温度 (°C)", "Core Temp (°C)" },
    { "grp_util",       "利用率 (%)",  "Utilization (%)" },
    { "grp_mem",        "内存 (MB)",   "Memory (MB)" },
    { "grp_sensors",    "温度传感器",  "Temperature Sensors" },
    { "grp_coregrid",   "每核利用率 (%)", "Per-Core Utilization (%)" },
    { "grp_control",    "控制",        "Control" },
    /* 身份栏字段 */
    { "f_device",       "设备",       "Device" },
    { "f_status",       "状态",       "Status" },
    { "f_cores",        "核心",       "Cores" },
    { "f_freq",         "频率",       "Freq" },
    { "f_ecc",          "ECC",        "ECC" },
    { "f_uos",          "uOS",        "uOS" },
    { "f_flash",        "Flash",      "Flash" },
    { "f_driver",       "主机驱动",   "Host Driver" },
    /* 温度表 */
    { "th_sensor",      "传感器",     "Sensor" },
    { "th_value",       "温度",       "Temp" },
    { "th_status",      "状态",       "Status" },
    { "s_die",          "核心 (die)",  "Core (die)" },
    { "s_gddr",         "显存 (GDDR)", "Memory (GDDR)" },
    { "s_fanin",        "进风",       "Fan In" },
    { "s_fanout",       "出风",       "Fan Out" },
    { "s_vccp",         "核心轨 (VccP)", "Core Rail (VccP)" },
    { "s_vddg",         "北桥轨 (VddG)", "Uncore Rail (VddG)" },
    { "s_vddq",         "显存轨 (VddQ)", "Mem Rail (VddQ)" },
    { "sensor_ok",      "正常",       "OK" },
    { "sensor_invalid", "传感器无效", "sensor invalid" },
    /* 状态值 */
    { "v_on",           "开",         "On" },
    { "v_off",          "关",         "Off" },
    { "v_offline",      "不在线",     "offline" },
    { "v_online",       "online",     "online" },
    { "v_active",       "活跃",       "active" },
    { "v_sampling",     "采样中（等待第二个周期）", "Sampling (needs a second cycle)" },
    /* 明细行 */
    { "d_power",        "均值 %1 W · 峰值 %2 W · 限额 %3–%4 W · 物理上限 %5 W",
                        "avg %1 W · peak %2 W · limit %3–%4 W · phys max %5 W" },
    { "d_mem",          "已用 %1 / %2 MB · 空闲 %3 MB",
                        "used %1 / %2 MB · free %3 MB" },
    /* 按钮 */
    { "btn_turbo",      "Turbo: %1%2",  "Turbo: %1%2" },
    { "btn_led",        "LED: %1",      "LED: %1" },
    { "btn_maint",      "维护模式: %1", "Maintenance: %1" },
    { "btn_lang",       "EN",           "中文" },
    { "btn_devsel",     "设备:",        "Device:" },
    /* 动作结果 */
    { "act_done",       "✓ %1 已执行",  "✓ %1 done" },
    { "act_failed",     "✗ %1 失败: %2", "✗ %1 failed: %2" },
    { "act_offline",    "设备不在线",   "Device offline" },
    { "a_turbo",        "Turbo",       "Turbo" },
    { "a_led",          "LED",         "LED" },
    { "a_maint",        "维护模式",    "maintenance mode" },
    /* 确认框 */
    { "cf_title",       "确认",        "Confirm" },
    { "cf_turbo_on",    "确定开启 Turbo？",  "Enable Turbo?" },
    { "cf_turbo_off",   "确定关闭 Turbo？",  "Disable Turbo?" },
    { "cf_led_on",      "确定开启 LED 告警灯？", "Enable LED alert?" },
    { "cf_led_off",     "确定关闭 LED 告警灯？", "Disable LED alert?" },
    { "cf_maint_enter", "进入维护模式？设备将立即离线，正在运行的任务会被打断！",
                        "Enter maintenance mode? The device goes offline immediately; running jobs will be interrupted!" },
    { "cf_maint_leave", "退出维护模式（设备将重新上线）？",
                        "Leave maintenance mode? The device will come back online." },
    /* 其它 */
    { "coregrid_wait",  "等待利用率数据（需两个采集周期）",
                        "Waiting for utilization (needs two cycles)" },
    { "core_tip",       "核心 %1: %2%", "Core %1: %2%" },
    { "pie_used",       "已用 %1 MB",  "Used %1 MB" },
    { "pie_free",       "空闲 %1 MB",  "Free %1 MB" },
    { "pie_usage",      "使用率 %1%",  "Usage %1%" },
    { "dev_offline_sfx", "%1 (离线)",  "%1 (offline)" },
    { "fatal_nodev",    "枚举不到 Xeon Phi 设备（检查 lsmod | grep mic、ls /dev/mic*、micctrl --status）",
                        "No Xeon Phi devices found (check lsmod | grep mic, ls /dev/mic*, micctrl --status)" },
    /* 菜单 / 汇总字段条 / 计数字段 */
    { "menu_card",      "卡(&C)",     "&Cards" },
    { "menu_advanced",  "高级(&A)",   "&Advanced" },
    { "menu_exit",      "退出",       "Exit" },
    { "menu_settings",  "高级设置…",  "Advanced Settings…" },
    { "menu_about",     "关于…",      "About…" },
    { "menu_show_all",  "全部显示(&S)", "&Show All" },
    { "menu_hide_all",  "全部隐藏(&H)", "&Hide All" },
    { "menu_error_log", "错误日志(&E)", "&Error Log" },
    { "menu_update_app", "更新应用(&U)", "&Update Application" },
    { "menu_info",      "信息(&I)",   "&Info" },
    { "about_text",     "micsmc-qt — micsmc-gui 的 LoongArch/Qt6 复刻\n数据层: libmicmgmt (MPSS 3.8.6)",
                        "micsmc-qt — LoongArch/Qt6 replica of micsmc-gui\nData layer: libmicmgmt (MPSS 3.8.6)" },
    /* 工具栏下拉按钮 "Cards ▾" / "Advanced ▾" */
    { "tool_cards",     "Cards ▾",    "Cards ▾" },
    { "tool_advanced",  "Advanced ▾", "Advanced ▾" },
    /* NavBar 计数字段 */
    { "nav_cards",      "Cards:  %1", "Cards:  %1" },
    { "nav_memory",     "Memory:  %1 GB", "Memory:  %1 GB" },
    { "nav_cores",      "Cores:  %1", "Cores:  %1" },
    { "tip_prev_view",  "上一个视图", "Go to previous view" },
    { "tip_next_view",  "下一个视图", "Go to next view" },
    { "nav_title",      "%1: %2",     "%1: %2" },
    { "nav_none",       "无设备",     "no device" },
    { "nav_view_util",  "利用率视图", "Utilization View" },
    { "nav_view_histo", "核直方视图", "Core Histogram View" },
    { "nav_view_hist",  "历史利用率视图", "Historical Utilization View" },
    /* 总览面板（上带）文本 */
    { "sys_title",      "利用率视图（所有设备）", "Utilization View (All Devices)" },
    { "sys_temp1",      "平均核心",   "Average Core" },
    { "sys_temp2",      "温度 %1C",  "Temperature %1C" },
    { "sys_mem1",       "总量",       "Total" },
    { "sys_mem2",       "内存用量",   "Memory Usage" },
    { "sys_mem_val",    "总内存: %1 MB", "Total Memory: %1" },
    { "sys_pow1",       "总量",       "Total" },
    { "sys_pow2",       "功耗用量",   "Power Usage" },
    { "sys_pow_val",    "最大功率: %1 W", "Maximum Power is: %1 Watts" },
    { "sys_temp_warn",  "核心温度高于警告级", "Core Temperature above warning level" },
    { "sys_temp_crit",  "核心温度高于严重级", "Core Temperature above critical level" },
    /* 卡视图（下带）字段文本 */
    { "cv_temp1",       "处理器核心", "Processor Core" },
    { "cv_temp2",       "温度 %1C",  "Temperature %1C" },
    { "cv_temp2b",      "温度  °C",  "Temperature  °C" },
    /* 图例 "System [%]" / "User [%]" */
    { "leg_sys",        "System [%]", "System [%]" },
    { "leg_user",       "User [%]",   "User [%]" },
    /* X 轴标题 */
    { "axis_avg_core_util", "平均核心利用率", "Average Core Utilization" },
    { "axis_core_util",     "核心利用率",     "Core Utilization" },
    /* 字段值单位 */
    { "unit_gb",        "%1 GB",    "%1 GB" },
    { "unit_watts",     "%1 W",     "%1 Watts" },
    /* 详情面板右上角视图切换按钮（原版图标 :/utilization.png / :/histo.png） */
    { "vbtn_util",      "▤", "▤" },
    { "vbtn_histo",     "▥", "▥" },
    { "tip_go_util",    "Go to Utilization View", "Go to Utilization View" },
    { "tip_go_histo",   "Go to Core Histogram View", "Go to Core Histogram View" },
    /* CardView 四象限 */
    { "cv_util",        "核心利用率",   "Core Utilization" },
    { "cv_temp",        "处理器核心温度", "Processor Core Temperature" },
    { "cv_mem",         "内存用量",     "Memory Usage" },
    { "cv_power",       "功耗",         "Power Usage" },
    { "cv_temp_ok",     "正常",         "normal" },
    { "cv_temp_warn",   "警告：接近温度限值", "WARNING: near temperature limit" },
    { "cv_temp_crit",   "严重：超过温度限值", "CRITICAL: temperature limit exceeded" },
    { "cv_power_text",  "瞬时 %1 W · 均值 %2 W", "inst %1 W · avg %2 W" },
    { "cv_power_ok",    "限值 %1–%2 W", "limits %1–%2 W" },
    { "cv_power_warn",  "警告：超过低功耗限值", "WARNING: above low power limit" },
    { "cv_power_crit",  "严重：超过高功耗限值", "CRITICAL: above high power limit" },
    /* Advanced Settings 对话框 */
    { "st_title",       "高级设置",     "Advanced Settings" },
    { "st_card_settings", "卡设置",     "Card Settings" },
    { "st_connections", "卡连接",       "Card Connections" },
    { "st_connected",   "已连接",       "connected" },
    { "st_logfile",     "日志文件",     "Log File" },
    { "st_logfile_na",  "日志轮转由 mpssd 管理（未接入本复刻）",
                        "Log rotation is managed by mpssd (not wired in this replica)" },
    { "st_ecc_na",      "需要维护模式全流程（未实现）",
                        "Requires the maintenance-mode flow (not implemented)" },
};

inline QString s(const char *key)
{
    const bool en = isEn();
    for (const Entry &e : ENTRIES) {
        if (!strcmp(e.key, key))
            return QString::fromUtf8(en ? e.en : e.zh);
    }
    return QString::fromUtf8(key);
}

} // namespace I18n

#define L(key) I18n::s(key)

#endif /* MICSMC_QT_I18N_H */
