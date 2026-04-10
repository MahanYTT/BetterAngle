#include "shared/Profile.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>
#include <sstream>
#include <locale>

bool Profile::Load(const std::wstring& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    // Simple manual JSON parser to avoid external dependencies
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    
    // Extract name
    size_t namePos = content.find("\"name\": \"");
    if (namePos != std::string::npos) {
        size_t end = content.find("\"", namePos + 9);
        std::string n = content.substr(namePos + 9, end - (namePos + 9));
        name = std::wstring(n.begin(), n.end());
    }

    // Extract scales
    auto extractDouble = [&](const std::string& key) -> double {
        size_t pos = content.find("\"" + key + "\": ");
        if (pos == std::string::npos) return 0.003;
        size_t end = content.find_first_of(",}", pos + 3 + key.length());
        std::string valStr = content.substr(pos + 3 + key.length(), end - (pos + 3 + key.length()));
        for (char& c : valStr) if (c == ',') c = '.'; // Fix existing comma-polluted files
        std::istringstream iss(valStr);
        iss.imbue(std::locale("C"));
        double d = 0.003;
        iss >> d;
        return d;
    };

    scale_normal = extractDouble("scale_normal");
    scale_diving = extractDouble("scale_diving");
    roi_x = (int)extractDouble("roi_x");
    roi_y = (int)extractDouble("roi_y");
    roi_w = (int)extractDouble("roi_w");
    roi_h = (int)extractDouble("roi_h");
    target_color = (COLORREF)extractDouble("target_color");
    tolerance = (int)extractDouble("tolerance");
    
    return true;
}

bool Profile::Save(const std::wstring& path) {
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open()) return false;

    std::string n;
    for (wchar_t c : name) n += (char)c;
    // Ensure we write with a safe dot decimal regardless of locale
    std::ostringstream oss;
    oss.imbue(std::locale("C"));
    oss << "{\n";
    oss << "  \"name\": \"" << n << "\",\n";
    oss << "  \"scale_normal\": " << scale_normal << ",\n";
    oss << "  \"scale_diving\": " << scale_diving << ",\n";
    oss << "  \"roi_x\": " << roi_x << ",\n";
    oss << "  \"roi_y\": " << roi_y << ",\n";
    oss << "  \"roi_w\": " << roi_w << ",\n";
    oss << "  \"roi_h\": " << roi_h << ",\n";
    oss << "  \"target_color\": " << target_color << ",\n";
    oss << "  \"tolerance\": " << tolerance << "\n";
    oss << "}";
    
    f << oss.str();

    return true;
}

std::vector<Profile> GetProfiles(const std::wstring& directory) {
    std::vector<Profile> profiles;
    WIN32_FIND_DATAW findData;
    std::wstring searchPath = directory + L"/*.json";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                Profile p;
                if (p.Load(directory + L"/" + findData.cFileName)) {
                    profiles.push_back(p);
                }
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    return profiles;
}
