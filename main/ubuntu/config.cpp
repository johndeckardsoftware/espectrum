/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include <string>
#include "config.h"

#include <unordered_map>
#include <utility>

namespace
{
std::unordered_map<std::string, std::string> settings;
}

bool Config::isTapeLoadModeFlash(void)
{
    return get("tape") == "fast";
}

bool Config::set(std::string key, std::string value)
{
    settings[std::move(key)] = std::move(value);
    return true;
}

std::string Config::get(std::string key)
{
    const auto it = settings.find(key);
    return it == settings.end() ? std::string{} : it->second;
}