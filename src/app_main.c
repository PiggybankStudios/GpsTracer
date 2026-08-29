/*
File:   app_main.c
Author: Taylor Robbins
Date:   08\24\2026
Description: 
	** This is the only file that gets compiled. All other source files are #included from this one
*/

// +--------------------------------------------------------------+
// |                       PigCore Includes                       |
// +--------------------------------------------------------------+
#define PIG_CORE_IMPLEMENTATION 1
#include "base/base_all.h"
#include "file_fmt/file_fmt_all.h"
#include "gfx/gfx_all.h"
#include "gfx/gfx_system_global.h" //Adds GfxSystem gfx global, and simpler API where we don't need to pass gfx as first parameter to every GfxSystem function
#include "input/input_all.h"
#include "lib/lib_all.h"
#include "mem/mem_all.h"
#include "misc/misc_all.h"
#include "os/os_all.h"
#include "parse/parse_all.h"
#include "phys/phys_all.h"
#include "std/std_all.h"
#include "struct/struct_all.h"
#include "ui/ui_all.h"

#include "lib/lib_sokol_app_impl.c"

#if !TARGET_IS_ANDROID
#error This project is only meant to compile on Android!
#endif

// +--------------------------------------------------------------+
// |                           Headers                            |
// +--------------------------------------------------------------+
#include "main2d_shader.glsl.h"
#include "app_defines.h"
#include "app_resources.h"
#include "app_input.h"
#include "app_state.h"

// +--------------------------------------------------------------+
// |                           Globals                            |
// +--------------------------------------------------------------+
AppState appStruct = ZEROED;
AppState* app = nullptr;

Arena* stdHeap = nullptr;
Arena* untrackedStdHeap = nullptr;

AppInput* appIn = nullptr;
u64 ProgramTime = 0;
r32 ElapsedMs = 0.0f;
r32 TimeScale = 0.0f;
v2i ScreenSizei = V2i_Zero_Const;
v2 ScreenSize = V2_Zero_Const;

// +--------------------------------------------------------------+
// |                         Source Files                         |
// +--------------------------------------------------------------+
#include "app_resources.c"
#include "app_input.c"

// +--------------------------------------------------------------+
// |                App Initialization and Cleanup                |
// +--------------------------------------------------------------+
void AppInit(void)
{
	TracyCZoneN(Zone_Func, "AppInit", true);
	
	MainThreadId = OsGetCurrentThreadId();
	OsSetThreadName(nullptr, StrLit("MainThread"));
	
	InitScratchArenasVirtual(SCRATCH_ARENAS_SIZE);
	
	WriteLine_O("+==============================+");
	WriteLine_O("|          " PROJECT_READABLE_NAME_STR "          |");
	WriteLine_O("+==============================+");
	#if COMPILER_IS_MSVC
	WriteLine_N("Compiled by MSVC");
	#elif COMPILER_IS_CLANG
	WriteLine_N("Compiled by Clang");
	#elif COMPILER_IS_GCC
	WriteLine_N("Compiled by GCC");
	#endif
	
	sg_desc gfxDesc = ZEROED;
	// gfxDesc.buffer_pool_size = ?; //int
	// gfxDesc.image_pool_size = ?; //int
	// gfxDesc.sampler_pool_size = ?; //int
	// gfxDesc.shader_pool_size = ?; //int
	// gfxDesc.pipeline_pool_size = ?; //int
	// gfxDesc.attachments_pool_size = ?; //int
	// gfxDesc.uniform_buffer_size = ?; //int
	// gfxDesc.max_commit_listeners = ?; //int
	// gfxDesc.disable_validation = ?; //bool    // disable validation layer even in debug mode, useful for tests
	// gfxDesc.d3d11_shader_debugging = ?; //bool    // if true, HLSL shaders are compiled with D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
	// gfxDesc.mtl_force_managed_storage_mode = ?; //bool // for debugging: use Metal managed storage mode for resources even with UMA
	// gfxDesc.mtl_use_command_buffer_with_retained_references = ?; //bool    // Metal: use a managed MTLCommandBuffer which ref-counts used resources
	// gfxDesc.wgpu_disable_bindgroups_cache = ?; //bool  // set to true to disable the WebGPU backend BindGroup cache
	// gfxDesc.wgpu_bindgroups_cache_size = ?; //int      // number of slots in the WebGPU bindgroup cache (must be 2^N)
	// gfxDesc.allocator = ?; //sg_allocator TODO: Fill this out!
	gfxDesc.environment = GetSokolGfxEnvironment();
	gfxDesc.logger.func = SokolLogCallback;
	InitSokolGraphics(gfxDesc);
	InitGfxSystem(stdHeap, &gfx);
	
	InitRandomSeries(&app->random, RandomSeriesType_LinearCongruential64);
	u64 randomSeed = OsGetCurrentTimestamp(false);
	PrintLine_D("Random seed: %llu", randomSeed);
	SeedRandomSeriesU64(&app->random, randomSeed);
	
	InitAppResources(&app->resources, stdHeap);
	
	InitCompiledShader(&app->mainShader, stdHeap, main2d);
	
	OsMarkStartTime();
	app->appInitFinished = true;
	TracyCZoneEnd(Zone_Func);
}

