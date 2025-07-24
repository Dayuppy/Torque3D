#ifndef _FONT_CACHE_LOADER_H_
#define _FONT_CACHE_LOADER_H_

#include "core/util/tVector.h"
#include "gfx/gFont.h"
#include "core/strings/stringFunctions.h"

// Load all fonts in the "fonts/" directory and cache them by name/size.
void loadAllFonts();

// Retrieve a cached font by name and size.
GFont* getCachedFont(const String& fontName, U32 size);

#endif
