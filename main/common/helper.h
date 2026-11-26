/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#pragma once

#include <string>

void _print(const char*, const char*, ...);
int cInt(char[]);
int cppInt(const std::string& str);
char* cStr(int value, char *);
std::string cppStr(int value);