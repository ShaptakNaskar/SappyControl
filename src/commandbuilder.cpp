#include "commandbuilder.h"

#include <QStringList>

namespace {
constexpr auto kPowerScript =
    "/usr/bin/ryzenadj -a \"$1\" -b \"$2\" -c \"$3\" -f \"$4\" "
    "--apu-skin-temp=\"$4\" --dgpu-skin-temp=\"$4\" --max-performance "
    "&& /usr/bin/systemctl start nvidia-powerd "
    "&& if [ \"$5\" = auto ]; then /usr/bin/nbfc set -a; "
    "else /usr/bin/nbfc set -s \"$5\"; fi";
}

std::optional<QString> CommandBuilder::validatePowerSettings(const PowerSettings &settings)
{
    const QList<int> limits{
        settings.stapmLimitMilliwatts,
        settings.fastLimitMilliwatts,
        settings.slowLimitMilliwatts,
    };

    for (const int limit : limits) {
        if (limit < 5'000 || limit > 100'000) {
            return QStringLiteral("Power limits must be between 5 W and 100 W.");
        }
    }

    if (settings.temperatureCelsius < 40 || settings.temperatureCelsius > 100) {
        return QStringLiteral("Temperature must be between 40 °C and 100 °C.");
    }

    if (settings.fanPercent && (*settings.fanPercent < 0 || *settings.fanPercent > 100)) {
        return QStringLiteral("Fan speed must be between 0% and 100%.");
    }

    return std::nullopt;
}

CommandSpec CommandBuilder::powerMode(const PowerSettings &settings, const QString &displayName)
{
    const QString fan = settings.fanPercent
        ? QString::number(*settings.fanPercent)
        : QStringLiteral("auto");

    return {
        QStringLiteral("/usr/bin/pkexec"),
        {
            QStringLiteral("/usr/bin/sh"),
            QStringLiteral("-c"),
            QString::fromLatin1(kPowerScript),
            QStringLiteral("sappy-controls"),
            QString::number(settings.stapmLimitMilliwatts),
            QString::number(settings.fastLimitMilliwatts),
            QString::number(settings.slowLimitMilliwatts),
            QString::number(settings.temperatureCelsius),
            fan,
        },
        displayName,
    };
}

CommandSpec CommandBuilder::wifi(const QString &executable, const bool disable, const bool persistent)
{
    QStringList arguments{disable ? QStringLiteral("--kill") : QStringLiteral("--off")};
    if (disable && persistent) {
        arguments.append(QStringLiteral("--persist"));
    }

    return {
        executable,
        arguments,
        disable ? QStringLiteral("Disable Wi-Fi") : QStringLiteral("Restore Wi-Fi"),
    };
}

QString CommandBuilder::shellPreview(const CommandSpec &command)
{
    QStringList parts{quoteForPreview(command.program)};
    for (const auto &argument : command.arguments) {
        if (argument == QString::fromLatin1(kPowerScript)) {
            parts.append(QStringLiteral("<validated power sequence>"));
        } else {
            parts.append(quoteForPreview(argument));
        }
    }
    return parts.join(QLatin1Char(' '));
}

QString CommandBuilder::quoteForPreview(const QString &argument)
{
    if (!argument.contains(QLatin1Char(' ')) && !argument.contains(QLatin1Char('\''))) {
        return argument;
    }

    QString quoted = argument;
    quoted.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'") + quoted + QStringLiteral("'");
}
