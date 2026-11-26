/**
 * @brief Supported ZX Spectrum hardware models.
 */
#define ZX48K 48
#define ZX128K 128

// Override ZX_MODEL for non-ESP-IDF builds, or select a model in menuconfig.
#ifndef ZX_MODEL
#if defined(CONFIG_SPECTRUM_MEMORY_MODEL_128K) && CONFIG_SPECTRUM_MEMORY_MODEL_128K
#define ZX_MODEL ZX128K
#else
#define ZX_MODEL ZX48K
#endif
#endif

#if ZX_MODEL != ZX48K && ZX_MODEL != ZX128K
#error "ZX_MODEL must be ZX48K or ZX128K"
#endif
