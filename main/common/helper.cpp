/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include <cstdio>
#include <cstdarg>
#include <stdio.h>
#include <string.h>
#include "helper.h"

// Substitute for ESP_LOGx
void _print(const char* str1, const char* format, ...) {
    std::printf("%s: ", str1);
    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);
    std::printf("\n");
}

int cInt(const char s[]) {
    int num = 0;
    int n = strlen(s);

    for (int i = 0; i < n; i++)
        num = num * 10 + (s[i] - 48);

    return num;
}

int cppInt(const std::string& s)
{
    int num = 0;
    int n = s.length();

    for (int i = 0; i < n; i++)
        num = num * 10 + (int(s[i]) - 48);
    return num;
}

char* cStr(int value, char * str)
{
    sprintf(str, "%d", value);
    return str;
}

std::string cppStr(int value)
{
    return std::to_string(value);
}


