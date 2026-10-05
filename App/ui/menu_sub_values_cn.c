/* UTF-8 Chinese strings for submenu values (ccc-style). */

#include "misc.h"
#include "radio.h"
#include "ui/menu.h"
#include "ui/menu_sub_values_cn.h"

#ifdef ENABLE_CHINESE

const char gSubMenu_OFF_ON_CN[][4] = {
    "OFF",     /* 关 */
    "ON",     /* 开 */
};

#ifdef ENABLE_AUDIO_BAR
const char gSubMenu_MIC_BAR_STYLE_CN[][20] = {
    "OFF",             /* 关闭 */
    "ポップ\nアップ",             /* 弹窗 */
};
#endif

const char gSubMenu_SFT_D_CN[][4] = {
    "OFF",     /* 无 */
    "+",
    "-",
};

const char gSubMenu_W_N_CN[][10] = {
    "ワイド", /* 宽带 */
    "ナロー", /* 窄带 */
};

const char *const gSubMenu_RXMode_CN[] = {
    "メイン のみ",                 /* 仅主信道 */
    "デュアル\n受信",                 /* 双频守候 */
    "クロス\nバンド",                                         /* 跨段 */
    "主送信\n2波受信",                 /* 主发双收 */
};

const char *const gSubMenu_MDF_CN[] = {
    "周波数",                                         /* 频率 */
    "CH番号",                             /* 信道号 */
    "名前",                                         /* 名称 */
    "名前+周波数",                 /* 名称+频率 */
};

#ifdef ENABLE_DTMF_CALLING
const char gSubMenu_D_RSP_CN[][11] = {
    "なし",                             /* 无动作 */
    "呼出音",                                         /* 响铃 */
    "応答",                                         /* 回复 */
    "両方",                                         /* 双端 */
};
#endif

const char *const gSubMenu_PTT_ID_CN[] = {
    "OFF",
    "開始符号",
    "終了符号",
    "開始+終了",
    "APOLLO\nQUINDAR",
};

const char gSubMenu_PONMSG_CN[][10] = {
    "OFF",
    "標準",
    "ロゴ",
};

const char *const gSubMenu_BOOT_HINT_CN[] = {
    "にこじゃ",
    "漢字万歳",
    "2026年版",
};

const char *const gSubMenu_ROGER_CN[] = {
    "OFF", /* 关 — 与英文串共享其余项 */
    gSubMenu_ROGER[1],
    gSubMenu_ROGER[2],
    gSubMenu_ROGER[3],
    gSubMenu_ROGER[4],
    gSubMenu_ROGER[5],
    gSubMenu_ROGER[6],
};

const char gSubMenu_RESET_CN[][8] = {
    "VFO",
    "全部",           /* 全部 */
    "OEM",             /* 原厂 */
};

const char *const gSubMenu_F_LOCK_CN[] = {
    "標準\n137-174\n400-470",
    "FCCアマ\n144-148\n420-450",
#ifdef ENABLE_FEAT_F4HWN_CA
    "CAアマ\n144-148\n430-450",
#endif
    "JA/CEアマ\n144-146\n430-440",
    "GBアマ\n144-148\n430-440",
    "137-174\n400-430",
    "137-174\n400-438",
#ifdef ENABLE_FEAT_F4HWN_PMR
    "PMR 446",
#endif
#ifdef ENABLE_FEAT_F4HWN_GMRS_FRS_MURS
    "GMRS\nFRS\nMURS",
#endif
    "全送信 禁止",
    "全解除",
};

const char gSubMenu_RX_TX_CN[][16] = {
    "OFF",
    "送信",
    "受信",
    "受信/送信",
};

const char gSubMenu_BAT_TXT_CN[][10] = {
    "なし",
    "電圧",
    "残量%", /* 9 UTF-8 bytes — 行宽须 ≥10 含 '\\0' */
};

const char gSubMenu_SET_NAV_CN[][17] = {
    "左\n右\nUV-K1(8)",
    "上\n下\nUV-K5(6)",
};

#ifdef ENABLE_VOICE
const char gSubMenu_VOICE_CN[][4] = {
    "OFF",
    "中",
    "英",
};
#endif

#ifdef ENABLE_ALARM
const char gSubMenu_AL_MOD_CN[][5] = {
    "SITE",
    "TONE",
};
#endif

