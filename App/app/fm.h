/*
 * Dondji Firmware
 *
 * Copyright (c) 2026 BD1AHN
 *
 * Licensed under the Apache License, Version 2.0
 *
 * Project:
 *     叮咚鸡 (Dondji)
 *
 * Maintainer:
 *     BD1AHN
 *
 * Official Website:
 *     https://ethanyan6.github.io/Dondji/
 *
 * The Dondji name, logo, and official project identity
 * are protected separately from the source code license.
 */


/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#ifndef APP_FM_H
#define APP_FM_H

#ifdef ENABLE_FMRADIO

#include "driver/keyboard.h"
#include "misc.h"

#define FM_CHANNEL_UP   0x01
#define FM_CHANNEL_DOWN 0xFF

enum {
    FM_SCAN_OFF = 0U,
};

extern uint16_t          gFM_Channels[FM_CHANNELS_MAX];
extern bool              gFmRadioMode;
extern uint8_t           gFmRadioCountdown_500ms;
extern volatile uint16_t gFmPlayCountdown_10ms;
extern volatile int8_t   gFM_ScanState;
extern bool              gFM_AutoScan;
extern uint8_t           gFM_ChannelPosition;

#ifdef ENABLE_FEAT_STERANIAN_DISP_RADIOSTATION_NAME
// JA: FM 局名表示（着想は steranian/uv-k1-k5v3-firmware-custom, Apache-2.0。局名は専用表で持つ形に作り直し）
// SPI 0x012000 から 4KB（1 セクタ）
//   見出し 16B: "FMST", 版数(1), 件数(0-127), 残り 0xFF
//   項目 32B × 127: 周波数 u16 LE（100kHz 単位, 例 765 = 76.5MHz）, 予備 u16, 局名 28B（UTF-8, 余りは 0x00 か 0xFF）
#define FM_STATION_TABLE_ADDR   0x012000u
#define FM_STATION_HEADER_SIZE  16u
#define FM_STATION_ENTRY_SIZE   32u
#define FM_STATION_NAME_LEN     28u
#define FM_STATION_NAME_MAX     127u
#define FM_STATION_TABLE_VER    1u
void FM_LoadStationNames(void);
bool FM_GetStationName(uint16_t Frequency, char *pName);   // pName は FM_STATION_NAME_LEN + 1 バイト
#endif
// Doubts about          whether this should be signed or not
extern uint16_t          gFM_FrequencyDeviation;
extern bool              gFM_FoundFrequency;
extern uint16_t          gFM_RestoreCountdown_10ms;

bool    FM_CheckValidChannel(uint8_t Channel);
// returns first valid channel starting at Channel
uint8_t FM_FindNextChannel(uint8_t Channel, uint8_t Direction);
uint8_t FM_FindSaveChannel(uint16_t Frequency);
int     FM_ConfigureChannelState(void);
void    FM_TurnOff(void);
void    FM_EraseChannels(void);

void    FM_Tune(uint16_t Frequency, int8_t Step, bool bFlag);
void    FM_PlayAndUpdate(void);
int     FM_CheckFrequencyLock(uint16_t Frequency, uint16_t LowerLimit);

void    FM_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

void    FM_Play(void);
void    FM_Start(void);

#endif

#endif
