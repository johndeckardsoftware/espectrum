#pragma once

#include <stdio.h>
#include "sdmmc_cmd.h"

void esp_sdcard_port_init(void);
uint64_t esp_sdcard_port_get_size(void);
esp_err_t storage_init_sdmmc(sdmmc_card_t **card);