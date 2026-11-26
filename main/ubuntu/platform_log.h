/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#pragma once
#include <cstdio>

#define ESP_LOGI(tag, format, ...) std::fprintf(stdout, "[%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) std::fprintf(stderr, "[%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) std::fprintf(stderr, "[%s] " format "\n", tag, ##__VA_ARGS__)
#endif
