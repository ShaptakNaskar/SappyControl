#include "mainwindow.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr auto kStyleSheet = R"(
QMainWindow, QWidget#root {
    background: #0b0d12;
    color: #f4f6fb;
}
QScrollArea, QScrollArea > QWidget > QWidget {
    background: transparent;
    border: none;
}
QWidget#topBar {
    background: #10131a;
    border-bottom: 1px solid #242936;
}
QLabel#appMark {
    background: #f5b942;
    color: #17120a;
    border-radius: 12px;
    font-size: 18px;
    font-weight: 800;
}
QLabel#appTitle {
    color: #ffffff;
    font-size: 20px;
    font-weight: 750;
}
QLabel#appSubtitle, QLabel#muted, QLabel#cardDescription, QLabel#fieldHint {
    color: #929aaa;
}
QLabel#pageTitle {
    color: #ffffff;
    font-size: 30px;
    font-weight: 760;
}
QLabel#pageIntro {
    color: #aab1bf;
    font-size: 12px;
}
QLabel#eyebrow {
    color: #f5b942;
    font-size: 10px;
    font-weight: 800;
}
QLabel#cardTitle {
    color: #ffffff;
    font-size: 18px;
    font-weight: 720;
}
QLabel#sectionTitle {
    color: #f4f6fb;
    font-size: 13px;
    font-weight: 700;
}
QFrame#card {
    background: #131720;
    border: 1px solid #262c39;
    border-radius: 16px;
}
QFrame#divider {
    background: #282e3a;
    min-height: 1px;
    max-height: 1px;
    border: none;
}
QFrame#presetRow, QFrame#toolRow {
    background: #171c26;
    border: 1px solid #292f3c;
    border-radius: 11px;
}
QPushButton {
    min-height: 38px;
    padding: 0 16px;
    border-radius: 9px;
    font-weight: 700;
}
QPushButton#primary {
    color: #17120a;
    background: #f5b942;
    border: 1px solid #f5b942;
}
QPushButton#primary:hover { background: #ffd06b; border-color: #ffd06b; }
QPushButton#primary:pressed { background: #dca52f; }
QPushButton#secondary {
    color: #e8ebf2;
    background: #222936;
    border: 1px solid #353d4c;
}
QPushButton#secondary:hover { background: #2d3544; border-color: #4a5466; }
QPushButton#danger {
    color: #ffd9d6;
    background: #462525;
    border: 1px solid #713737;
}
QPushButton#danger:hover { background: #5b2d2d; }
QPushButton:disabled { color: #687080; background: #1b1f28; border-color: #282d37; }
QSpinBox {
    min-height: 36px;
    min-width: 88px;
    padding: 0 8px;
    color: #f5f7fb;
    background: #0d1016;
    border: 1px solid #323947;
    border-radius: 8px;
    selection-background-color: #f5b942;
    selection-color: #17120a;
}
QSpinBox:focus { border: 1px solid #f5b942; }
QSpinBox:disabled { color: #666e7d; background: #12151c; border-color: #242a35; }
QSpinBox::up-button, QSpinBox::down-button { width: 18px; border: none; }
QCheckBox { color: #cbd0da; spacing: 8px; }
QCheckBox::indicator {
    width: 17px;
    height: 17px;
    background: #0d1016;
    border: 1px solid #41495a;
    border-radius: 4px;
}
QCheckBox::indicator:checked { background: #f5b942; border-color: #f5b942; }
QLabel#badgeGood, QLabel#badgeWarn, QLabel#badgeNeutral {
    padding: 4px 9px;
    border-radius: 8px;
    font-size: 10px;
    font-weight: 750;
}
QLabel#badgeGood { color: #a7efc1; background: #173527; border: 1px solid #28523e; }
QLabel#badgeWarn { color: #ffd6a0; background: #3b2c18; border: 1px solid #604729; }
QLabel#badgeNeutral { color: #b9c1d0; background: #202632; border: 1px solid #323a49; }
QPlainTextEdit {
    color: #c9d1df;
    background: #090b0f;
    border: 1px solid #282e39;
    border-radius: 10px;
    padding: 10px;
    selection-background-color: #72571e;
}
QToolTip {
    color: #ffffff;
    background: #242936;
    border: 1px solid #424b5c;
    padding: 5px;
}
)";

QFrame *divider()
{
    auto *line = new QFrame;
    line->setObjectName(QStringLiteral("divider"));
    return line;
}

QLabel *availabilityBadge(const QString &text = QStringLiteral("Checking"))
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("badgeNeutral"));
    label->setAlignment(Qt::AlignCenter);
    return label;
}

void updateBadge(QLabel *badge, const bool available, const QString &availableText = QStringLiteral("Ready"))
{
    badge->setText(available ? availableText : QStringLiteral("Not found"));
    badge->setObjectName(available ? QStringLiteral("badgeGood") : QStringLiteral("badgeWarn"));
    badge->style()->unpolish(badge);
    badge->style()->polish(badge);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_process(new QProcess(this))
{
    setWindowTitle(QStringLiteral("Sappy's Controls"));
    setMinimumSize(980, 760);
    resize(1120, 860);
    setStyleSheet(QString::fromLatin1(kStyleSheet));

    auto *root = new QWidget;
    root->setObjectName(QStringLiteral("root"));
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *topBar = new QWidget;
    topBar->setObjectName(QStringLiteral("topBar"));
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(28, 15, 28, 15);
    topLayout->setSpacing(13);

    auto *mark = new QLabel(QStringLiteral("SC"));
    mark->setObjectName(QStringLiteral("appMark"));
    mark->setAlignment(Qt::AlignCenter);
    mark->setFixedSize(44, 44);
    topLayout->addWidget(mark);

    auto *identity = new QVBoxLayout;
    identity->setSpacing(1);
    auto *appTitle = new QLabel(QStringLiteral("Sappy's Controls"));
    appTitle->setObjectName(QStringLiteral("appTitle"));
    auto *appSubtitle = new QLabel(QStringLiteral("Your daily system shortcuts"));
    appSubtitle->setObjectName(QStringLiteral("appSubtitle"));
    identity->addWidget(appTitle);
    identity->addWidget(appSubtitle);
    topLayout->addLayout(identity);
    topLayout->addStretch();

    m_statusDot = new QLabel(QStringLiteral("●"));
    m_statusDot->setStyleSheet(QStringLiteral("color: #54d18b; font-size: 15px;"));
    m_statusText = new QLabel(QStringLiteral("Ready"));
    m_statusText->setObjectName(QStringLiteral("muted"));
    topLayout->addWidget(m_statusDot);
    topLayout->addWidget(m_statusText);
    rootLayout->addWidget(topBar);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 30, 32, 34);
    contentLayout->setSpacing(20);

    auto *pageTitle = new QLabel(QStringLiteral("Control center"));
    pageTitle->setObjectName(QStringLiteral("pageTitle"));
    auto *pageIntro = new QLabel(QStringLiteral("Tune performance, manage Wi-Fi, and launch your network tools from one place."));
    pageIntro->setObjectName(QStringLiteral("pageIntro"));
    pageIntro->setWordWrap(true);
    contentLayout->addWidget(pageTitle);
    contentLayout->addWidget(pageIntro);

    auto *cards = new QGridLayout;
    cards->setHorizontalSpacing(18);
    cards->setVerticalSpacing(18);
    cards->setColumnStretch(0, 1);
    cards->setColumnStretch(1, 1);
    cards->addWidget(buildPowerCard(), 0, 0, 1, 2);
    cards->addWidget(buildNetworkCard(), 1, 0);
    cards->addWidget(buildToolsCard(), 1, 1);
    cards->addWidget(buildActivityCard(), 2, 0, 1, 2);
    contentLayout->addLayout(cards);
    contentLayout->addStretch();

    scrollArea->setWidget(content);
    rootLayout->addWidget(scrollArea);
    setCentralWidget(root);

    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        appendOutput(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        appendOutput(QString::fromLocal8Bit(m_process->readAllStandardError()), QStringLiteral("error"));
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](const QProcess::ProcessError error) {
        if (error != QProcess::Crashed) {
            appendOutput(QStringLiteral("Could not start command: %1\n").arg(m_process->errorString()), QStringLiteral("error"));
            setBusy(false);
        }
    });
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](const int exitCode, const QProcess::ExitStatus exitStatus) {
            const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
            appendOutput(success
                    ? QStringLiteral("✓ Finished successfully.\n")
                    : QStringLiteral("✕ Finished with exit code %1.\n").arg(exitCode),
                success ? QStringLiteral("success") : QStringLiteral("error"));
            setBusy(false);
        });

    refreshAvailability();
}

