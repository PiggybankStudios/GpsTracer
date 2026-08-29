/*
File:   app_resources.c
Author: Taylor Robbins
Date:   08\29\2026
Description: 
	** Provides a file access API that will route to either the
	** embedded resources zip (generated at compile time as generated
	** C source code containing a u8 array) or to the resources
	** bundled into the .apk (and extracted\cached to disk)
*/

#if USE_EMBEDDED_RESOURCES_ZIP
#include "app_resources_zip.c"
#endif

void InitAppResources(AppResources* resources, Arena* arena)
{
	NotNull(resources);
	ClearPointer(resources);
	resources->usingEmbeddedZip = USE_EMBEDDED_RESOURCES_ZIP;
	#if USE_EMBEDDED_RESOURCES_ZIP
	Slice zipFileContents = MakeSlice(ArrayCount(app_resources_zip_bytes), &app_resources_zip_bytes[0]);
	Result openResult = OpenZipArchive(stdHeap, zipFileContents, &resources->zipFile);
	if (openResult != Result_Success) { NotifyPrint_E("Failed to parse builtin zip file %llu bytes as zip archive: %s", zipFileContents.length, GetResultStr(openResult)); }
	Assert(openResult == Result_Success);
	#else
	//TODO: Should we extract all of the resources now?
	#endif
}

