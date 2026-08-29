/*
File:   app_resources.h
Author: Taylor Robbins
Date:   08\29\2026
*/

#ifndef _APP_RESOURCES_H
#define _APP_RESOURCES_H

#include "app_resources_zip.h"

typedef struct AppResources AppResources;
struct AppResources
{
	bool usingEmbeddedZip;
	#if USE_EMBEDDED_RESOURCES_ZIP
	ZipArchive zipFile;
	#endif
};

#endif //  _APP_RESOURCES_H
