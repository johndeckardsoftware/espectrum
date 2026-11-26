/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <string>
#include "config.h"

static const char *TAG = "config";

void Config::deleteAll(void) {
    nvs_handle_t handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_erase_all(handle);
    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG, "storage erase all");
};

bool Config::isTapeLoadModeFlash(void) {
    bool ret = false;
    nvs_handle_t handle;
    char buf[64] = {0};
    size_t required_size = sizeof(buf);
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_str(handle, "tape", buf, &required_size));
        if ("fast" == std::string(buf)) {
            ret = true;
        }
        nvs_close(handle);
    }
    return ret;
}

int32_t Config::getScreenshotProg(void) {
    nvs_handle_t handle;
    esp_err_t err;
    const char * key = "ssprog";
    int32_t prog = 0;
    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_i32(handle, key, &prog));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_i32(handle, key, ++prog));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(handle));
        nvs_close(handle);
    }
    return prog;
}

bool Config::set(std::string key, std::string value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(handle, key.c_str(), value.c_str()));
        nvs_commit(handle);
        nvs_close(handle);
    }
    return err;
}

std::string Config::get(std::string key) {
    nvs_handle_t handle;
    char buf[64] = {0};
    size_t required_size = sizeof(buf);
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_str(handle, key.c_str(), buf, &required_size));
        nvs_close(handle);
    }
    return std::string(buf);
}

void Config::get(const char *key, int32_t * out) {
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_i32(handle, key, out));
        nvs_close(handle);
    }
}

void Config::get(std::string key, int32_t * out) {
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_i32(handle, key.c_str(), out));
        nvs_close(handle);
    }
}

void Config::set(std::string key, int32_t value) {
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_i32(handle, key.c_str(), value));
        nvs_commit(handle);
        nvs_close(handle);
    }
}

void Config::get(const char *key, int8_t * out) {
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_i8(handle, key, out));
        nvs_close(handle);
    }
}

void Config::set(const char *key, int8_t value) {
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_i8(handle, key, value));
        nvs_commit(handle);
        nvs_close(handle);
    }
}
