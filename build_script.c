/*
File:   build_script.c
Author: Taylor Robbins
Date:   08\24\2026
Description: 
	** This is the script that holds the logic for building the app.
	** This script is compiled with the build.sh/build.bat which
	** utilizes PigBuild to compile and run this script as builder.exe inside a `build/` folder
*/

#include "pig_build_base.h"
#if BUILDING_ON_OSX
//NOTE: For Windows/Linux this should be set as an environment variable.
//      However on OSX it is difficult to set an environment variable globally, so we hard-code the SDK path for that platform.
//      See pig_build_android.h -> GetAndroidSdkPath()
#define ANDROID_SDK "/Users/robbitay/Library/Android/sdk"
#endif

#include "pig_build.h"
#include "pig_build_optional.h"

//TODO: Make something to replace https://romannurik.github.io/AndroidAssetStudio/

int main(int argc, char* argv[])
{
	PigBuildDebugMode = false;
	RecompileIfNeeded(StrArray_Empty);
	PrintLine("[%s...]", BUILD_SCRIPT_EXE_NAME);
	
	StrArray cliArgs = EMPTY;
	//NOTE: We skip the first argument which is just the executable relative path
	for (int aIndex = 1; aIndex < argc; aIndex++) { AddStrNt(&cliArgs, argv[aIndex]); }
	
	if (!DoesFolderExist(StrLit("../core")) || !DoesFileExist(StrLit("../core/src/base/base_compiler_check.h")))
	{
		WriteLine_E(
			"Please download PigCore into a folder named \"core\" in the root directory!\n"
			"git clone https://github.com/PiggybankStudios/PigCore core"
		);
		return 2;
	}
	
	// +--------------------------------------------------------------+
	// |                     Parse build_config.h                     |
	// +--------------------------------------------------------------+
	StrArray buildConfigTags = EMPTY;
	Str buildConfigContents = ReadEntireFile(StrLit("../build_config.h"));
	Str PROJECT_READABLE_NAME = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("PROJECT_READABLE_NAME")));
	Str PROJECT_FOLDER_NAME   = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("PROJECT_FOLDER_NAME")));
	Str PROJECT_SO_NAME       = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("PROJECT_SO_NAME")));
	Str PROJECT_APK_NAME      = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("PROJECT_APK_NAME")));
	Str ANDROID_SIGNING_KEY_PATH     = CopyStr(TryExtractStrDefine(buildConfigContents, StrLit("ANDROID_SIGNING_KEY_PATH"),  Str_Empty));
	Str ANDROID_SIGNING_PASS_PATH    = CopyStr(TryExtractStrDefine(buildConfigContents, StrLit("ANDROID_SIGNING_PASS_PATH"), Str_Empty));
	Str ANDROID_NDK_VERSION          = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("ANDROID_NDK_VERSION")));
	Str ANDROID_PLATFORM_FOLDERNAME  = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("ANDROID_PLATFORM_FOLDERNAME")));
	Str ANDROID_BUILD_TOOLS_VERSION  = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("ANDROID_BUILD_TOOLS_VERSION")));
	Str ANDROID_PACKAGE_PATH         = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("ANDROID_PACKAGE_PATH")));
	Str ANDROID_ACTIVITY_PATH        = CopyStr(ExtractStrDefine(buildConfigContents, StrLit("ANDROID_ACTIVITY_PATH")));
	#define LOAD_CONFIG(CONFIG_NAME)                                                     \
		bool CONFIG_NAME = ExtractBoolDefine(buildConfigContents, StrLit(#CONFIG_NAME)); \
		if (CONFIG_NAME) { AddStrLit(&buildConfigTags, #CONFIG_NAME); }                  \
		do {} while(0)
	LOAD_CONFIG(DEBUG_BUILD);
	LOAD_CONFIG(BUILD_FOR_DEVICE);
	LOAD_CONFIG(BUILD_FOR_SIMULATOR);
	LOAD_CONFIG(BUILD_FAT_APK);
	LOAD_CONFIG(BUILD_SHADERS);
	LOAD_CONFIG(ZIP_RESOURCES_FOR_EMBEDDING);
	LOAD_CONFIG(USE_EMBEDDED_RESOURCES_ZIP);
	LOAD_CONFIG(INSTALL_APK);
	LOAD_CONFIG(DUMP_PREPROCESSOR);
	LOAD_CONFIG(DUMP_ASSEMBLY);
	#undef LOAD_CONFIG
	
	Str apkFilename = JoinStrings2(PROJECT_APK_NAME, StrLit(".apk"));
	
	// +==============================+
	// | Enforce Config Restrictions  |
	// +==============================+
	if (!BUILD_FOR_DEVICE && !BUILD_FOR_SIMULATOR && !(INSTALL_APK && DoesFileExist(apkFilename)))
	{
		WriteLine_E("You must enable either BUILD_FOR_DEVICE or BUILD_FOR_SIMULATOR (or INSTALL_APK if the .apk has been built previously)");
		return 1;
	}
	if (USE_EMBEDDED_RESOURCES_ZIP && !ZIP_RESOURCES_FOR_EMBEDDING &&
		(!DoesFileExist(StrLit("app_resources.zip")) ||
		 !DoesFileExist(StrLit("gen/app_resources_zip.h")) ||
		 !DoesFileExist(StrLit("gen/app_resources_zip.c"))
	    ))
	{
		WriteLine("Auto-enabling ZIP_RESOURCES_FOR_EMBEDDING because they haven't been zipped before and USE_EMBEDDED_RESOURCES_ZIP is enabled");
		ZIP_RESOURCES_FOR_EMBEDDING = true;
	}
	
	// +--------------------------------------------------------------+
	// |                     Setup Compiler Flags                     |
	// +--------------------------------------------------------------+
	Str androidSdkPath = GetAndroidSdkPath();
	PrintLine("Using Android SDK at \"%.*s\"", StrPrint(androidSdkPath));
	AndroidBinPaths androidPaths = EMPTY;
	FillAndroidBinPaths(&androidPaths, androidSdkPath, ANDROID_NDK_VERSION, ANDROID_PLATFORM_FOLDERNAME, ANDROID_BUILD_TOOLS_VERSION);
	
	CliArgs commonCompilerArgs = EMPTY;
	CliArgs commonLinkerArgs = EMPTY;
	AddIncludeDirArgLit(&commonCompilerArgs, "[ROOT]");
	AddIncludeDirArgLit(&commonCompilerArgs, "[ROOT]/src");
	AddIncludeDirArgLit(&commonCompilerArgs, "[ROOT]/build/gen");
	FillPigCoreFlags(&commonCompilerArgs, &commonLinkerArgs, StrLit("[ROOT]/core"));
	FillAndroidFlags(&commonCompilerArgs, &commonLinkerArgs, &androidPaths);
	
	CliArgs thingsToLink = EMPTY;
	//TODO: This should get filled with things like shader.o, tracy.a, imgui.a, etc.
	
	// +--------------------------------------------------------------+
	// |                        Zip Resources                         |
	// +--------------------------------------------------------------+
	if (ZIP_RESOURCES_FOR_EMBEDDING)
	{
		MyCreateFolder(StrLit("gen"), false);
		BundleResourcesZip(
			StrLit("../resources"),
			StrLit("app_resources.zip"),
			StrLit("gen/app_resources_zip.h"),
			StrLit("gen/app_resources_zip.c"),
			StrLit("app_resources_zip_bytes")
		);
	}
	
	// +--------------------------------------------------------------+
	// |                        Compile to .so                        |
	// +--------------------------------------------------------------+
	// Compile the program to .so that will get embedded into the .apk under `/lib/[arch]/PROJECT_SO_NAME'
	{
		CliArgs args = EMPTY;
		AddArgNt(&args, CLI_QUOTED_ARG, "[ROOT]/src/app_main.c");
		AddArgNt(&args, CLI_QUOTED_ARG, "[ROOT]/build/gen/main2d_shader.glsl.c");
		// for (u64 archIndex = 1; archIndex < AndroidTargetArchitecture_Count; archIndex++)
		// {
		// 	AndroidTargetArchitecture architecture = (AndroidTargetArchitecture)archIndex;
		// 	for (u64 sIndex = 0; sIndex < clang_AndroidShaderObjects[archIndex].length; sIndex++)
		// 	{
		// 		AddTaggedArgStr(&args, GetAndroidTargetArchitectureTag(architecture), CLI_QUOTED_ARG, clang_AndroidShaderObjects[archIndex].strings[sIndex]);
		// 	}
		// }
		AddArgList(&args, &commonCompilerArgs);
		AddArgList(&args, &commonLinkerArgs);
		AddArgList(&args, &thingsToLink);
		
		StrArray tags = EMPTY;
		AddTag(&tags, T_LANG_C);
		AddStrArray(&tags, &buildConfigTags);
		
		Str compileOutputFilename = (DUMP_PREPROCESSOR ? StrLit("main_PREPROCESSED.c") : PROJECT_SO_NAME);
		BuildAndroidSharedLibraries(&androidPaths, StrLit(".."),
			&args,
			&tags,
			StrLit("lib"),
			compileOutputFilename,
			BUILD_FAT_APK
		);
	}
	
	// +--------------------------------------------------------------+
	// |                         Package .apk                         |
	// +--------------------------------------------------------------+
	if (!DUMP_PREPROCESSOR)
	{
		//TODO: Eventually we should compile some actual Java code
		Str classesDexPath = StrLit("classes.dex");
		if (!DoesFileExist(classesDexPath))
		{
			WriteLine("Compiling Dummy.java to classes.dex...");
			CompileDummyJavaToClassesDex(&androidPaths, StrLit(".."),
				StrLit("Dummy.java"),
				classesDexPath
			);
		}
		
		if (!DoesFileExist(StrLit("resources.zip")))
		{
			WriteLine("Packaging resources.zip...");
			PackageAndroidResourcesZip(&androidPaths, StrLit(".."),
				StrLit("[ROOT]/android/res"),
				StrLit("resources.zip")
			);
		}
		
		PrintLine("Linking %.*s...", StrPrint(apkFilename));
		TryRemoveFile(apkFilename);
		LinkAndroidApk(&androidPaths, StrLit(".."),
			StrLit("[ROOT]/android/AndroidManifest.xml"),
			StrLit("resources.zip"),
			apkFilename
		);
		
		AddNativeBinariesAndClassesDexToAndroidApk(&androidPaths, StrLit(".."),
			apkFilename,
			StrLit("apk_temp"),
			StrLit("lib"),
			PROJECT_SO_NAME,
			classesDexPath,
			BUILD_FAT_APK
		);
		
		// Aligning the zip takes a little time and we don't have to do it for debug builds
		if (!DEBUG_BUILD)
		{
			WriteLine("Performing ZIP alignment...");
			AlignAndroidApk(&androidPaths, StrLit(".."), apkFilename, StrLit("aligned.apk"));
		}
		
		if (!IsEmptyStr(ANDROID_SIGNING_KEY_PATH))
		{
			PrintLine("Signing %.*s with %.*s...", StrPrint(apkFilename), StrPrint(ANDROID_SIGNING_KEY_PATH));
			SignAndroidApk(&androidPaths, StrLit(".."), apkFilename, ANDROID_SIGNING_KEY_PATH, ANDROID_SIGNING_PASS_PATH);
		}
		else
		{
			PrintLine("Debug Signing %.*s...", StrPrint(apkFilename));
			DebugSignAndroidApk(&androidPaths, StrLit(".."), apkFilename, StrLit("debug.keystore"));
		}
	}
	
	// +--------------------------------------------------------------+
	// |                         Install .apk                         |
	// +--------------------------------------------------------------+
	if (INSTALL_APK)
	{
		PrintLine("\n[Installing %.*s on Device...]", StrPrint(apkFilename));
		InstallAndroidApk(&androidPaths, StrLit(".."),
			apkFilename,
			ANDROID_ACTIVITY_PATH
		);
	}
}