#pragma once

#include "core/ConfigManager.h"

/**
 * Applies the reusable appearance settings to the current QApplication.
 *
 * The manager captures the platform palette and font once at startup, so the
 * System theme can always return to the user's native desktop appearance.
 */
class ThemeManager final
{
public:
    static void captureApplicationDefaults();
    static void apply(const AppConfig &config);
};
