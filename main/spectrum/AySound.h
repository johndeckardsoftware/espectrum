/*
ESPectrum, a Sinclair ZX Spectrum emulator for Espressif ESP32 SoC

AY SOUND EMULATION, based on libayemu by:
Sashnov Alexander <sashnov@ngs.ru> and Roman Scherbakov <v_soft@nm.ru>

Copyright (c) 2023, 2024 Víctor Iborra [Eremus] and 2023 David Crespo [dcrespo3d]
https://github.com/EremusOne/ZX-ESPectrum-IDF

Based on ZX-ESPectrum-Wiimote
Copyright (c) 2020, 2022 David Crespo [dcrespo3d]
https://github.com/dcrespo3d/ZX-ESPectrum-Wiimote

Based on previous work by Ramón Martinez and Jorge Fuertes
https://github.com/rampa069/ZX-ESPectrum

Original project by Pete Todd
https://github.com/retrogubbins/paseVGA

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

To Contact the dev team you can write to zxespectrum@gmail.com or
visit https://zxespectrum.speccy.org/contacto

*/

#ifndef AySound_h
#define AySound_h

#include <inttypes.h>
#include <stddef.h>

#define SAMPLES_PER_FRAME 312

typedef enum
{
    AYEMU_MONO = 0, AYEMU_ABC, AYEMU_ACB, AYEMU_BAC, AYEMU_BCA, AYEMU_CAB, AYEMU_CBA, AYEMU_STEREO_CUSTOM = 255
} ayemu_stereo_t;

typedef enum
{
    AYEMU_AY, AYEMU_YM, AYEMU_AY_LION17, AYEMU_YM_LION17, AYEMU_AY_KAY, AYEMU_YM_KAY, AYEMU_AY_LOG, AYEMU_YM_LOG, AYEMU_AY_CUSTOM, AYEMU_YM_CUSTOM
} ayemu_chip_t;

/** Selectable AY volume table implementations */
typedef enum
{
    AY_TABLE_DEFAULT_LOG = 0, AY_TABLE_LION17, AY_TABLE_RAMPA_2023, AY_TABLE_SOFTSPEC48, AY_TABLE_CUSTOM
} ay_table_mode_t;

/** Selectable Envelope table implementations */
typedef enum
{
    AY_ENV_STATIC = 0, AY_ENV_GENERATED
} ay_env_mode_t;

/** Selectable Noise generator implementations */
typedef enum
{
    AY_NOISE_HACKER_KAY = 0, AY_NOISE_SOFTSPEC48, AY_NOISE_JSPECCY
} ay_noise_mode_t;

typedef struct {
    int tone_a, tone_b, tone_c, noise;
    int R7_tone_a, R7_tone_b, R7_tone_c, R7_noise_a, R7_noise_b, R7_noise_c;
    int vol_a, vol_b, vol_c, env_a, env_b, env_c;
    int env_freq, env_style, IOPortA, IOPortB;
} ayemu_regdata_t;

typedef struct {
    int freq, channels, bpc;
} ayemu_sndfmt_t;

class AySound
{
public:
    static void updToneA(); static void updToneB(); static void updToneC();
    static void updNoisePitch(); static void updMixer();
    static void updVolA(); static void updVolB(); static void updVolC();
    static void updEnvFreq(); static void updEnvType();
    static void updIOPortA(); static void updIOPortB();

    static void reset();
    static uint8_t getRegisterData();
    static void selectRegister(uint8_t data);
    static void setRegisterData(uint8_t data);

    static void init();
    static int set_chip_type(ayemu_chip_t chip, int *custom_table);
    static void set_chip_freq(int chipfreq);
    static int set_stereo(ayemu_stereo_t stereo, int *custom_eq);
    static int set_sound_format(int freq, int chans, int bits);
    static void prepare_generation();
    static void gen_sound(int bufsize, int bufpos);

    // Runtime configuration setters & getters for selectable tables/algorithms
    static void set_table_mode(ay_table_mode_t mode, int *custom_table = NULL);
    static ay_table_mode_t get_table_mode();

    static void set_env_mode(ay_env_mode_t mode);
    static ay_env_mode_t get_env_mode();

    static void set_noise_mode(ay_noise_mode_t mode);
    static ay_noise_mode_t get_noise_mode();
    
    void configure(ay_table_mode_t t_mode, ay_env_mode_t e_mode, ay_noise_mode_t n_mode);
    
    static void (*updateReg[16])();
    static uint8_t SamplebufAY[SAMPLES_PER_FRAME];

private:
    static void reset_noise_seed();
    static int table[32];
    static ayemu_chip_t type;
    static int ChipFreq;
    static ay_table_mode_t table_mode;
    static ay_env_mode_t env_mode;
    static ay_noise_mode_t noise_mode;
    static ayemu_regdata_t ayregs;
    static ayemu_sndfmt_t sndfmt;
    static int default_chip_flag, default_stereo_flag, default_sound_format_flag, dirty;
    static int bit_a, bit_b, bit_c, bit_n, period_n;
    static int cnt_a, cnt_b, cnt_c, cnt_n, cnt_e;
    static int ChipTacts_per_outcount, Amp_Global;
    static int EnvNum, env_pos, Cur_Seed;
    static uint8_t regs[16];
    static uint8_t selectedRegister;
};

#endif // AySound_h
