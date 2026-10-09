/* main.cpp — micsmc-qt 入口 */
#include <QApplication>
#include <QFile>

#include "MainWindow.h"
#include "Snapshot.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("micsmc-qt"));
    app.setOrganizationName(QStringLiteral("mpss3-LoongArch-port"));

    /* 跨线程 queued signal 需要（Qt6 中带 Q_DECLARE_METATYPE 的自定义类型） */
    qRegisterMetaType<Snapshot>();

    /* 主题：默认原版亮色（original.qss，按 micsmc-gui 内嵌样式复刻）；
       环境变量 MICSMC_QT_THEME=dark 可切到暗色主题 */
    QString theme = QString::fromUtf8(qgetenv("MICSMC_QT_THEME"));
    if (theme != QStringLiteral("dark"))
        theme = QStringLiteral("original");
    QFile qss(QStringLiteral(":/res/") + theme + QStringLiteral(".qss"));
    if (!qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qss.setFileName(QStringLiteral(":/res/original.qss"));
        if (!qss.open(QIODevice::ReadOnly | QIODevice::Text))
            qss.close();
    }
    if (qss.isOpen())
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow w;
    w.show();
    return app.exec();
}