QWidget *MainWindow::createCard(const QString &eyebrow, const QString &title, const QString &description)
{
    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 20, 22, 22);
    layout->setSpacing(8);

    auto *eyebrowLabel = new QLabel(eyebrow.toUpper());
    eyebrowLabel->setObjectName(QStringLiteral("eyebrow"));
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("cardTitle"));
    auto *descriptionLabel = new QLabel(description);
    descriptionLabel->setObjectName(QStringLiteral("cardDescription"));
    descriptionLabel->setWordWrap(true);

    layout->addWidget(eyebrowLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addSpacing(8);
    return card;
}

QSpinBox *MainWindow::createSpinBox(const int minimum, const int maximum, const int value, const QString &suffix)
{
    auto *spinBox = new QSpinBox;
    spinBox->setRange(minimum, maximum);
    spinBox->setValue(value);
    spinBox->setSuffix(suffix);
    spinBox->setAlignment(Qt::AlignRight);
    return spinBox;
}

QPushButton *MainWindow::createPrimaryButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setObjectName(QStringLiteral("primary"));
    button->setCursor(Qt::PointingHandCursor);
    m_actionButtons.append(button);
    return button;
}

QPushButton *MainWindow::createSecondaryButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setObjectName(QStringLiteral("secondary"));
    button->setCursor(Qt::PointingHandCursor);
    m_actionButtons.append(button);
    return button;
}

