/*
File:   build_config.h
Author: Taylor Robbins
Date:   08\24\2026
Description:
	** This file contains a bunch of options that control the build script.
	** This file is both a C header file that can be #included from a .c file,
	** and it is also scraped by the build script to extract values to change the work it performs.
	** Because it is scraped, and not parsed with the full C language spec, we must
	** be careful to keep this file very simple and not introduce any syntax that
	** would confuse the scraper when it's searching for values
	** NOTE: See pig_build_misc.h for the logic that extracts #defines from header files
*/

#ifndef _BUILD_CONFIG_H
#define /*DONT SHOW IN CSWITCH*/ _BUILD_CONFIG_H

#define PROJECT_READABLE_NAME GPS Tracer
#define PROJECT_FOLDER_NAME   gps_tracer
#define PROJECT_SO_NAME       libapp.so
#define PROJECT_APK_NAME      GpsTracer

// +--------------------------------------------------------------+
// |                        Build Options                         |
// +--------------------------------------------------------------+
#define DEBUG_BUILD           1


#define BUILD_FOR_DEVICE      1
#define BUILD_FOR_SIMULATOR   0

// Compile native .so binaries to arm64-v8a, armeabi-v7a, and x86_64. If disabled we only compile for arm64-v8a (aka 64-bit ARM, most phones). See https://developer.android.com/ndk/guides/abis
#define BUILD_FAT_APK 0

// Runs the sokol-shdc.exe on all .glsl files in the source directory to produce .glsl.h and .glsl.c files and then compiles the .glsl.c files to .obj
#define BUILD_SHADERS                0
// This puts all the contents of _data/resources into a zip file and converts the contents of that zip into resources_zip.c (and resources_zip.h in app/)
#define ZIP_RESOURCES_FOR_EMBEDDING  0
// The .exe will use the resources_zip.h/c file instead of loading resources from disk
#define USE_EMBEDDED_RESOURCES_ZIP   1
// Installs the Android .apk onto a connected Android device through adb (Android Debug Bridge)
#define INSTALL_APK                  1

// Rather than compiling the project(s) it will simply output the
// result of the preprocessor's pass over the code to the build folder
#define DUMP_PREPROCESSOR 0
// Generates assembly listing files for all compilation units
#define DUMP_ASSEMBLY 0


// +===============================+
// | Optional Libraries/Frameworks |
// +===============================+
// Enables pig_core.dll and tests.exe using sokol_gfx.h (and on non-windows OS' adds required libraries for Sokol to work)
#define BUILD_WITH_SOKOL_GFX 1
// Enables tests.exe using sokol_app.h to create and manage a graphical window
#define BUILD_WITH_SOKOL_APP 1
// Enables tests.exe using clay.h to render UI elements
#define BUILD_WITH_CLAY      1
// Enables tests.exe using our own Immediate Mode style UI system
#define BUILD_WITH_PIG_UI    0
// Enables tests.exe and pig_core.dll being linked with imgui.obj
#define BUILD_WITH_IMGUI     0
// Enables building with the FreeType library which provides better font rasterizing support than stb_truetype.h (the default dependency)
#define BUILD_WITH_FREETYPE  0

//This should stay enabled, it makes the ui_system_core.h file #include "pig_ui_config.h"
#define PIG_CORE_INCLUDE_PIG_UI_CONFIG 1
// We don't need the debug output callback in base_debug_output_impl.h
#define DEBUG_OUTPUT_CALLBACK_GLOBAL 0

// +--------------------------------------------------------------+
// |                       Android Related                        |
// +--------------------------------------------------------------+
#define ANDROID_SIGNING_KEY_PATH     /Users/robbitay/my/misc/android_keystore.jks
#define ANDROID_SIGNING_PASS_PATH    /Users/robbitay/my/misc/android_keystore_password.txt

