/*
File:   build_script.c
Author: Taylor Robbins
Date:   08\24\2026
Description: 
	** This is the script that holds the logic for building the app.
	** This script is compiled with the build.sh/build.bat which
	** utilized PigBuild to compile and run this script inside a `build/` folder
*/

#include "pig_build.h"

int main(int argc, char* argv[])
{
	PigBuildDebugMode = false;
	RecompileIfNeeded(StrArray_Empty);
	WriteLine("Building...");
	//TODO: Implement me!
}