// +==============================+
// |         App Cleanup          |
// +==============================+
void AppCleanup(void)
{
	TracyCZoneN(Zone_Func, "AppCleanup", true);
	
	//TODO: Implement me!
	
	TracyCZoneEnd(Zone_Func);
}

// +--------------------------------------------------------------+
// |                          App Update                          |
// +--------------------------------------------------------------+
bool AppUpdate(void)
{
	TracyCZoneN(Zone_Func, "AppUpdate", true);
	
	TracyCZoneN(Zone_Update, "Update", true);
	{
		// +==============================+
		// |        Swap AppInputs        |
		// +==============================+
		{
			FreeAppInputAllocations(app->currentAppInput);
			MyMemCopy(app->currentAppInput, app->upcomingAppInput, sizeof(AppInput));
			SwapValues(AppInput*, app->currentAppInput, app->upcomingAppInput);
			RefreshAppInput(app->upcomingAppInput);
			PrepareAppInputForFrame(app->currentAppInput);
			appIn = app->currentAppInput;
			ProgramTime = appIn->programTime;
			ElapsedMs = appIn->elapsedMs;
			TimeScale = appIn->timeScale;
			ScreenSizei = appIn->screenSizei;
			ScreenSize = appIn->screenSize;
		}
	}
	TracyCZoneEnd(Zone_Update);
	
	TracyCZoneN(Zone_Render, "Render", true);
	{
		TracyCZoneN(Zone_BeginFrame, "BeginFrame", true);
		BeginFrame(GetSokolGfxSwapchain(), ScreenSizei, MonokaiBack, /*clearDepth*/1.0f);
		TracyCZoneEnd(Zone_BeginFrame);
		
		BindShader(&app->mainShader);
		// ClearDepthBuffer(1.0f);
		SetDepth(1.0f);
		mat4 projMat = Mat4_Identity;
		TransformMat4(&projMat, MakeScaleXYZMat4(1.0f/(ScreenSize.width/2.0f), 1.0f/(ScreenSize.height/2.0f), 1.0f));
		TransformMat4(&projMat, MakeTranslateXYZMat4(-1.0f, -1.0f, 0.0f));
		TransformMat4(&projMat, MakeScaleYMat4(-1.0f));
		SetProjectionMat(projMat);
		SetViewMat(Mat4_Identity);
		
		// TracyCZoneN(Zone_FontTextureUpdates, "FontTextureUpdates", true);
		// CommitAllFontTextureUpdates(&app->uiFont);
		// TracyCZoneEnd(Zone_FontTextureUpdates);
		
		TracyCZoneN(Zone_EndFrame, "EndFrame", true);
		EndFrame();
		TracyCZoneEnd(Zone_EndFrame);
	}
	TracyCZoneEnd(Zone_Render);
	
	TracyCZoneEnd(Zone_Func);
	return true; //TODO: If we ever conditionally render based on new input then we can pass false here when we didn't render
}

