/*
File:   app_defines.h
Author: Taylor Robbins
Date:   08\29\2026
*/

#ifndef _APP_DEFINES_H
#define _APP_DEFINES_H

#define SCRATCH_ARENAS_SIZE   Megabytes(32)

#define MIN_ELAPSED_MS              5  //ms
#define MAX_ELAPSED_MS              67 //ms
#define TIME_SCALE_TARGET_FRAMERATE 60 //fps
#define TIME_SCALE_ROUND_TOLERANCE  0.1f

#define UI_FONT_NAME        "SourceSansPro"
// #define UI_FONT_NAME        "Roboto"
// #define UI_FONT_NAME        "DroidSans" //really just maps to Roboto
#define UI_FONT_LARGE_SIZE  26
#define UI_FONT_SMALL_SIZE  18

#define IS_APP_HIGH_DPI_AWARE  true
#define APP_MSAA_SAMPLE_COUNT  2 //TODO: Does this work on Android?
#define APP_SWAP_INTERVAL      1 //TODO: 16ms aka 60fps? Is this ignored on Android?

#endif //  _APP_DEFINES_H
