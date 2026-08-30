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

typedef enum ShaderTargetPlatform ShaderTargetPlatform;
enum ShaderTargetPlatform
{
	ShaderTargetPlatform_None = 0x00,
	ShaderTargetPlatform_ThisPlatform  = 0x01,
	ShaderTargetPlatform_LinuxViaWsl   = 0x02,
	ShaderTargetPlatform_Android       = 0x04,
	ShaderTargetPlatform_Web           = 0x08,
	ShaderTargetPlatform_WebEmscripten = 0x10,
	ShaderTargetPlatform_Orca          = 0x20,
};

typedef struct ShaderInfo ShaderInfo;
struct ShaderInfo
{
	Str name;
	Str glslPath;
	Str headerPath;
	Str sourcePath;
	Str objPath;
	Str linuxObjPath;
	Str androidObjPaths[AndroidTargetArchitecture_Count];
	Str webObjPath;
	Str webEmscriptenObjPath;
	Str orcaObjPath;
};
TYPED_ARRAY(Array_ShaderInfo, ShaderInfo, infos);

Array_ShaderInfo CrossCompileShadersInFolderWithShdc(Str pigCoreFolder, Str targetDir, Str generatedCodeDir, u8 targetPlatforms, bool forceRebuild, StrArray* compileTags, const CliArgs* compilerArgs, CliArgs* linkerArgs)
{
	Array_ShaderInfo result = EMPTY;
	bool buildForThisPlatform        = IsFlagSet(targetPlatforms, ShaderTargetPlatform_ThisPlatform);
	bool crossCompileForLinuxWithWsl = (BUILDING_ON_WINDOWS && IsFlagSet(targetPlatforms, ShaderTargetPlatform_LinuxViaWsl));
	bool buildForAndroid             = IsFlagSet(targetPlatforms, ShaderTargetPlatform_Android);
	bool buildForWeb                 = IsFlagSet(targetPlatforms, ShaderTargetPlatform_Web);
	bool buildForWebEmscripten       = IsFlagSet(targetPlatforms, ShaderTargetPlatform_WebEmscripten);
	bool buildForOrca                = IsFlagSet(targetPlatforms, ShaderTargetPlatform_Orca);
	
	Str generatedCodeDirResolved = ResolveRootTo(generatedCodeDir, StrLit(".."));
	MyCreateFolder(generatedCodeDirResolved, true);
	
	FileIter fileIter = StartFileIter(ResolveRootTo(targetDir, StrLit("..")));
	Str iterPath = EMPTY;
	bool iterIsFolder = false;
	while (StepFileIter(&fileIter, &iterPath, &iterIsFolder))
	{
		if (!iterIsFolder && StrAnyCaseEquals(GetFileExtPart(iterPath, false), StrLit(".glsl")))
		{
			ShaderInfo* newShader = AddItemArray_ShaderInfo(&result);
			memset(newShader, 0x00, sizeof(ShaderInfo));
			newShader->glslPath = CopyStr(iterPath);
			FixPathSlashes(newShader->glslPath, '/');
			Str headerName = JoinStrings2(GetFileNamePart(newShader->glslPath, true), StrLit(".h"));
			Str objName = JoinStrings2(GetFileNamePart(newShader->glslPath, false), StrLit(OBJ_EXT));
			newShader->name = GetFileNamePart(newShader->glslPath, false);
			if (StrAnyCaseEndsWith(newShader->name, StrLit("shader"))) { newShader->name.length -= StrLit("shader").length; }
			if (StrExactEndsWith(newShader->name, StrLit("_"))) { newShader->name.length -= StrLit("_").length; }
			newShader->headerPath = JoinPaths(generatedCodeDir, headerName);
			newShader->sourcePath = ChangePathExtension(newShader->headerPath, StrLit(".c"), false);
			if (buildForThisPlatform) { newShader->objPath = JoinPaths(StrLit("[ROOT]/build/"), objName); }
			if (crossCompileForLinuxWithWsl) { newShader->linuxObjPath = JoinPaths(StrLit("[ROOT]/build/linux"), objName); }
			if (buildForAndroid)
			{
				for (u8 archIndex = 1; archIndex < AndroidTargetArchitecture_Count; archIndex++)
				{
					AndroidTargetArchitecture architecture = (AndroidTargetArchitecture)archIndex;
					Str archFolderName = MakeStrNt(GetAndroidTargetArchitectureFolderName(architecture));
					//TODO: Maybe we should have an option to not put android artifacts in android sub-folder. For projects that only build for Mobile this is annoying
					newShader->androidObjPaths[archIndex] = JoinPaths3(StrLit("[ROOT]/build/android/lib/"), archFolderName, objName);
				}
			}
			//TODO: Add support for buildForWeb
			//TODO: Add support for buildForWebEmscripten
			//TODO: Add support for buildForOrca
		}
	}
	
	for (u64 sIndex = 0; sIndex < result.length; sIndex++)
	{
		ShaderInfo* shaderInfo = &result.infos[sIndex];
		// PrintLine("Looking at \"%.*s\" -> \"%.*s\" \"%.*s\"", StrPrint(shaderInfo->glslPath), StrPrint(shaderInfo->headerPath), StrPrint(shaderInfo->sourcePath));
		
		// +==============================+
		// |       Generate .h File       |
		// +==============================+
		if (forceRebuild || !DoesFileExist(ResolveRootTo(shaderInfo->headerPath, StrLit(".."))))
		{
			PrintLine("Cross-Compiling %.*s to %.*s...", StrPrint(shaderInfo->glslPath), StrPrint(shaderInfo->headerPath));
			
			StrArray targetLanguages = EMPTY;
			AddStrLit(&targetLanguages, "glsl430");
			AddStrLit(&targetLanguages, "glsl310es");
			if (BUILDING_ON_WINDOWS) { AddStrLit(&targetLanguages, "hlsl5"); }
			if (BUILDING_ON_OSX) { AddStrLit(&targetLanguages, "metal_macos"); }
			Str targetLanguagesStr = JoinStrArray(&targetLanguages, StrLit(":"), false);
			
			CliArgs cmd = EMPTY;
			AddArgNt(&cmd, SHDC_FORMAT, "sokol_impl");
			AddArgNt(&cmd, SHDC_ERROR_FORMAT, "msvc");
			// AddArg(&cmd, SHDC_REFLECTION);
			AddArgStr(&cmd, SHDC_SHADER_LANGUAGES, targetLanguagesStr);
			AddArgStr(&cmd, SHDC_INPUT, shaderInfo->glslPath);
			AddArgStr(&cmd, SHDC_OUTPUT, shaderInfo->headerPath);
			
			Str shdcExe = JoinPaths(ResolveRootTo(pigCoreFolder, StrLit("..")), StrLit(EXE_SHDC));
			FixPathSlashes(shdcExe, PATH_SEP_CHAR);
			RunCliProgramAndExitOnFailure(shdcExe, &cmd, FormatStr(EXE_SHDC_NAME " failed to generate C header for %.*s with target languages %.*s!", StrPrint(shaderInfo->glslPath), StrPrint(targetLanguagesStr)));
			AssertFileExist(ResolveRootTo(shaderInfo->headerPath, StrLit("..")), true);
			
			ScrapeShaderHeaderFileAndAddExtraInfo(ResolveRootTo(shaderInfo->headerPath, StrLit("..")), ResolveRootTo(shaderInfo->glslPath, StrLit("..")));
		}
		
		// +==============================+
		// |       Generate .c File       |
		// +==============================+
		if (forceRebuild || !DoesFileExist(ResolveRootTo(shaderInfo->sourcePath, StrLit(".."))))
		{
			PrintLine("Creating %.*s...", StrPrint(shaderInfo->sourcePath));
			
			Str headerFileName = GetFileNamePart(shaderInfo->headerPath, true);
			Str sourceFileContents = FormatStr(
				"\n"
				"#include \"shader_include.h\"\n"
				"\n"
				"#include \"%.*s\"\n",
				StrPrint(headerFileName)
			);
			CreateAndWriteFile(ResolveRootTo(shaderInfo->sourcePath, StrLit("..")), sourceFileContents, true);
		}
		
		// +==============================+
		// | Compile .c files to .obj/.o  |
		// +==============================+
		if (buildForThisPlatform && (forceRebuild || !DoesFileExist(ResolveRootTo(shaderInfo->objPath, StrLit("..")))))
		{
			PrintLine("Building %.*s for %s...", StrPrint(shaderInfo->glslPath), BUILDING_ON_NAME);
			//TODO: Implement me!
			
			if (BUILDING_ON_WINDOWS)
			{
				CliArgs cmd = EMPTY;
				AddArg(&cmd, CL_COMPILE);
				AddArgStr(&cmd, CLI_QUOTED_ARG, shaderInfo->sourcePath);
				AddArgStr(&cmd, CL_OBJ_FILE, shaderInfo->objPath);
				AddIncludeDirArgStr(&cmd, GetDirectoryPart(shaderInfo->sourcePath, true));
				if (compilerArgs != nullptr) { AddArgList(&cmd, compilerArgs); }
				
				StrArray tags = EMPTY;
				if (compileTags != nullptr) { AddStrArray(&tags, compileTags); }
				AddTag(&tags, T_MSVC_CL);
				AddTag(&tags, T_WINDOWS);
				AddTag(&tags, T_LANG_C);
				AddTag(&tags, T_OBJECT);
				
				RunCliProgramAndExitOnFailureTags(StrLit(EXE_MSVC_CL), tags, &cmd, FormatStr("Failed to build %.*s for Windows!", StrPrint(shaderInfo->sourcePath)));
				AssertFileExist(ResolveRootTo(shaderInfo->objPath, StrLit("..")), true);
			}
			if (BUILDING_ON_LINUX)
			{
				AssertMsg(false, "Unimplemented"); //TODO: Implement me!
			}
			if (BUILDING_ON_OSX)
			{
				AssertMsg(false, "Unimplemented"); //TODO: Implement me!
			}
		}
		if (crossCompileForLinuxWithWsl && (forceRebuild || !DoesFileExist(ResolveRootTo(shaderInfo->linuxObjPath, StrLit("..")))))
		{
			PrintLine("Building %.*s for Linux via WSL...", StrPrint(shaderInfo->glslPath));
			AssertMsg(false, "Unimplemented"); //TODO: Implement me!
		}
		if (buildForAndroid && (forceRebuild || !DoesFileExist(ResolveRootTo(shaderInfo->androidObjPaths[AndroidTargetArchitecture_Arm8], StrLit("..")))))
		{
			PrintLine("Building %.*s for Android...", StrPrint(shaderInfo->glslPath));
			
			for (u64 archIndex = 1; archIndex < AndroidTargetArchitecture_Count; archIndex++)
			{
				AndroidTargetArchitecture architecture = (AndroidTargetArchitecture)archIndex;
				Str archFolderName = MakeStrNt(GetAndroidTargetArchitectureFolderName(architecture));
				
				Str objPath = shaderInfo->androidObjPaths[archIndex];
				Str objDir = ResolveRootTo(GetDirectoryPart(objPath, false), StrLit(".."));
				Str oldWorkingDir = GetFullPath(StrLit("."), '/');
				MyCreateFolder(objDir, true);
				chdir(objDir.chars);
				
				CliArgs cmd = EMPTY;
				cmd.pathSepChar = '/';
				cmd.rootDirPath = StrLit("../../../..");
				AddArg(&cmd, CLANG_COMPILE);
				AddArgStr(&cmd, CLI_QUOTED_ARG, shaderInfo->sourcePath);
				AddArgStr(&cmd, CLANG_OUTPUT_FILE, objPath);
				AddIncludeDirArgStr(&cmd, GetDirectoryPart(shaderInfo->sourcePath, true));
				AddArgNt(&cmd, CLANG_TARGET_ARCHITECTURE, GetAndroidTargetArchitectureTargetStr(architecture));
				if (compilerArgs != nullptr) { AddArgList(&cmd, compilerArgs); }
				
				StrArray tags = EMPTY;
				if (compileTags != nullptr) { AddStrArray(&tags, compileTags); }
				AddTag(&tags, T_CLANG);
				AddTag(&tags, T_ANDROID);
				AddTag(&tags, T_LANG_C);
				AddTag(&tags, T_OBJECT);
				AddStrNt(&tags, GetAndroidTargetArchitectureTag(architecture));
				
				RunCliProgramAndExitOnFailureTags(StrLit(EXE_CLANG), tags, &cmd, FormatStr("Failed to build %.*s for Android (arch=%s)", StrPrint(objPath), GetAndroidTargetArchitectureStr(architecture)));
				AssertFileExist(ResolveRootTo(objPath, StrLit("../../../..")), true);
				
				chdir(oldWorkingDir.chars);
			}
		}
		if (buildForWeb && (forceRebuild || !DoesFileExist(ResolveRootTo(shaderInfo->webObjPath, StrLit("..")))))
		{
			PrintLine("Building %.*s for Web...", StrPrint(shaderInfo->glslPath));
			AssertMsg(false, "Unimplemented"); //TODO: Implement me!
		}
		if (buildForWebEmscripten && (forceRebuild || !DoesFileExist(ResolveRootTo(shaderInfo->webEmscriptenObjPath, StrLit("..")))))
		{
			PrintLine("Building %.*s for Web (Emscripten)...", StrPrint(shaderInfo->glslPath));
			AssertMsg(false, "Unimplemented"); //TODO: Implement me!
		}
		if (buildForOrca && (forceRebuild || !DoesFileExist(ResolveRootTo(shaderInfo->orcaObjPath, StrLit("..")))))
		{
			PrintLine("Building %.*s for Orca...", StrPrint(shaderInfo->glslPath));
			AssertMsg(false, "Unimplemented"); //TODO: Implement me!
		}
		
		// +======================================+
		// | Add Objects to linkerArgs with Tags  |
		// +======================================+
		if (DoesFileExist(ResolveRootTo(shaderInfo->objPath, StrLit(".."))) && buildForThisPlatform)
		{
			AddTaggedArgStr(linkerArgs, T_SHADER_OBJS, CLI_QUOTED_ARG, shaderInfo->objPath);
		}
		if (DoesFileExist(ResolveRootTo(shaderInfo->linuxObjPath, StrLit(".."))) && crossCompileForLinuxWithWsl)
		{
			AddTaggedArgStr(linkerArgs, T_SHADER_OBJS T_LINUX, CLI_QUOTED_ARG, shaderInfo->linuxObjPath);
		}
		if (buildForAndroid)
		{
			for (u8 archIndex = 1; archIndex < AndroidTargetArchitecture_Count; archIndex++)
			{
				AndroidTargetArchitecture architecture = (AndroidTargetArchitecture)archIndex;
				if (DoesFileExist(ResolveRootTo(shaderInfo->androidObjPaths[archIndex], StrLit(".."))))
				{
					StrArray tags = EMPTY;
					AddTag(&tags, T_SHADER_OBJS);
					AddTag(&tags, T_ANDROID);
					AddTag(&tags, GetAndroidTargetArchitectureTag(architecture));
					AddTaggedArgStr(linkerArgs, JoinStrArray(&tags, StrLit("|"), false).chars, CLI_QUOTED_ARG, shaderInfo->androidObjPaths[archIndex]);
				}
			}
		}
		//TODO: Add support for buildForWeb
		//TODO: Add support for buildForWebEmscripten
		//TODO: Add support for buildForOrca
	}
	
	return result;
}

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
	LOAD_CONFIG(REBUILD_SHADERS);
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
	
	MyCreateFolder(StrLit("gen"), false);
	
	// +--------------------------------------------------------------+
	// |                        Zip Resources                         |
	// +--------------------------------------------------------------+
	if (ZIP_RESOURCES_FOR_EMBEDDING)
	{
		
		BundleResourcesZip(
			StrLit("../resources"),
			StrLit("app_resources.zip"),
			StrLit("gen/app_resources_zip.h"),
			StrLit("gen/app_resources_zip.c"),
			StrLit("app_resources_zip_bytes")
		);
	}
	
	// +--------------------------------------------------------------+
	// |                       Compile Shaders                        |
	// +--------------------------------------------------------------+
	Array_ShaderInfo shaders = CrossCompileShadersInFolderWithShdc(
		StrLit("[ROOT]/core"),
		StrLit("[ROOT]/src"),
		StrLit("[ROOT]/build/gen"),
		ShaderTargetPlatform_Android,
		REBUILD_SHADERS,
		&buildConfigTags,
		&commonCompilerArgs,
		&commonLinkerArgs //shader objects are added to linker args here
	);
	
	// +--------------------------------------------------------------+
	// |                        Compile to .so                        |
	// +--------------------------------------------------------------+
	// Compile the program to .so that will get embedded into the .apk under `/lib/[arch]/PROJECT_SO_NAME'
	{
		CliArgs args = EMPTY;
		AddArgNt(&args, CLI_QUOTED_ARG, "[ROOT]/src/app_main.c");
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
		AddTag(&tags, T_SHADER_OBJS);
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