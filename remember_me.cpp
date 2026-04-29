#include "remember_me.h"
#include <windows.h>
#include <string>
#include <stdlib.h> // for _dupenv_s and free

namespace RememberMe
{
    static std::string GetDirPath()
    {
        char* appdata = nullptr;
        size_t len = 0;
        if (_dupenv_s(&appdata, &len, "APPDATA") != 0 || appdata == nullptr) {
            return "";
        }
        std::string dir = std::string(appdata) + "\\Safety";
        free(appdata);
        return dir;
    }

    static std::string GetIniPath()
    {
        std::string dir = GetDirPath();
        if (dir.empty()) return "";
        return dir + "\\user.ini";
    }

    static bool EnsureDir()
    {
        std::string dir = GetDirPath();
        if (dir.empty()) return false;
        DWORD attr = GetFileAttributesA(dir.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) return true;
        return CreateDirectoryA(dir.c_str(), NULL) != 0;
    }

    bool Save(const std::string& discordId, const std::string& licenseKey)
    {
        if (discordId.empty() || licenseKey.empty())
            return Clear();

        if (!EnsureDir()) return false;

        std::string iniPath = GetIniPath();
        if (iniPath.empty()) return false;

        BOOL res1 = WritePrivateProfileStringA("RememberMe", "DiscordID", discordId.c_str(), iniPath.c_str());
        BOOL res2 = WritePrivateProfileStringA("RememberMe", "LicenseKey", licenseKey.c_str(), iniPath.c_str());

        return res1 && res2;
    }

    bool Load(std::string& outDiscordId, std::string& outLicenseKey)
    {
        outDiscordId.clear();
        outLicenseKey.clear();

        std::string iniPath = GetIniPath();
        if (iniPath.empty()) return false;

        DWORD attr = GetFileAttributesA(iniPath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) return false;

        char buf[256] = { 0 };
        DWORD len = GetPrivateProfileStringA("RememberMe", "DiscordID", "", buf, sizeof(buf), iniPath.c_str());
        if (len > 0) outDiscordId = buf;

        memset(buf, 0, sizeof(buf));
        len = GetPrivateProfileStringA("RememberMe", "LicenseKey", "", buf, sizeof(buf), iniPath.c_str());
        if (len > 0) outLicenseKey = buf;

        return !outDiscordId.empty() && !outLicenseKey.empty();
    }

    bool Clear()
    {
        std::string iniPath = GetIniPath();
        if (iniPath.empty()) return false;

        DWORD attr = GetFileAttributesA(iniPath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) return true;

        return DeleteFileA(iniPath.c_str()) != 0;
    }
}