QWidget *MainWindow::buildPowerCard()
{
    auto *card = createCard(
        QStringLiteral("Power & thermals"),
        QStringLiteral("Performance profiles"),
        QStringLiteral("Apply your regular Ryzen power limits, temperature ceiling, NVIDIA service, and NBFC fan mode."));
    auto *layout = qobject_cast<QVBoxLayout *>(card->layout());

    auto makePreset = [this](const QString &title, const QString &description, QSpinBox *&temperature,
                          QSpinBox *&fan, const bool automaticFan) {
        auto *row = new QFrame;
        row->setObjectName(QStringLiteral("presetRow"));
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(14, 12, 14, 12);
        rowLayout->setSpacing(10);

        auto *textLayout = new QVBoxLayout;
        textLayout->setSpacing(2);
        auto *name = new QLabel(title);
        name->setObjectName(QStringLiteral("sectionTitle"));
        auto *detail = new QLabel(description);
        detail->setObjectName(QStringLiteral("fieldHint"));
        detail->setWordWrap(true);
        textLayout->addWidget(name);
        textLayout->addWidget(detail);
        rowLayout->addLayout(textLayout, 1);

        temperature = createSpinBox(40, 100, title.contains(QStringLiteral("High")) ? 93 : 50, QStringLiteral(" °C"));
        temperature->setToolTip(QStringLiteral("Temperature ceiling"));
        rowLayout->addWidget(temperature);

        if (!automaticFan) {
            fan = createSpinBox(0, 100, 99, QStringLiteral(" %"));
            fan->setToolTip(QStringLiteral("Fixed NBFC fan speed"));
            rowLayout->addWidget(fan);
        }

        auto *button = createPrimaryButton(title.contains(QStringLiteral("High"))
                ? QStringLiteral("Activate")
                : QStringLiteral("Force idle"));
        rowLayout->addWidget(button);
        return qMakePair(row, button);
    };

    auto high = makePreset(
        QStringLiteral("High performance"),
        QStringLiteral("55 / 75 / 65 W · fixed fan"),
        m_performanceTemp,
        m_performanceFan,
        false);
    layout->addWidget(high.first);
    connect(high.second, &QPushButton::clicked, this, [this] {
        applyPowerMode({55'000, 75'000, 65'000, m_performanceTemp->value(), m_performanceFan->value()},
            QStringLiteral("High performance mode"));
    });

    QSpinBox *unusedFan = nullptr;
    auto idle = makePreset(
        QStringLiteral("Force idle"),
        QStringLiteral("45 / 45 / 45 W · automatic fan"),
        m_idleTemp,
        unusedFan,
        true);
    layout->addWidget(idle.first);
    connect(idle.second, &QPushButton::clicked, this, [this] {
        applyPowerMode({45'000, 45'000, 45'000, m_idleTemp->value(), std::nullopt},
            QStringLiteral("Force idle mode"));
    });

    layout->addWidget(divider());
    auto *customTitle = new QLabel(QStringLiteral("CUSTOM TUNING"));
    customTitle->setObjectName(QStringLiteral("eyebrow"));
    layout->addWidget(customTitle);

    auto *customGrid = new QGridLayout;
    customGrid->setHorizontalSpacing(12);
    customGrid->setVerticalSpacing(7);
    const QStringList labels{
        QStringLiteral("Sustained"), QStringLiteral("Fast"), QStringLiteral("Slow"),
        QStringLiteral("Temperature"), QStringLiteral("Fan speed")};
    for (int column = 0; column < labels.size(); ++column) {
        auto *label = new QLabel(labels.at(column));
        label->setObjectName(QStringLiteral("fieldHint"));
        customGrid->addWidget(label, 0, column);
    }

    m_customStapm = createSpinBox(5, 100, 55, QStringLiteral(" W"));
    m_customFast = createSpinBox(5, 100, 75, QStringLiteral(" W"));
    m_customSlow = createSpinBox(5, 100, 65, QStringLiteral(" W"));
    m_customTemp = createSpinBox(40, 100, 93, QStringLiteral(" °C"));
    m_customFan = createSpinBox(0, 100, 99, QStringLiteral(" %"));
    const QList<QSpinBox *> inputs{m_customStapm, m_customFast, m_customSlow, m_customTemp, m_customFan};
    for (int column = 0; column < inputs.size(); ++column) {
        inputs.at(column)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        customGrid->addWidget(inputs.at(column), 1, column);
    }
    m_customFanAuto = new QCheckBox(QStringLiteral("Automatic fan control"));
    m_customFanAuto->setToolTip(QStringLiteral("Use NBFC automatic fan mode instead of a fixed percentage."));
    customGrid->addWidget(m_customFanAuto, 2, 4, Qt::AlignLeft);
    layout->addLayout(customGrid);

    connect(m_customFanAuto, &QCheckBox::toggled, m_customFan, &QSpinBox::setDisabled);

    auto *customActions = new QHBoxLayout;
    auto *hint = new QLabel(QStringLiteral("Values are validated before elevation."));
    hint->setObjectName(QStringLiteral("fieldHint"));
    customActions->addWidget(hint);
    customActions->addStretch();
    auto *apply = createSecondaryButton(QStringLiteral("Apply custom tuning"));
    customActions->addWidget(apply);
    layout->addLayout(customActions);
    connect(apply, &QPushButton::clicked, this, &MainWindow::applyCustomPowerMode);

    return card;
}

QWidget *MainWindow::buildNetworkCard()
{
    auto *card = createCard(
        QStringLiteral("Network device"),
        QStringLiteral("Wi-Fi control"),
        QStringLiteral("Unload or restore the configured Wi-Fi driver using the bundled helper."));
    auto *layout = qobject_cast<QVBoxLayout *>(card->layout());

    auto *availabilityRow = new QHBoxLayout;
    auto *helperLabel = new QLabel(QStringLiteral("Wi-Fi helper"));
    helperLabel->setObjectName(QStringLiteral("sectionTitle"));
    m_wifiAvailability = availabilityBadge();
    availabilityRow->addWidget(helperLabel);
    availabilityRow->addStretch();
    availabilityRow->addWidget(m_wifiAvailability);
    layout->addLayout(availabilityRow);

    m_wifiPersistent = new QCheckBox(QStringLiteral("Keep Wi-Fi disabled after reboot"));
    m_wifiPersistent->setToolTip(QStringLiteral("Writes a modprobe blacklist and rebuilds initramfs."));
    layout->addWidget(m_wifiPersistent);

    auto *warning = new QLabel(QStringLiteral("Persistent mode changes modprobe and udev configuration, then rebuilds initramfs."));
    warning->setObjectName(QStringLiteral("fieldHint"));
    warning->setWordWrap(true);
    layout->addWidget(warning);

    auto *actions = new QHBoxLayout;
    auto *disable = new QPushButton(QStringLiteral("Disable Wi-Fi"));
    disable->setObjectName(QStringLiteral("danger"));
    disable->setCursor(Qt::PointingHandCursor);
    m_actionButtons.append(disable);
    auto *restore = createSecondaryButton(QStringLiteral("Restore Wi-Fi"));
    actions->addWidget(disable);
    actions->addWidget(restore);
    layout->addLayout(actions);
    layout->addStretch();

    connect(disable, &QPushButton::clicked, this, [this] { runWifi(true); });
    connect(restore, &QPushButton::clicked, this, [this] { runWifi(false); });
    return card;
}

QWidget *MainWindow::buildToolsCard()
{
    auto *card = createCard(
        QStringLiteral("Diagnostics"),
        QStringLiteral("Network tools"),
        QStringLiteral("Open interactive and long-running checks in a dedicated terminal window."));
    auto *layout = qobject_cast<QVBoxLayout *>(card->layout());

    auto *speedRow = new QFrame;
    speedRow->setObjectName(QStringLiteral("toolRow"));
    auto *speedLayout = new QHBoxLayout(speedRow);
    speedLayout->setContentsMargins(13, 11, 13, 11);
    auto *speedText = new QVBoxLayout;
    speedText->setSpacing(2);
    auto *speedTitle = new QLabel(QStringLiteral("Speedcheck"));
    speedTitle->setObjectName(QStringLiteral("sectionTitle"));
    auto *speedDescription = new QLabel(QStringLiteral("Interactive connection stability test"));
    speedDescription->setObjectName(QStringLiteral("fieldHint"));
    speedText->addWidget(speedTitle);
    speedText->addWidget(speedDescription);
    speedLayout->addLayout(speedText, 1);
    m_speedcheckAvailability = availabilityBadge();
    speedLayout->addWidget(m_speedcheckAvailability);
    auto *runSpeedcheck = createPrimaryButton(QStringLiteral("Open"));
    speedLayout->addWidget(runSpeedcheck);
    layout->addWidget(speedRow);
    connect(runSpeedcheck, &QPushButton::clicked, this, &MainWindow::launchSpeedcheck);

    auto *hetznerRow = new QFrame;
    hetznerRow->setObjectName(QStringLiteral("toolRow"));
    auto *hetznerLayout = new QVBoxLayout(hetznerRow);
    hetznerLayout->setContentsMargins(13, 11, 13, 11);
    hetznerLayout->setSpacing(8);
    auto *hetznerHeader = new QHBoxLayout;
    auto *hetznerTitle = new QLabel(QStringLiteral("Hetzner nettest"));
    hetznerTitle->setObjectName(QStringLiteral("sectionTitle"));
    m_hetznerAvailability = availabilityBadge();
    hetznerHeader->addWidget(hetznerTitle);
    hetznerHeader->addStretch();
    hetznerHeader->addWidget(m_hetznerAvailability);
    hetznerLayout->addLayout(hetznerHeader);

    m_speedOnly = new QCheckBox(QStringLiteral("Speed only"));
    m_noLog = new QCheckBox(QStringLiteral("Don't write logs"));
    auto *options = new QHBoxLayout;
    options->addWidget(m_speedOnly);
    options->addWidget(m_noLog);
    options->addStretch();
    hetznerLayout->addLayout(options);
    auto *runHetzner = createSecondaryButton(QStringLiteral("Run in terminal"));
    hetznerLayout->addWidget(runHetzner, 0, Qt::AlignRight);
    layout->addWidget(hetznerRow);
    connect(runHetzner, &QPushButton::clicked, this, &MainWindow::launchHetznerTest);

    layout->addStretch();
    return card;
}

QWidget *MainWindow::buildActivityCard()
{
    auto *card = createCard(
        QStringLiteral("Live activity"),
        QStringLiteral("Command output"),
        QStringLiteral("Output from power and Wi-Fi actions appears here. Terminal tools keep their own interactive output."));
    auto *layout = qobject_cast<QVBoxLayout *>(card->layout());

    auto *toolbar = new QHBoxLayout;
    m_commandStatus = new QLabel(QStringLiteral("No command running"));
    m_commandStatus->setObjectName(QStringLiteral("fieldHint"));
    toolbar->addWidget(m_commandStatus);
    toolbar->addStretch();

    m_stopButton = new QPushButton(QStringLiteral("Stop"));
    m_stopButton->setObjectName(QStringLiteral("danger"));
    m_stopButton->setEnabled(false);
    m_stopButton->setCursor(Qt::PointingHandCursor);
    toolbar->addWidget(m_stopButton);
    auto *clear = createSecondaryButton(QStringLiteral("Clear"));
    toolbar->addWidget(clear);
    layout->addLayout(toolbar);

    m_output = new QPlainTextEdit;
    m_output->setReadOnly(true);
    m_output->setMinimumHeight(180);
    m_output->setMaximumBlockCount(2'000);
    m_output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_output->setPlaceholderText(QStringLiteral("Commands and their output will appear here…"));
    layout->addWidget(m_output);

    connect(clear, &QPushButton::clicked, m_output, &QPlainTextEdit::clear);
    connect(m_stopButton, &QPushButton::clicked, this, [this] {
        appendOutput(QStringLiteral("Stopping command…\n"), QStringLiteral("error"));
        m_process->terminate();
        QTimer::singleShot(2'000, m_process, [this] {
            if (m_process->state() != QProcess::NotRunning) {
                m_process->kill();
            }
        });
    });
    return card;
}

void MainWindow::applyPowerMode(const PowerSettings &settings, const QString &name)
{
    if (const auto error = CommandBuilder::validatePowerSettings(settings)) {
        QMessageBox::warning(this, QStringLiteral("Invalid tuning"), *error);
        return;
    }
    runCommand(CommandBuilder::powerMode(settings, name));
}

void MainWindow::applyCustomPowerMode()
{
    const std::optional<int> fan = m_customFanAuto->isChecked()
        ? std::nullopt
        : std::optional<int>{m_customFan->value()};
    applyPowerMode({
                       m_customStapm->value() * 1'000,
                       m_customFast->value() * 1'000,
                       m_customSlow->value() * 1'000,
                       m_customTemp->value(),
                       fan,
                   },
        QStringLiteral("Custom power tuning"));
}

void MainWindow::runCommand(const CommandSpec &command)
{
    if (m_process->state() != QProcess::NotRunning) {
        QMessageBox::information(this, QStringLiteral("Command already running"),
            QStringLiteral("Wait for the current command to finish or stop it first."));
        return;
    }

    appendOutput(QStringLiteral("\n[%1] %2\n$ %3\n")
                     .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")),
                         command.displayName,
                         CommandBuilder::shellPreview(command)));
    setBusy(true, command.displayName);
    m_process->start(command.program, command.arguments);
}

void MainWindow::runWifi(const bool disable)
{
    const QString executable = findWifiHelper();
    if (executable.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Wi-Fi helper not found"),
            QStringLiteral("Reinstall Sappy's Controls or install wifi-kill somewhere in PATH."));
        return;
    }

    const bool persistent = disable && m_wifiPersistent->isChecked();
    if (persistent) {
        const auto choice = QMessageBox::warning(this,
            QStringLiteral("Disable Wi-Fi after reboot?"),
            QStringLiteral("This will write persistent system configuration and rebuild initramfs. Continue?"),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (choice != QMessageBox::Yes) {
            return;
        }
    }

    runCommand(CommandBuilder::wifi(executable, disable, persistent));
}

void MainWindow::launchSpeedcheck()
{
    const QString executable = findExecutable(QStringLiteral("speedcheck"), QStringLiteral("/usr/local/bin/speedcheck"));
    if (executable.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("speedcheck not found"),
            QStringLiteral("Install speedcheck somewhere in PATH."));
        return;
    }
    launchInTerminal(QStringLiteral("Speedcheck"), executable, {});
}