//folder name inside %ANDROID_SDK%/ndk/
#define ANDROID_NDK_VERSION          29.0.13599879
//folder name inside %ANDROID_SDK%/platforms/
#define ANDROID_PLATFORM_FOLDERNAME  android-36
//folder name inside %ANDROID_SDK%/build-tools/
#define ANDROID_BUILD_TOOLS_VERSION  36.0.0
#define ANDROID_PACKAGE_PATH         com.piggybank.gpstracer
#define ANDROID_ACTIVITY_PATH        com.piggybank.gpstracer/android.app.NativeActivity

// +--------------------------------------------------------------+
// |                        String Defines                        |
// +--------------------------------------------------------------+
#ifndef STRINGIFY_DEFINE
#define STRINGIFY_DEFINE(define) STRINGIFY(define)
#endif
#ifndef STRINGIFY
#define STRINGIFY(text)          #text
#endif

#define PROJECT_READABLE_NAME_STR STRINGIFY_DEFINE(PROJECT_READABLE_NAME)
#define PROJECT_FOLDER_NAME_STR   STRINGIFY_DEFINE(PROJECT_FOLDER_NAME)
#define PROJECT_SO_NAME_STR       STRINGIFY_DEFINE(PROJECT_SO_NAME)
#define PROJECT_APK_NAME_STR      STRINGIFY_DEFINE(PROJECT_APK_NAME)

#define ANDROID_SIGNING_KEY_PATH_STR  STRINGIFY_DEFINE(ANDROID_SIGNING_KEY_PATH)
#ifdef ANDROID_SIGNING_PASSWORD
#define ANDROID_SIGNING_PASSWORD_STR  STRINGIFY_DEFINE(ANDROID_SIGNING_PASSWORD)
#endif
#ifdef ANDROID_SIGNING_PASS_PATH
#define ANDROID_SIGNING_PASS_PATH_STR STRINGIFY_DEFINE(ANDROID_SIGNING_PASS_PATH)
#endif
#define ANDROID_NDK_VERSION_STR         STRINGIFY_DEFINE(ANDROID_NDK_VERSION)
#define ANDROID_PLATFORM_FOLDERNAME_STR STRINGIFY_DEFINE(ANDROID_PLATFORM_FOLDERNAME)
#define ANDROID_BUILD_TOOLS_VERSION_STR STRINGIFY_DEFINE(ANDROID_BUILD_TOOLS_VERSION)
#define ANDROID_PACKAGE_PATH_STR        STRINGIFY_DEFINE(ANDROID_PACKAGE_PATH)
#define ANDROID_ACTIVITY_PATH_STR       STRINGIFY_DEFINE(ANDROID_ACTIVITY_PATH)


//Required by pig_build_pig_core_gui_app.h
#define BUILD_INTO_SINGLE_UNIT       1
#define BUILD_THIS_PLATFORM          0
#define BUILD_LINUX_VIA_WSL          0
#define BUILD_PIGGEN                 0
#define RUN_PIGGEN                   0
#define GENERATE_PROTOBUF            0
#define BUILD_TRACY_DLL              0
#define BUILD_PIG_CORE_DLL           0
#define BUILD_APP_EXE                0
#define BUILD_APP_DLL                0
#define PROFILING_ENABLED            0
#define USE_OSX_APP_BUNDLE_RESOURCES 0
#define RUN_APP                      0
#define COPY_TO_DATA_DIRECTORY       0
#define BUILD_WITH_RAYLIB            0
#define BUILD_WITH_BOX2D             0
#define BUILD_WITH_SDL               0
#define BUILD_WITH_OPENVR            0
#define BUILD_WITH_PHYSX             0
#define BUILD_WITH_METADESK          0
#define BUILD_WITH_HTTP              0
#define BUILD_WITH_PROTOBUF          0
#define BUILD_WITH_GTK               0
#define PROJECT_DLL_NAME       GpsTracerApp
#define PROJECT_EXE_NAME       GpsTracer

#endif //  _BUILD_CONFIG_H
