/* SettingsDialog.h — Advanced Settings 对话框（micsmc-qt）
 *
 * 三个分组：
 *   Card Settings 组：Turbo/ECC/LED × 每卡 的复选框网格
 *   Card Connections 组：每卡状态显示（重连由 micctrl 负责）
 *   Log File 组：占位（日志轮转由 mpssd 配置，本程序未接入）
 * ECC 复选框禁用：需要维护模式全流程（enter_maint → set_ecc_mode_start →
 * 轮询 progress → done → 复位），当前未实现，故该复选框置灰。
 */
#ifndef MICSMC_QT_SETTINGSDIALOG_H
#define MICSMC_QT_SETTINGSDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QVector>

class QCheckBox;

struct CardSettingIntent {
    int devIndex = 0;
    bool turbo = false;
    bool led = false;
};

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    /* names: 设备名列表；current: 每卡当前 {turbo, led} 状态 */
    SettingsDialog(const QStringList &names,
                   const QVector<QPair<bool, bool>> &current,
                   QWidget *parent = nullptr);

    QVector<CardSettingIntent> intents() const { return m_intents; }

private slots:
    void accept() override;

private:
    QStringList m_names;
    QVector<QPair<bool, bool>> m_current;
    QVector<QCheckBox *> m_turboBoxes;
    QVector<QCheckBox *> m_ledBoxes;
    QVector<CardSettingIntent> m_intents;
};

#endif /* MICSMC_QT_SETTINGSDIALOG_H */