#ifndef ENABLE_FEAT_F4HWN
const char gSubMenu_SCRAMBLER_CN[][7] = {
    "OFF",
    "2600Hz",
    "2700Hz",
    "2800Hz",
    "2900Hz",
    "3000Hz",
    "3100Hz",
    "3200Hz",
    "3300Hz",
    "3400Hz",
    "3500Hz",
};
#endif

const char gSubMenu_TXP_CN[][6] = {
    "USER",
    "LOW 1",
    "LOW 2",
    "LOW 3",
    "LOW 4",
    "LOW 5",
    "MID",
    "HIGH",
};

#ifdef ENABLE_FEAT_F4HWN
const char gSubMenu_SET_PWR_CN[][6] = {
    "< 20m",
    "125m",
    "250m",
    "500m",
    "1",
    "2",
    "5",
};

const char gSubMenu_SET_LCK_CN[][12] = {
    "キー",
    "キー+PTT",
};

const char gSubMenu_SET_MET_CN[][16] = {
    "簡易",
    "クラシック",
};

#ifdef ENABLE_FEAT_F4HWN_SCAN_FASTER
const char gSubMenu_SET_SCN_CN[][7] = {
    "通常",
    "高速",
};
#endif

# ifdef ENABLE_FEAT_F4HWN_AUDIO
const char gSubMenu_SET_AUD_FM_CN[][6] = {
    "FLAT",
    "CLEAN",
    "MID",
    "BOOST",
    "MAX",
};

const char gSubMenu_SET_AUD_AM_CN[][6] = {
    "SHARP",
    "STOCK",
    "OPEN",
};
# endif

# ifdef ENABLE_FEAT_F4HWN_NARROWER
const char gSubMenu_SET_NFM_CN[][16] = {
    "狭域",
    "更にナロー",
};
# endif

# ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
const char gSubMenu_SET_KEY_CN[][16] = {
    "MENUキー",
    "上キー",
    "下キー",
    "EXITキー",
    "*キー",
};
# endif
#endif /* ENABLE_FEAT_F4HWN */

const char *const gSubMenu_SIDEFUNCTIONS_CN[] = {
    "なし",
#ifdef ENABLE_FLASHLIGHT
    "ライト",
#endif
    "送信出力",
    "モニター",
    "スキャン",
#ifdef ENABLE_VOX
    "VOX",
#endif
#ifdef ENABLE_ALARM
    "警報",
#endif
#ifdef ENABLE_FMRADIO
    "FMラジオ",
#endif
#ifdef ENABLE_TX1750
    "1750Hz",
#endif
#ifdef ENABLE_REGA
    "REGA\n警報",
    "REGA\nテスト",
#endif
    "キーロック",
    "A/B切替",
    "VFO\nCH",
    "モード",
#ifdef ENABLE_BLMIN_TMP_OFF
    "照明一時OFF",
#endif
#ifdef ENABLE_FEAT_F4HWN
    "受信 モード",
    "メインのみ",
    "PTT",
    "ワイド\nナロー",
    "ミュート",
# ifdef ENABLE_FEAT_F4HWN_AUDIO
    "RxA",
# endif
# ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    "出力\n高",
    "シフト\n解除",
# endif
#endif
};

/* 内、计、划 不在嵌入中文字库；第一行用 Inside，第三行用「频段」（字库有 频、段）。 */
const char gSubMenu_TX_LOCK_INSIDE_CN[] =
    "Inside\nF Lock\n帯域";

const char gSubMenu_LIST_CN_OFF[] = "OFF";
const char gSubMenu_LIST_CN_ALL[] = "全部";
const char gSubMenu_SC_REV_STOP_CN[] = "停止";
const char gSubMenu_ABR_ON_CN[] = "ON";
const char gSubMenu_MEM_NONE_CN[] = "なし";

/* Same order as ModulationMode_t / gModulationStr (size MODULATION_UKNOWN). */
const char gSubMenu_MODULATION_CN[][8] = {
    "FM",
    "AM",
    "USB",
#ifdef ENABLE_BYP_RAW_DEMODULATORS
    "BYP",
    "RAW",
#endif
};

_Static_assert(ARRAY_SIZE(gSubMenu_MODULATION_CN) == MODULATION_UKNOWN, "gSubMenu_MODULATION_CN");

#ifdef ENABLE_SPECTRUM
const char gSubMenu_SPECTRUM_MODE_CN[][13] = {
    "シンプル",     /* 简单 */
    "詳細",     /* 专业 */
};
#endif

#endif /* ENABLE_CHINESE */
