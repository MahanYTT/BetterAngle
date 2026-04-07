#include "shared/Config.h"
#include <fstream>
#include <iostream>

AppConfig g_config;

void AppConfig::Load() {
    std::ifstream f("config_pro.bin", std::ios::binary);
    if (f) {
        f.read((char*)this, sizeof(AppConfig));
        f.close();
    }
}

void AppConfig::Save() {
    std::ofstream f("config_pro.bin", std::ios::binary);
    if (f) {
        f.write((char*)this, sizeof(AppConfig));
        f.close();
    }
}
