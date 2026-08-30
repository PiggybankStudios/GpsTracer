/*
File:   app_jni.c
Author: Taylor Robbins
Date:   08\30\2026
Description: 
	** Holds functions that call out to the Java VM through the JNI
	** Mostly the functions in this file wrap a series of calls to
	** things in PigCore's "os_jni.h"
*/

#define MOBILE_BASE_DPI 160 //pixels per inch

r32 GetScreenDpiScale(i32* screenDpiOut)
{
	AConfiguration* androidConfig = AConfiguration_new();
	AConfiguration_fromAssetManager(androidConfig, AndroidNativeActivity->assetManager);
	i32 screenDpi = AConfiguration_getDensity(androidConfig);
	if (screenDpi == ACONFIGURATION_DENSITY_DEFAULT) { screenDpi = MOBILE_BASE_DPI; }
	else if (screenDpi == ACONFIGURATION_DENSITY_NONE || screenDpi == ACONFIGURATION_DENSITY_ANY) { screenDpi = MOBILE_BASE_DPI; }
	else if (screenDpi == ACONFIGURATION_DENSITY_TV) { screenDpi = 213; }
	else if (screenDpi < 0) { screenDpi = MOBILE_BASE_DPI; }
	SetOptionalOutPntr(screenDpiOut, screenDpi);
	return (r32)screenDpi / (r32)MOBILE_BASE_DPI;
}

r32 GetAndroidFontScale()
{
	r32 result = 1.0f;
	// Java equivalent:
	//   return Activity.getResources().getConfiguration().fontScale
	JavaVMAttachBlock(env)
	{
		jobject resources = jCall_getResources(env, AndroidNativeActivity);
		jobject configuration = jCall_getConfiguration(env, resources);
		result = jObjGetField(env, configuration, /*staticField*/false, "fontScale", "F", JvmType_Float, true).floatValue;
		(*env)->DeleteLocalRef(env, configuration);
		(*env)->DeleteLocalRef(env, resources);
	}
	return MaxR32(0.1f, result);
}

void GetScreenSafeMargins(v4* screenMarginsOut, v4* avoidCutoutsMarginsOut)
{
	SetOptionalOutPntr(screenMarginsOut, V4_Zero);
	SetOptionalOutPntr(avoidCutoutsMarginsOut, V4_Zero);
	
	JavaVMAttachBlock(env)
	{
		if ((*env)->GetVersion(env) > jGetField_Build_VERSION_CODES(env, "P"))
		{
			jobject window    = jCall_getWindow(env, AndroidNativeActivity); Assert(window != nullptr);
			jobject decorView = jCall_getDecorView(env, window); Assert(decorView != nullptr);
			jobject insets    = jCall_getRootWindowInsets(env, decorView); Assert(insets != nullptr);
			
			if (screenMarginsOut != nullptr)
			{
				screenMarginsOut->left   = (r32)jCall_getSystemWindowInsetLeft(env,   insets);
				screenMarginsOut->top    = (r32)jCall_getSystemWindowInsetTop(env,    insets);
				screenMarginsOut->right  = (r32)jCall_getSystemWindowInsetRight(env,  insets);
				screenMarginsOut->bottom = (r32)jCall_getSystemWindowInsetBottom(env, insets);
			}
			
			if (avoidCutoutsMarginsOut != nullptr)
			{
				//TODO: For some reason this stopped working! It was giving us information about what portion of the screen had no overlaps with cutouts and then it stopped giving us anything ever
				// We tried the following two blocks to configure the cutout reporting mode. We need to do more research to figure out what the full reliable path is for getting cutout information
				
				// Java equivalent:
				//   WindowManager.LayoutParams = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
				// i32 LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS = jClassGetField(env, true, "android/view/WindowManager$LayoutParams", "LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS", "I", JvmType_Int, true).intValue;
				// jClassSetField(env, false, "android/view/WindowManager$LayoutParams", "layoutInDisplayCutoutMode", "I", MakeJvmReturnInt(LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS));
				
				// static bool setAttributes = false;
				// if (!setAttributes)
				// {
				// 	jobject windowAttribs = jCall_getAttributes(env, window);
				// 	JvmReturn LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS = jClassGetField(env, true, "android/view/WindowManager$LayoutParams", "LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS", "I", JvmType_Int, true);
				// 	jObjSetField(env, windowAttribs, false, "layoutInDisplayCutoutMode", "I", LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS);
				// 	jCall_setAttributes(env, window, windowAttribs);
				// 	(*env)->DeleteLocalRef(env, windowAttribs);
				// 	WriteLine_I("Success:\nvar attribs = Window.getAttributes()\nattribs.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;\nWindow.setAttributes(attribs);");
				// 	setAttributes = true;
				// }
				
				//TODO: Really we should do DisplayCutout.getBoundingRects() which returns a List<Rect> for all cutouts
				jobject displayCutout = jCall_getDisplayCutout(env, insets);
				if (displayCutout != nullptr)
				{
					avoidCutoutsMarginsOut->left   = (r32)jCall_getSafeInsetLeft(env,   displayCutout);
					avoidCutoutsMarginsOut->top    = (r32)jCall_getSafeInsetTop(env,    displayCutout);
					avoidCutoutsMarginsOut->right  = (r32)jCall_getSafeInsetRight(env,  displayCutout);
					avoidCutoutsMarginsOut->bottom = (r32)jCall_getSafeInsetBottom(env, displayCutout);
					
					avoidCutoutsMarginsOut->left   = MaxR32(screenMarginsOut->left,   avoidCutoutsMarginsOut->left);
					avoidCutoutsMarginsOut->top    = MaxR32(screenMarginsOut->top,    avoidCutoutsMarginsOut->top);
					avoidCutoutsMarginsOut->right  = MaxR32(screenMarginsOut->right,  avoidCutoutsMarginsOut->right);
					avoidCutoutsMarginsOut->bottom = MaxR32(screenMarginsOut->bottom, avoidCutoutsMarginsOut->bottom);
					
					(*env)->DeleteLocalRef(env, displayCutout);
				}
				else
				{
					*avoidCutoutsMarginsOut = *screenMarginsOut;
					static bool printedWarning = false;
					if (!printedWarning) { WriteLine_W("Can't get DisplayCutout from WindowInsets"); printedWarning = true; }
				}
			}
			
			(*env)->DeleteLocalRef(env, insets);
			(*env)->DeleteLocalRef(env, decorView);
			(*env)->DeleteLocalRef(env, window);
		}
		else
		{
			static bool printedWarning = false;
			if (!printedWarning) { WriteLine_W("Can't get safeMargins for the app window because JVM version is too old!"); printedWarning = true; }
		}
	}
}
