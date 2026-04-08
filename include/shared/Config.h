#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>
#include <string>

struct Keybind {
    int vk;
    int mod;
    std::wstring name;
};

struct AppConfig {
    Keybind panel = { 'U', MOD_CONTROL, L"Toggle Panel" };
    Keybind roi = { 'R', MOD_CONTROL, L"ROI Select" };
    Keybind crosshair = { VK_F10, 0, L"Crosshair" };
    Kebind setZero = { 'G', MOD_CONTROL, L"Set Zero"};
    void Load();
    void Save();
};

extern AppConfig g_config;

#endif
