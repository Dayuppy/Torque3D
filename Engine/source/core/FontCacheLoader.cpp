//-----------------------------------------------------------------------------
// FontCacheLoader.cpp
//-----------------------------------------------------------------------------

#include "FontCacheLoader.h"

#include "core/util/path.h"
#include "console/console.h"
#include "gfx/gFont.h"
#include "platform/platform.h"
#include "core/strings/stringUnit.h"
#include "core/util/tVector.h"
#include "core/stream/fileStream.h"
#include "console/engineAPI.h"
#include "module/moduleManager.h"
#include "core/resourceManager.h"
#include "assets/assetManager.h"
#include "platform/nativeDialogs/fileDialog.h" // for Torque::FS
#include "core/util/str.h"                 // for dStr* functions

// Global font cache map
Map<String, Resource<GFont>> gFontCache;

/// Helper to build a cache key
static String buildFontKey(const String& name, U32 size)
{
    return name + ":" + String::ToString(size);
}

void loadAllFonts()
{
    String game = Con::getVariable("$appName");
    String fontsDirStr = "data/" + game + "/fonts";
    Torque::Path fontsDir(fontsDirStr);

    // Mount the source font directory
    if (!Torque::FS::IsDirectory(fontsDir))
    {
        Con::warnf("[FontCache] Font directory not found: %s", fontsDir.getFullPath().c_str());
        return;
    }

    if (!Torque::FS::Mount("fonts", fontsDir))
    {
        Con::warnf("[FontCache] Failed to mount font path: %s", fontsDir.getFullPath().c_str());
        return;
    }

    // Ensure data/cache/fonts exists
    String cacheBase = "data/cache/fonts";
    Torque::Path cacheBasePath(cacheBase);

    String fontsSubFolder = "fonts";
    String fullCachePathStr = cacheBasePath.getFullPath() + "/" + fontsSubFolder;
    Torque::Path fullCachePath(fullCachePathStr);

    if (!Torque::FS::IsDirectory(fullCachePath))
    {
        Con::printf("[FontCache] Creating cache folder: %s", fullCachePath.getFullPath().c_str());
        Torque::FS::CreatePath(fullCachePath);
    }

    if (!Torque::FS::Mount("fontcache", fullCachePath))
    {
        Con::warnf("[FontCache] Failed to mount font cache path: %s", fullCachePath.getFullPath().c_str());
        return;
    }

    // Verify mount target
    {
        FileStream test;
        if (test.open("fontcache:/_test.txt", Torque::FS::File::Write))
        {
            test.writeLine((const U8*)"OK");
            test.close();
            Con::printf("[FontCache] Verified write access to fontcache:/");
        }
        else
        {
            Con::warnf("[FontCache] Cannot write to fontcache:/ mount.");
            return;
        }
    }

    // Scan fonts
    Torque::Path virtualFontPath("fonts:/");
    Con::printf("[FontCache] Looking for fonts in: %s", virtualFontPath.getFullPath().c_str());

    Vector<String> fontFiles;
    S32 found = Torque::FS::FindByPattern(virtualFontPath, "*.ttf", true, fontFiles);

    Con::printf("[FontCache] Found %d .ttf files in %s", found, fontsDir.getFullPath().c_str());

    if (fontFiles.empty())
    {
        Con::warnf("[FontCache] No TTF files found. Aborting.");
        return;
    }

    // Process each font
    for (const String& path : fontFiles)
    {
        Torque::Path fontPath(path);
        String fontName = fontPath.getFileName(); // e.g., "Roboto-Regular"

        Con::printf("  [FontCache] Processing font: %s", fontName.c_str());

        for (U32 size : {12, 14, 16, 18, 20, 24, 28, 32})
        {
            String gftName = String::ToString("%s_%u.font.gft", fontName.c_str(), size);
            String gftVirtualPath = "fontcache:/" + gftName;

            if (!Torque::FS::IsFile(gftVirtualPath))
            {
                Con::printf("    [FontCache] Generating .gft: %s", gftVirtualPath.c_str());

                Resource<GFont> genFont = GFont::create(path.c_str(), size);
                if (!genFont)
                {
                    Con::warnf("    [FontCache] Failed to generate font: %s @ %u", fontName.c_str(), size);
                    continue;
                }

                FileStream fs;
                if (fs.open(gftVirtualPath.c_str(), Torque::FS::File::Write))
                {
                    genFont->writeToStream(&fs);
                    fs.close();
                    Con::printf("    [FontCache] Saved .gft: %s", gftVirtualPath.c_str());
                }
                else
                {
                    Con::warnf("    [FontCache] Could not write .gft to: %s", gftVirtualPath.c_str());
                    continue;
                }
            }

            // Load using just the fontName (as GFont auto-detects .gft)
            Resource<GFont> font = GFont::create(fontName.c_str(), size);
            if (font)
            {
                String key = buildFontKey(fontName, size);
                gFontCache.insert(key, font);
                Con::printf("    [FontCache] Cached: %s @ %u", fontName.c_str(), size);
            }
            else
            {
                Con::warnf("    [FontCache] Failed to load cached .gft: %s @ %u", fontName.c_str(), size);
            }
        }
    }

    Con::printf("[FontCache] Finished loading %u fonts.", gFontCache.size());
}



GFont* getCachedFont(const String& fontName, U32 size)
{
    String key = buildFontKey(fontName, size);
    if (gFontCache.contains(key))
        return gFontCache[key];
    return nullptr;
}

DefineEngineFunction(loadAllFonts, void, (), ,
    "Scan the fonts directory for TTF files and cache them by name and size.")
{
    loadAllFonts();
}

DefineEngineFunction(listCachedFonts, void, (), ,
    "Print a list of cached fonts and sizes to the console.")
{
    if (gFontCache.isEmpty())
    {
        Con::printf("[FontCache] No fonts cached.");
        return;
    }

    Con::printf("[FontCache] Listing %u cached fonts:", gFontCache.size());

    for (Map<String, Resource<GFont>>::Iterator it = gFontCache.begin(); it != gFontCache.end(); ++it)
    {
        Con::printf(" - %s", it->key.c_str());
    }
}
