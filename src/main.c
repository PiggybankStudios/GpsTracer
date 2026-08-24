/*
File:   main.c
Author: Taylor Robbins
Date:   08\24\2026
Description: 
	** This is the only file that gets compiled. All other source files are #included from this one
*/

#include <stdio.h>
#include "base/base_all.h"

#include "app_resources_zip.h"
#if USE_EMBEDDED_RESOURCES_ZIP
#include "app_resources_zip.c"
#endif

int main(int argc, char* argv[])
{
	printf("Hello from Android!\n");
	return 0;
}