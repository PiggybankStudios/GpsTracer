/*
File:   app_state.h
Author: Taylor Robbins
Date:   08\29\2026
Description:
	** This one structure holds all of the state for the Application
	** It is zero initialized by appStruct global in app_main.c and
	** the "AppState* app" global is how most consumers access it's data
	** Some parts of the state are initialize in sokol_main, and others
	** later in AppInit.
	** NOTE: "appInitFinished" can be used as a simple test gate code
	** that expects to only run after initialization has finished but may
	** get called early in the initialization process for one reason or another.
*/

#ifndef _APP_STATE_H
#define _APP_STATE_H

typedef struct AppState AppState;
struct AppState
{	
	bool appInitFinished;
	
	Arena stdHeapStruct;
	Arena untrackedStdHeapStruct;
	RandomSeries random;
	AppResources resources;
	
	AppInput appInputsArray[2];
	AppInput* currentAppInput;
	AppInput* upcomingAppInput;
	
	Shader mainShader;
	
};

#endif //  _APP_STATE_H
