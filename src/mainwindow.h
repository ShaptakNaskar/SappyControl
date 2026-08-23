#pragma once

#include "commandbuilder.h"

#include <QMainWindow>
#include <QProcess>

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QWidget *buildPowerCard();
    QWidget *buildNetworkCard();
    QWidget *buildToolsCard();
    QWidget *buildActivityCard();
    QWidget *createCard(const QString &eyebrow, const QString &title, const QString &description);
    QSpinBox *createSpinBox(int minimum, int maximum, int value, const QString &suffix);
    QPushButton *createPrimaryButton(const QString &text);
    QPushButton *createSecondaryButton(const QString &text);

    void applyPowerMode(const PowerSettings &settings, const QString &name);
    void applyCustomPowerMode();
    void runCommand(const CommandSpec &command);
    void runWifi(bool disable);
    void launchSpeedcheck();
    void launchHetznerTest();
    void launchInTerminal(const QString &title, const QString &program, const QStringList &arguments);
    void appendOutput(const QString &text, const QString &kind = {});
    void setBusy(bool busy, const QString &label = {});
    void refreshAvailability();
    [[nodiscard]] QString findExecutable(const QString &name, const QString &fallback = {}) const;
    [[nodiscard]] QString findWifiHelper() const;
    [[nodiscard]] QString findHetznerHelper() const;

    QProcess *m_process{};
    QList<QPushButton *> m_actionButtons;
    QLabel *m_statusDot{};
    QLabel *m_statusText{};
    QLabel *m_commandStatus{};
    QLabel *m_wifiAvailability{};
    QLabel *m_speedcheckAvailability{};
    QLabel *m_hetznerAvailability{};
    QPlainTextEdit *m_output{};
    QPushButton *m_stopButton{};

    QSpinBox *m_performanceTemp{};
    QSpinBox *m_performanceFan{};
    QSpinBox *m_idleTemp{};
    QSpinBox *m_customStapm{};
    QSpinBox *m_customFast{};
    QSpinBox *m_customSlow{};
    QSpinBox *m_customTemp{};
    QSpinBox *m_customFan{};
    QCheckBox *m_customFanAuto{};
    QCheckBox *m_wifiPersistent{};
    QCheckBox *m_speedOnly{};
    QCheckBox *m_noLog{};
};
