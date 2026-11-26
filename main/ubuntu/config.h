/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

/**
 * @brief NVS Storage area helper 
 */
class Config
{
private:
public:
    /**
     * @brief Return true if tape if configured for flash load.
     */
    bool isTapeLoadModeFlash(void);

    /**
     * @brief Set key=value in nvs.
     */
    bool set(std::string key, std::string value);
    std::string get(std::string key);
};
#endif // CONFIG_H
