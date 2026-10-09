/* SettingsDialog.cpp — 原版 Advanced Settings 对话框复刻 */
#include "SettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "I18n.h"

SettingsDialog::SettingsDialog(const QStringList &names,
                               const QVector<QPair<bool, bool>> &current,
                               QWidget *parent)
    : QDialog(parent), m_names(names), m_current(current)
{
    setWindowTitle(L("st_title"));
    setModal(true);

    auto *root = new QVBoxLayout(this);

    /* Card Settings：复选框网格（行=特征，列=卡） */
    auto *gs = new QGroupBox(L("st_card_settings"), this);
    auto *grid = new QGridLayout(gs);
    grid->setSpacing(8);

    for (int c = 0; c < m_names.size(); c++) {
        auto *h = new QLabel(m_names[c], gs);
        grid->addWidget(h, 0, c + 1, Qt::AlignCenter);
    }
    auto *lTurbo = new QLabel(L("a_turbo"), gs);
    grid->addWidget(lTurbo, 1, 0);
    auto *lEcc = new QLabel(QStringLiteral("ECC"), gs);
    grid->addWidget(lEcc, 2, 0);
    auto *lLed = new QLabel(L("a_led"), gs);
    grid->addWidget(lLed, 3, 0);

    for (int c = 0; c < m_names.size(); c++) {
        auto *tb = new QCheckBox(gs);
        tb->setChecked(c < m_current.size() ? m_current[c].first : false);
        grid->addWidget(tb, 1, c + 1, Qt::AlignCenter);
        m_turboBoxes.append(tb);

        auto *eb = new QCheckBox(gs);
        eb->setEnabled(false);   /* 维护模式全流程未实现 */
        eb->setToolTip(L("st_ecc_na"));
        grid->addWidget(eb, 2, c + 1, Qt::AlignCenter);

        auto *lb = new QCheckBox(gs);
        lb->setChecked(c < m_current.size() ? m_current[c].second : false);
        grid->addWidget(lb, 3, c + 1, Qt::AlignCenter);
        m_ledBoxes.append(lb);
    }
    root->addWidget(gs);

    /* Card Connections：状态显示（重连在原版里重启卡，此处只读） */
    auto *gc = new QGroupBox(L("st_connections"), this);
    auto *cl = new QVBoxLayout(gc);
    for (int c = 0; c < m_names.size(); c++) {
        auto *l = new QLabel(
            QStringLiteral("%1 — %2").arg(m_names[c])
                .arg(c < m_current.size() ? L("st_connected")
                                          : L("v_offline")), gc);
        cl->addWidget(l);
    }
    if (m_names.isEmpty())
        cl->addWidget(new QLabel(L("no_dev"), gc));
    root->addWidget(gc);

    /* Log File：占位说明 */
    auto *gl = new QGroupBox(L("st_logfile"), this);
    auto *ll = new QVBoxLayout(gl);
    ll->addWidget(new QLabel(L("st_logfile_na"), gl));
    root->addWidget(gl);

    auto *bb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bb);

    /* 默认 intents 为空（取消时不应用） */
}

void SettingsDialog::accept()
{
    m_intents.clear();
    for (int c = 0; c < m_names.size(); c++) {
        CardSettingIntent it;
        it.devIndex = c;
        bool curTurbo = c < m_current.size() ? m_current[c].first : false;
        bool curLed = c < m_current.size() ? m_current[c].second : false;
        /* 只有勾选状态变化的才生成意图（原版：Turbo/LED 即时生效不重启） */
        if (m_turboBoxes.value(c) &&
            m_turboBoxes[c]->isChecked() != curTurbo) {
            it.turbo = true;
        }
        if (m_ledBoxes.value(c) && m_ledBoxes[c]->isChecked() != curLed) {
            it.led = true;
        }
        /* 无变化也保留条目（带原状态），便于上层统一处理 */
        m_intents.append(it);
    }
    QDialog::accept();
}
