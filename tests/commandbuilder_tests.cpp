#include "commandbuilder.h"

#include <QtTest/QTest>

class CommandBuilderTests final : public QObject {
    Q_OBJECT

private slots:
    void acceptsExpectedPowerProfiles();
    void rejectsUnsafePowerValues();
    void buildsPowerCommandWithPositionalArguments();
    void buildsWifiVariants();
};

void CommandBuilderTests::acceptsExpectedPowerProfiles()
{
    QVERIFY(!CommandBuilder::validatePowerSettings({55'000, 75'000, 65'000, 93, 99}));
    QVERIFY(!CommandBuilder::validatePowerSettings({45'000, 45'000, 45'000, 50, std::nullopt}));
}

void CommandBuilderTests::rejectsUnsafePowerValues()
{
    QVERIFY(CommandBuilder::validatePowerSettings({4'999, 75'000, 65'000, 93, 99}));
    QVERIFY(CommandBuilder::validatePowerSettings({55'000, 75'000, 65'000, 101, 99}));
    QVERIFY(CommandBuilder::validatePowerSettings({55'000, 75'000, 65'000, 93, 101}));
}

void CommandBuilderTests::buildsPowerCommandWithPositionalArguments()
{
    const auto command = CommandBuilder::powerMode({55'000, 75'000, 65'000, 93, 99}, QStringLiteral("Performance"));
    QCOMPARE(command.program, QStringLiteral("/usr/bin/pkexec"));
    QCOMPARE(command.arguments.at(0), QStringLiteral("/usr/bin/sh"));
    QCOMPARE(command.arguments.at(4), QStringLiteral("55000"));
    QCOMPARE(command.arguments.at(7), QStringLiteral("93"));
    QCOMPARE(command.arguments.at(8), QStringLiteral("99"));
    QVERIFY(!CommandBuilder::shellPreview(command).contains(QStringLiteral("&&")));

    const auto automaticFan = CommandBuilder::powerMode(
        {45'000, 45'000, 45'000, 50, std::nullopt}, QStringLiteral("Automatic fan"));
    QCOMPARE(automaticFan.arguments.at(8), QStringLiteral("auto"));
}

void CommandBuilderTests::buildsWifiVariants()
{
    const auto persistent = CommandBuilder::wifi(QStringLiteral("/tmp/wifi-kill"), true, true);
    QCOMPARE(persistent.arguments, QStringList({QStringLiteral("--kill"), QStringLiteral("--persist")}));

    const auto restore = CommandBuilder::wifi(QStringLiteral("/tmp/wifi-kill"), false, true);
    QCOMPARE(restore.arguments, QStringList({QStringLiteral("--off")}));
}

QTEST_MAIN(CommandBuilderTests)
#include "commandbuilder_tests.moc"
