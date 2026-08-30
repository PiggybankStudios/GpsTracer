/*
File:   app_helpers.c
Author: Taylor Robbins
Date:   08\29\2026
Description: 
	** Holds various functions that have an Application-wide scope but live
	** in here to try and reduce clutter in app_main.c
*/

void AppLoadFonts()
{
	r32 dpiScale = GetScreenDpiScale(nullptr);
	r32 fontScale = GetAndroidFontScale();
	app->fontBakeScale = dpiScale * fontScale;
	Str8 uiFontName = StrLit(UI_FONT_NAME);
	r32 uiFontLargeSize = UI_FONT_LARGE_SIZE * app->fontBakeScale;
	r32 uiFontSmallSize = UI_FONT_SMALL_SIZE * app->fontBakeScale;
	u8 styleNone = FontStyleFlag_None;
	u8 styleBold = FontStyleFlag_Bold;
	u8 styleItalic = FontStyleFlag_Italic;
	u8 styleBoldItalic = FontStyleFlag_Bold|FontStyleFlag_Italic;
	app->uiFont = InitFont(stdHeap, StrLit("UiFont"));
	FontBakeSettings fontBakes[] = {
		//Large Font
		{ .name=uiFontName, .size=uiFontLargeSize, .style=styleNone,       .fillKerningTable=true },
		{ .name=uiFontName, .size=uiFontLargeSize, .style=styleBold,       .fillKerningTable=true },
		{ .name=uiFontName, .size=uiFontLargeSize, .style=styleItalic,     .fillKerningTable=true },
		{ .name=uiFontName, .size=uiFontLargeSize, .style=styleBoldItalic, .fillKerningTable=true },
		//Small Font
		{ .name=uiFontName, .size=uiFontSmallSize, .style=styleNone,       .fillKerningTable=true },
		{ .name=uiFontName, .size=uiFontSmallSize, .style=styleBold,       .fillKerningTable=true },
		{ .name=uiFontName, .size=uiFontSmallSize, .style=styleItalic,     .fillKerningTable=true },
		{ .name=uiFontName, .size=uiFontSmallSize, .style=styleBoldItalic, .fillKerningTable=true },
	};
	FontCharRange charRanges[] = {
		FontCharRange_ASCII,
		// FontCharRange_LatinSupplementAccent,
		// FontCharRange_LatinExtA,
		// FontCharRange_Cyrillic,
		// FontCharRange_Hiragana, FontCharRange_Katakana,
	};
	Result bakeResult = TryAttachAndMultiBakeFontAtlases(
		&app->uiFont,
		ArrayCount(fontBakes), &fontBakes[0],
		/*minAtlasSize*/128, /*maxAtlasSize*/2048,
		ArrayCount(charRanges), &charRanges[0]
	);
	AssertFmt(bakeResult == Result_Success || bakeResult == Result_Partial, "Failed to create uiFont (loading system font \"%.*s\")! Error=%s", StrPrint(uiFontName), GetResultStr(bakeResult));
}