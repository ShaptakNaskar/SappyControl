#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

struct CommandSpec {
    QString program;
    QStringList arguments;
    QString displayName;
};

struct PowerSettings {
    int stapmLimitMilliwatts{};
    int fastLimitMilliwatts{};
    int slowLimitMilliwatts{};
    int temperatureCelsius{};
    std::optional<int> fanPercent;
};

class CommandBuilder final {
public:
    [[nodiscard]] static std::optional<QString> validatePowerSettings(const PowerSettings &settings);
    [[nodiscard]] static CommandSpec powerMode(const PowerSettings &settings, const QString &displayName);
    [[nodiscard]] static CommandSpec wifi(const QString &executable, bool disable, bool persistent);
    [[nodiscard]] static QString shellPreview(const CommandSpec &command);

private:
    [[nodiscard]] static QString quoteForPreview(const QString &argument);
};
