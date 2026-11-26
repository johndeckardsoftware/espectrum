 /*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#ifndef ULIST_H
#define ULIST_H

#include <stdint.h>
#include <vector>
#include <string>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"

#define ULIST_POS_X 6
#define ULIST_POS_Y 6

/**
 * @brief Represents a menu item with optional NVS key-value storage info.
 */
struct ListItem
{
std::string text;
std::string nvsKey;
    std::string nvsValue;
    bool hasNvs = false;

    ListItem(const char* t) : text(t), hasNvs(false) {}
    ListItem(const std::string& t) : text(t), hasNvs(false) {}
    ListItem(const char* t, const char* k, const char* v) : text(t), nvsKey(k), nvsValue(v), hasNvs(true) {}
    ListItem(const std::string& t, const std::string& k, const std::string& v) : text(t), nvsKey(k), nvsValue(v), hasNvs(true) {}
    ListItem(const std::string& t, const std::string& k, bool b) : text(t), nvsKey(k), hasNvs(b) {}
};

/**
 * @brief Micro list handler with pagination support
 */
class uList
{
private:
    lgfx::LGFX_Device &lcd;
    LGFX_Sprite sprList;
    std::vector<ListItem> _items;
    Config config;
    int16_t selectedItem;
    int16_t currentPage;
    const int itemsPerPage = 6; // Number of items displayed per page

public:
    /**
     * @brief Constructs a new paged uList instance.
     */
    uList(lgfx::LGFX_Device &display, std::vector<ListItem> &items, Config &config);

    /**
     * @brief Draws the item list page and pagination bar, pushing it to the display.
     */
    void drawItemList();

    /**
     * @brief Checks touch coordinates against items and pagination buttons.
     * @return Index of the selected item, -2 for page navigation, or -1 on miss.
     */
    int16_t checkKeyHit(int16_t relX, int16_t relY);

    /**
     * @brief Gets the currently selected item index.
     */
    inline int16_t getSelectedItem() const { return selectedItem; }

    /**
     * @brief Returns the list item at the specified index safely.
     * @param index Vector index of the item.
     * @return const ListItem& Reference to the item structure.
     */
    const ListItem& getItem(size_t index) const;
    inline const LGFX_Sprite* getSprite() const { return &sprList; }
};

#endif // ULIST_H
