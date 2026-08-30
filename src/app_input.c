/*
File:   app_input.c
Author: Taylor Robbins
Date:   08\29\2026
Description: 
	** Handles taking input events from SokolApp and Android and
	** making modifications to upcomingAppInput in AppState. Then
	** at the start of each frame we swap out the upcoming and current
	** AppInput, clear handling and changed flags, and update various
	** bits of the AppInput like timing info and screenSize
	** This file also contains the Input API which is how the majority of
	** code interfaces with the currentAppInput pntr in AppState.
	** NOTE: In app_main.c there is an AppState* appIn global variable that
	**       provides a convenient way to access app->currentAppInput
*/

// Called right before overwriting the old AppInput with the new one
// Allows us to free dynamic memory allocations so they don't leak
void FreeAppInputAllocations(AppInput* appInput)
{
	//TODO: Anything we need to free?
}

// After an AppInput has been consumed, we reset handling and changed flags so it
// can start getting mutated by events that would re-set these flags
void RefreshAppInput(AppInput* appInput)
{
	RefreshKeyboardStateHandling(&appInput->keyboard, &appInput->keyboardHandling);
	RefreshMouseStateHandling(&appInput->mouse, &appInput->mouseHandling);
	RefreshTouchscreenStateHandling(&appInput->touchscreen, &appInput->touchscreenHandling);
	appInput->screenSizeChanged = false;
	
	IncrementU64(appInput->frameIndex);
}

void UpdateScreenSizeInAppInput(AppInput* appInput, v2i newScreenSizei)
{
	if (!AreEqualV2i(appInput->screenSizei, newScreenSizei))
	{
		appInput->screenSizei = newScreenSizei;
		appInput->screenSize = ToV2Fromi(appInput->screenSizei);
		appInput->screenReci = MakeReciV(V2i_Zero, appInput->screenSizei);
		appInput->screenRec = MakeRecV(V2_Zero, appInput->screenSize);
		appInput->screenSizeChanged = true;
	}
}

//NOTE: frameIndex is incremented in RefreshAppInput
void PrepareAppInputForFrame(AppInput* appInput)
{
	OsTime prevTime = appInput->currentTime;
	appInput->currentTime = OsGetTime();
	if (appInput->frameIndex == 0) { prevTime = appInput->currentTime; } //ignore difference between 0 and first frame time
	
	appInput->programTime = appInput->currentTime.msSinceStart;
	appInput->programTimeRemainder = appInput->currentTime.msSinceStartRemainder;
	
	r32 elapsedMsRemainder = 0.0f;
	u64 elapsedMs = OsTimeDiffMsU64(prevTime, appInput->currentTime, &elapsedMsRemainder);
	appInput->unclampedElapsedMsR64 = (r64)elapsedMs + (r64)elapsedMsRemainder;
	if (elapsedMs < MIN_ELAPSED_MS) { elapsedMs = MIN_ELAPSED_MS; elapsedMsRemainder = 0.0f; }
	else if (elapsedMs > MAX_ELAPSED_MS) { elapsedMs = MAX_ELAPSED_MS; elapsedMsRemainder = 0.0f; }
	else if (elapsedMs == MAX_ELAPSED_MS && elapsedMsRemainder > 0.0f) { elapsedMsRemainder = 0.0f; }
	appInput->elapsedMsR64 = (r64)elapsedMs + (r64)elapsedMsRemainder;
	appInput->elapsedMs = (r32)appInput->elapsedMsR64;
	
	appInput->timeScaleR64 = appInput->elapsedMsR64 / (1000.0 / TIME_SCALE_TARGET_FRAMERATE);
	if (AreSimilarR64(appInput->timeScaleR64, 1.0, TIME_SCALE_ROUND_TOLERANCE)) { appInput->timeScaleR64 = 1.0; }
	appInput->timeScale = (r32)appInput->timeScaleR64;
	
	v2i newScreenSizei = MakeV2i((i32)sapp_width(), (i32)sapp_height());
	UpdateScreenSizeInAppInput(appInput, newScreenSizei);
}

void HandleSokolAppInputEvent(const sapp_event* event)
{
	if (app == nullptr || !app->appInitFinished) { return; }
	DebugAssert(OsIsMainThread());
	
	uxx programTime = OsGetTime().msSinceStart;
	v2i screenSizei = MakeV2i((i32)sapp_width(), (i32)sapp_height());
	bool isHandledByKeyboardMouseOrTouchscreenState = HandleSokolKeyboardMouseAndTouchEvents(
		event,
		programTime,
		screenSizei,
		&app->upcomingAppInput->keyboard, &app->upcomingAppInput->mouse, &app->upcomingAppInput->touchscreen,
		/*isMouseLocked*/ false
	);
	if (!isHandledByKeyboardMouseOrTouchscreenState)
	{
		switch (event->type)
		{
			case SAPP_EVENTTYPE_RESIZED: UpdateScreenSizeInAppInput(app->upcomingAppInput, screenSizei); break;
			case SAPP_EVENTTYPE_ICONIFIED:         { /*TODO: Implement me!*/ } break;
			case SAPP_EVENTTYPE_RESTORED:          { /*TODO: Implement me!*/ } break;
			case SAPP_EVENTTYPE_FOCUSED:           { /*TODO: Implement me!*/ } break;
			case SAPP_EVENTTYPE_UNFOCUSED:         { /*TODO: Implement me!*/ } break;
			case SAPP_EVENTTYPE_SUSPENDED:         { /*TODO: Implement me!*/ } break;
			case SAPP_EVENTTYPE_RESUMED:           { /*TODO: Implement me!*/ } break;
			case SAPP_EVENTTYPE_QUIT_REQUESTED:    { /*TODO: Implement me!*/ } break;
			case SAPP_EVENTTYPE_CLIPBOARD_PASTED:  { /*TODO: Implement me!*/ } break;
			case SAPP_EVENTTYPE_FILES_DROPPED:     { /*TODO: Implement me!*/ } break;
			// case SAPP_EVENTTYPE_RESIZE_RENDER: NOTE: This never happens on Android. This is our own custom modification to sokol_app.h to support smooth resizing on Desktop platforms
			default:
			{
				PrintLine_D("Unhandled Sokol App Event: %d", event->type);
			} break;
		}
	}
}
