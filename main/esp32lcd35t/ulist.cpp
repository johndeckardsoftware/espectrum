/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include <vector>
#include <string>
#include <cstdio>
#include "ulist.h"
#include "esp_lcd_touch.h"

extern esp_lcd_touch_handle_t touch_handle;

#define LIST_WIDTH  400
#define LIST_HEIGHT 312
#define LIST_X      4
#define LIST_Y      4
#define ROW_H       37

uList::uList(lgfx::LGFX_Device &display, std::vector<ListItem>& items, Config &_config)
    : lcd(display),
      sprList(&lcd),
      _items(items),
      config(_config),
      selectedItem(-1),
      currentPage(0)
{
    sprList.setColorDepth(16);
    sprList.setPsram(true);
    sprList.createSprite(LIST_WIDTH, LIST_HEIGHT);

    // Automatically highlight the item whose NVS value matches the stored setting
    for (size_t i = 0; i < _items.size(); ++i) {
        if (_items[i].hasNvs) {
            if (_items[i].nvsValue == config.get(_items[i].nvsKey)) {
                selectedItem = static_cast<int16_t>(i);
                currentPage = i / itemsPerPage;
            }
        }
    }
}

void uList::drawItemList()
{
    sprList.fillSprite(TFT_BLACK);
    sprList.setTextSize(2);

    int totalPages = (_items.size() + itemsPerPage - 1) / itemsPerPage;
    if (totalPages == 0) totalPages = 1;
    if (currentPage >= totalPages) currentPage = totalPages - 1;
    if (currentPage < 0) currentPage = 0;

    int startIndex = currentPage * itemsPerPage;
    int endIndex = startIndex + itemsPerPage;
    if (endIndex > (int)_items.size()) {
        endIndex = _items.size();
    }

    // 1. Draw visible items for the current page
    for (int i = startIndex; i < endIndex; ++i)
    {
        int rowIdx = i - startIndex;
        const int rowY = LIST_Y + rowIdx * ROW_H;
        const uint16_t background = (i == selectedItem) ? TFT_BLUE : TFT_DARKGREY;

        sprList.fillRect(LIST_X, rowY, sprList.width() - 2 * LIST_X, ROW_H - 2, background);
        sprList.drawRect(LIST_X, rowY, sprList.width() - 2 * LIST_X, ROW_H - 2, TFT_LIGHTGREY);

        sprList.setTextColor(TFT_WHITE, background);
        sprList.setCursor(LIST_X + 12, rowY + 10);
        sprList.print(_items.at(i).text.c_str());
    }

    // 2. Draw Bottom Navigation Bar (y = 270 to 312)
    int navY = 270;

    // Back Button
    sprList.fillRect(4, navY, 86, 36, TFT_DARKGREY);
    sprList.drawRect(4, navY, 86, 36, TFT_LIGHTGREY);
    sprList.setTextColor(TFT_WHITE, TFT_DARKGREY);
    sprList.setCursor(20, navY + 10);
    sprList.print("< Back");

    // Prev Page Button
    bool canPrev = (currentPage > 0);
    sprList.fillRect(95, navY, 80, 36, canPrev ? TFT_DARKGREY : TFT_BLACK);
    sprList.drawRect(95, navY, 80, 36, TFT_LIGHTGREY);
    sprList.setTextColor(canPrev ? TFT_WHITE : TFT_DARKGREY, canPrev ? TFT_DARKGREY : TFT_BLACK);
    sprList.setCursor(110, navY + 10);
    sprList.print("Prev");

    // Page Indicator Text
    char pageStr[32];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", currentPage + 1, totalPages);
    sprList.setTextColor(TFT_WHITE, TFT_BLACK);
    sprList.setCursor(190, navY + 10);
    sprList.print(pageStr);

    // Next Page Button
    bool canNext = (currentPage < totalPages - 1);
    sprList.fillRect(265, navY, 80, 36, canNext ? TFT_DARKGREY : TFT_BLACK);
    sprList.drawRect(265, navY, 80, 36, TFT_LIGHTGREY);
    sprList.setTextColor(canNext ? TFT_WHITE : TFT_DARKGREY, canNext ? TFT_DARKGREY : TFT_BLACK);
    sprList.setCursor(285, navY + 10);
    sprList.print("Next");

    // Dummy filler or space on right edge if needed, or expand buttons
    sprList.pushSprite(ULIST_POS_X, ULIST_POS_Y);
}

int16_t uList::checkKeyHit(int16_t touchX, int16_t touchY)
{
    if (touchX >= 432 && touchY >= 296) {
        return -3; // screenshot
    }

    int totalPages = (_items.size() + itemsPerPage - 1) / itemsPerPage;

    // Check bottom navigation bar (y between 270 and 312)
    if (touchY >= 270 && touchY < 312) {
        if (touchX >= 4 && touchX <= 90) {
            // Back button hit -> returns -1 to indicate exit/back
            return -1;
        }
        else if (touchX >= 95 && touchX <= 175) {
            // Prev button hit
            if (currentPage > 0) {
                currentPage--;
                drawItemList();
            }
            return -2; // Page change code
        }
        else if (touchX >= 265 && touchX <= 345) {
            // Next button hit
            if (currentPage < totalPages - 1) {
                currentPage++;
                drawItemList();
            }
            return -2; // Page change code
        }
        return -1; // Back
    }

    // Check item list row clicks
    const int listWidth = sprList.width() - 2 * LIST_X;
    if (touchX < LIST_X || touchX >= LIST_X + listWidth || touchY < LIST_Y)
        return -1;

    const int rowIdx = (touchY - LIST_Y) / ROW_H;
    if (rowIdx < 0 || rowIdx >= itemsPerPage)
        return -1;

    size_t absoluteIndex = currentPage * itemsPerPage + rowIdx;
    if (absoluteIndex >= _items.size())
        return -1;

    selectedItem = static_cast<int16_t>(absoluteIndex);
    drawItemList();
    return selectedItem;
}

const ListItem& uList::getItem(size_t index) const
{
    return _items.at(index);
}
