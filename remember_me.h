#pragma once
#include <string>

namespace RememberMe
{
    bool Load(std::string& outDiscordId, std::string& outLicenseKey);
    bool Save(const std::string& discordId, const std::string& licenseKey);
    bool Clear();
}