// +--------------------------------------------------------------+
// |                      App Event Handling                      |
// +--------------------------------------------------------------+
void AppHandleEvent(const sapp_event* event)
{
	TracyCZoneN(Zone_Func, "AppHandleEvent", true);
	HandleSokolAppInputEvent(event);
	TracyCZoneEnd(Zone_Func);
}

// +--------------------------------------------------------------+
// |                        SokolApp Main                         |
// +--------------------------------------------------------------+
void* SokolAppAllocator_AllocFunc(size_t numBytes, void* userData)
{
	return AllocMem((Arena*)userData, (uxx)numBytes);
}
void SokolAppAllocator_FreeFunc(void* allocPntr, void* userData)
{
	return FreeMem((Arena*)userData, allocPntr, /*numBytes*/0);
}
sapp_desc sokol_main(int argc, char* argv[])
{
	#if PROFILING_ENABLED
	Str8 projectName = StrLit(PROJECT_READABLE_NAME_STR);
	TracyCAppInfo(projectName.chars, projectName.length);
	#endif
	TracyCZoneN(Zone_Func, "sokol_main", true);
	
	//NOTE: On Android this function is called on a **different thread** from the rest of the callbacks!
	//      So we should be careful which bits of state we initialize here.
	OsSetThreadName(nullptr, StrLit("NativeAndroidActivityThread"));
	
	InitScratchArenasVirtual(Kilobytes(16)); //small scratch arenas for possible debug output on this thread
	InitDebugOutputRouter(); //initialize the debug output mutex as early as possible
	
	app = &appStruct;
	stdHeap = &app->stdHeapStruct;
	untrackedStdHeap = &app->untrackedStdHeapStruct;
	InitArenaStdHeap(stdHeap);
	InitArenaStdHeap(untrackedStdHeap);
	FlagSet(untrackedStdHeap->flags, ArenaFlag_AllowFreeWithoutSize);
	FlagSet(untrackedStdHeap->flags, ArenaFlag_AllowNullptrFree);
	
	OsMarkStartTime(); //NOTE: This is reset at the end of AppInit as well!
	
	app->currentAppInput = &app->appInputsArray[0];
	app->upcomingAppInput = &app->appInputsArray[1];
	appIn = app->currentAppInput;
	
	sapp_desc appDesc = ZEROED;
	// appDesc.user_data  = nullptr; //NOTE: If we fill this, we could then use: init_userdata_cb, frame_userdata_cb, cleanup_userdata_cb, event_userdata_cb
	appDesc.init_cb    = AppInit;
	appDesc.frame_cb   = AppUpdate;
	appDesc.cleanup_cb = AppCleanup;
	appDesc.event_cb   = AppHandleEvent;
	// appDesc.width = 640; //TODO: I don't think these matter on Android?
	// appDesc.height = 480; //TODO: I don't think these matter on Android?
	appDesc.sample_count = 2; //MSAA sample count, TODO: Does this work on Android?
	appDesc.swap_interval = 1; //TODO: 16ms aka 60fps? Is this ignored on Android?
	appDesc.high_dpi = true; //TODO: Does this matter on Android?
	appDesc.fullscreen = true;
	appDesc.alpha = false;
	appDesc.window_title = PROJECT_READABLE_NAME_STR;
	appDesc.enable_clipboard = true;
	appDesc.clipboard_size = Kilobytes(64);
	appDesc.enable_dragndrop = false;
	appDesc.icon.sokol_default = true; //TODO: Change this to false
	// appDesc.icon.images[0] = ?; //TODO: Load our icon from resources!
	appDesc.allocator.user_data = (void*)untrackedStdHeap; //We have to used the "untracked" std heap because free_fn does not pass allocation size
	appDesc.allocator.alloc_fn = SokolAppAllocator_AllocFunc;
	appDesc.allocator.free_fn = SokolAppAllocator_FreeFunc;
	appDesc.logger.user_data = nullptr;
	appDesc.logger.func = SokolLogCallback;
	appDesc.enable_touch_input = true;
	
	TracyCZoneEnd(Zone_Func);
	return appDesc;
}