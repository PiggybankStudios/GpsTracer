/*
File:   app_input.h
Author: Taylor Robbins
Date:   08\29\2026
*/

#ifndef _APP_INPUT_H
#define _APP_INPUT_H

typedef struct AppInput AppInput;
struct AppInput
{
	u64 frameIndex;
	
	v2i screenSizei;
	v2 screenSize;
	reci screenReci;
	rec screenRec;
	bool screenSizeChanged;
	
	OsTime currentTime;
	u64 programTime; //ms since start of program (or really, since end of AppInit)
	r32 programTimeRemainder;
	r64 unclampedElapsedMsR64;
	r64 elapsedMsR64;
	r32 elapsedMs; //NOTE: Capped between [MIN_ELAPSED_MS, MAX_ELAPSED_MS]
	r64 timeScaleR64;
	r32 timeScale; //NOTE: Rounded to 1.0 when within TIME_SCALE_ROUND_TOLERANCE
	
	KeyboardState keyboard;
	MouseState mouse;
	TouchscreenState touchscreen;
	
	KeyboardStateHandling keyboardHandling;
	MouseStateHandling mouseHandling;
	TouchscreenStateHandling touchscreenHandling;
};

#endif //  _APP_INPUT_H