void MainWindow::launchHetznerTest()
{
    const QString script = findHetznerHelper();
    if (script.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Script not found"),
            QStringLiteral("Reinstall Sappy's Controls or place hetzner-nettest.sh in your home directory."));
        return;
    }

    QStringList arguments{script};
    if (m_speedOnly->isChecked()) {
        arguments.append(QStringLiteral("--speedonly"));
    }
    if (m_noLog->isChecked()) {
        arguments.append(QStringLiteral("--nolog"));
    }
    launchInTerminal(QStringLiteral("Hetzner nettest"), QStringLiteral("/usr/bin/bash"), arguments);
}

void MainWindow::launchInTerminal(const QString &title, const QString &program, const QStringList &arguments)
{
    struct Terminal {
        QString name;
        QStringList prefix;
    };
    const QList<Terminal> terminals{
        {QStringLiteral("konsole"), {QStringLiteral("--new-tab"), QStringLiteral("-p"), QStringLiteral("tabtitle=%1").arg(title), QStringLiteral("-e")}},
        {QStringLiteral("kitty"), {QStringLiteral("--title"), title}},
        {QStringLiteral("alacritty"), {QStringLiteral("--title"), title, QStringLiteral("-e")}},
        {QStringLiteral("gnome-terminal"), {QStringLiteral("--title"), title, QStringLiteral("--")}},
        {QStringLiteral("xfce4-terminal"), {QStringLiteral("--title"), title, QStringLiteral("-x")}},
        {QStringLiteral("xterm"), {QStringLiteral("-T"), title, QStringLiteral("-e")}},
    };

    for (const auto &terminal : terminals) {
        const QString terminalExecutable = QStandardPaths::findExecutable(terminal.name);
        if (terminalExecutable.isEmpty()) {
            continue;
        }

        QStringList terminalArguments = terminal.prefix;
        terminalArguments.append(program);
        terminalArguments.append(arguments);
        if (QProcess::startDetached(terminalExecutable, terminalArguments)) {
            appendOutput(QStringLiteral("\n[%1] Opened %2 in %3.\n")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")),
                                 title,
                                 terminal.name),
                QStringLiteral("success"));
            return;
        }
    }

    QMessageBox::warning(this, QStringLiteral("No terminal found"),
        QStringLiteral("Install Konsole, Kitty, Alacritty, GNOME Terminal, Xfce Terminal, or xterm."));
}

