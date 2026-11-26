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
     * @brief delete all keys in "storage"
     */
    static void deleteAll(void);

    /**
     * @brief Return true if tape if configured for flash load.
     */
    static bool isTapeLoadModeFlash(void);
    
    /**
     * @brief Return true if tape if configured for flash load.
     */
    static int32_t getScreenshotProg(void);

    /**
     * @brief Set key=value in nvs.
     */
    static bool set(std::string key, std::string value);

    /**
     * @brief Get string value for key in nvs.
     */
    static std::string get(std::string key);

    /**
     * @brief Get int32_t value for key in nvs.
     */
    static void get(const char *key, int32_t * out);
    static void get(std::string key, int32_t * out);
    static void set(std::string key, int32_t value);

    /**
     * @brief Get int8_t value for key in nvs.
     */
    static void get(const char *key, int8_t * out);

    /**
     * @brief Set int8_t value for key in nvs.
     */
    static void set(const char *key, int8_t value);

};
#endif // CONFIG_H