void MainWindow::appendOutput(const QString &text, const QString &kind)
{
    if (!m_output || text.isEmpty()) {
        return;
    }

    QString color = QStringLiteral("#c9d1df");
    if (kind == QStringLiteral("error")) {
        color = QStringLiteral("#ff9c95");
    } else if (kind == QStringLiteral("success")) {
        color = QStringLiteral("#83dda6");
    }

    QTextCharFormat format;
    format.setForeground(QColor(color));
    auto cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);
    m_output->setTextCursor(cursor);
    m_output->ensureCursorVisible();
}

void MainWindow::setBusy(const bool busy, const QString &label)
{
    for (auto *button : m_actionButtons) {
        if (button != m_stopButton) {
            button->setEnabled(!busy);
        }
    }
    m_stopButton->setEnabled(busy);
    m_commandStatus->setText(busy ? QStringLiteral("Running: %1").arg(label) : QStringLiteral("No command running"));
    m_statusText->setText(busy ? QStringLiteral("Working") : QStringLiteral("Ready"));
    m_statusDot->setStyleSheet(busy
            ? QStringLiteral("color: #f5b942; font-size: 15px;")
            : QStringLiteral("color: #54d18b; font-size: 15px;"));
}

void MainWindow::refreshAvailability()
{
    const QString wifi = findWifiHelper();
    const QString speedcheck = findExecutable(QStringLiteral("speedcheck"), QStringLiteral("/usr/local/bin/speedcheck"));
    const QString hetzner = findHetznerHelper();
    updateBadge(m_wifiAvailability, !wifi.isEmpty());
    updateBadge(m_speedcheckAvailability, !speedcheck.isEmpty());
    updateBadge(m_hetznerAvailability, !hetzner.isEmpty(), QStringLiteral("Script ready"));
}

QString MainWindow::findExecutable(const QString &name, const QString &fallback) const
{
    const QString inPath = QStandardPaths::findExecutable(name);
    if (!inPath.isEmpty()) {
        return inPath;
    }
    const QFileInfo fallbackInfo(fallback);
    return fallbackInfo.exists() && fallbackInfo.isExecutable() ? fallbackInfo.absoluteFilePath() : QString{};
}

QString MainWindow::findWifiHelper() const
{
    const QString bundled = QStandardPaths::findExecutable(QStringLiteral("sappy-wifi-kill"));
    if (!bundled.isEmpty()) {
        return bundled;
    }
    return findExecutable(QStringLiteral("wifi-kill"),
        QDir::home().filePath(QStringLiteral(".local/bin/wifi-kill")));
}

QString MainWindow::findHetznerHelper() const
{
    return findExecutable(QStringLiteral("sappy-hetzner-nettest"),
        QDir::home().filePath(QStringLiteral("hetzner-nettest.sh")));
}
