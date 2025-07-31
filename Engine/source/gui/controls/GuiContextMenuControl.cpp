//-----------------------------------------------------------------------------
// File: GuiContextMenuControl.cpp
// Purpose: Minimal, self-sizing context menu control for Torque 3D, with bitmap
//          background asset support, highlight bitmaps, overlay glow, text frame,
//          and optional sub-menu support.
//-----------------------------------------------------------------------------

#include "GuiContextMenuControl.h"
#include "console/engineAPI.h"
#include "gfx/gfxTextureProfile.h"
#include "gui/core/guiCanvas.h"
#include "console/console.h"
#include "core/util/safeDelete.h"

IMPLEMENT_CONOBJECT(GuiContextMenuControl);
ConsoleDocClass(GuiContextMenuControl,
    "@brief GUI control: minimal context menu popup with bitmap background asset,\n"
    "highlight-bitmap, text-frame, overlay glow, and optional sub-menu support.\n"
    "@ingroup GuiControls"
);

// Provide definitions for the nested entry type to resolve linker errors
GuiContextMenuEntry::GuiContextMenuEntry() {}
GuiContextMenuEntry::~GuiContextMenuEntry() {}

static const char* kSubmenuPrefix = "SubMenu_";

//-----------------------------------------------------------------------------
// Sizing constants
//-----------------------------------------------------------------------------
static const S32 entryHeight = 28;
static const S32 horizontalPadding = 32;
static const S32 verticalPadding = 16;
static const S32 minMenuWidth = 140;
static const S32 maxMenuWidth = 400;
static const S32 minMenuHeight = entryHeight + verticalPadding;
static const S32 maxMenuHeight = 600;

//-----------------------------------------------------------------------------
// Constructor / Destructor
//-----------------------------------------------------------------------------
GuiContextMenuControl::GuiContextMenuControl()
    : mBackgroundBitmapAsset(nullptr),
    mBackgroundBitmap(nullptr),
    mHighlightBitmapAsset(nullptr),
    mHighlightBitmap(nullptr),
    mGlowBitmapAsset(nullptr),
    mGlowBitmap(nullptr),
    mBackgroundColor(50, 50, 50, 240),
    mTextColor(255, 255, 255, 255),
    mHighlightColor(80, 160, 240, 255),
    mTextFrameColor(0, 0, 0, 192),
    mFontSize(18),
    mCustomFont(nullptr),
    mDrawTextFrame(true),
    mTextFramePadding(2),
    mUseHighlightBitmap(false),
    mUseGlow(false),
    mGlowColor(255, 255, 0, 255),
    mGlowThickness(4),
    mVisible(false),
    mHoverIndex(-1),
    mSubMenuPrefix(StringTable->EmptyString())
{
    mVisible = false;
    mHoverIndex = -1;
    mContextObjId = String();
    mFontName = StringTable->insert("Roboto Condensed");
    setExtent(Point2I(minMenuWidth, minMenuHeight));
}

GuiContextMenuControl::~GuiContextMenuControl()
{
    clearEntries();
}

//-----------------------------------------------------------------------------
// Persist fields
//-----------------------------------------------------------------------------
void GuiContextMenuControl::initPersistFields()
{
    Parent::initPersistFields();
    addProtectedField("backgroundBitmapAsset", TypeImageAssetPtr,
        Offset(mBackgroundBitmapAsset, GuiContextMenuControl),
        &_setBackgroundBitmapAsset, &_getBackgroundBitmapAsset,
        "Background image asset ID");

    addProtectedField("highlightBitmapAsset", TypeImageAssetPtr,
        Offset(mHighlightBitmapAsset, GuiContextMenuControl),
        &_setHighlightBitmapAsset, &_getHighlightBitmapAsset,
        "Optional image asset ID for hovered entry");

    addProtectedField("glowBitmapAsset", TypeImageAssetPtr,
        Offset(mGlowBitmapAsset, GuiContextMenuControl),
        &_setGlowBitmapAsset, &_getGlowBitmapAsset,
        "Optional image asset ID for overlay glow");

    addField("backgroundColor", TypeColorI,
        Offset(mBackgroundColor, GuiContextMenuControl),
        "Background fill color");
    addField("textColor", TypeColorI,
        Offset(mTextColor, GuiContextMenuControl),
        "Entry text color");
    addField("highlightColor", TypeColorI,
        Offset(mHighlightColor, GuiContextMenuControl),
        "Fallback highlight color if no highlight bitmap used");
    addField("textFrameColor", TypeColorI,
        Offset(mTextFrameColor, GuiContextMenuControl),
        "Color for text frame overlay");
    addField("fontName", TypeCaseString,
        Offset(mFontName, GuiContextMenuControl),
        "Font face name");
    addField("fontSize", TypeS32,
        Offset(mFontSize, GuiContextMenuControl),
        "Font size");

    addField("drawTextFrame", TypeBool,
        Offset(mDrawTextFrame, GuiContextMenuControl),
        "Enable text frame behind entry text");
    addField("textFramePadding", TypeS32,
        Offset(mTextFramePadding, GuiContextMenuControl),
        "Padding for text frame");

    addField("useHighlightBitmap", TypeBool,
        Offset(mUseHighlightBitmap, GuiContextMenuControl),
        "Use highlight bitmap asset on hover");
    addField("useGlow", TypeBool,
        Offset(mUseGlow, GuiContextMenuControl),
        "Enable overlay glow");
    addField("glowColor", TypeColorI,
        Offset(mGlowColor, GuiContextMenuControl),
        "Overlay glow color");
    addField("glowThickness", TypeS32,
        Offset(mGlowThickness, GuiContextMenuControl),
        "Overlay glow thickness");

    addField("subMenuPrefix", TypeCaseString,
        Offset(mSubMenuPrefix, GuiContextMenuControl),
        "Prefix that marks entries as sub-menu commands");
}

//-----------------------------------------------------------------------------
// Background asset setter/getter & refresh
//-----------------------------------------------------------------------------
bool GuiContextMenuControl::_setBackgroundBitmapAsset(void* obj, const char* idx, const char* data)
{
    GuiContextMenuControl* ctrl = static_cast<GuiContextMenuControl*>(obj);
    if (ctrl->mBackgroundBitmapAsset.notNull())
        ctrl->mBackgroundBitmapAsset->getChangedSignal()
        .remove(ctrl, &GuiContextMenuControl::onBackgroundBitmapAssetRefresh);

    if (!data || !*data)
    {
        ctrl->mBackgroundBitmapAsset.clear();
        ctrl->mBackgroundBitmap = GFXTexHandle();
    }
    else
    {
        ctrl->mBackgroundBitmapAsset = AssetPtr<ImageAsset>(StringTable->insert(data));
        ctrl->mBackgroundBitmapAsset->getChangedSignal()
            .notify(ctrl, &GuiContextMenuControl::onBackgroundBitmapAssetRefresh);
        ctrl->mBackgroundBitmap = ctrl->mBackgroundBitmapAsset
            ->getTexture(&GFXDefaultGUIProfile);
    }
    ctrl->setUpdate();
    return true;
}

const char* GuiContextMenuControl::_getBackgroundBitmapAsset(void* obj, const char*)
{
    GuiContextMenuControl* ctrl = static_cast<GuiContextMenuControl*>(obj);
    return ctrl->mBackgroundBitmapAsset.notNull()
        ? ctrl->mBackgroundBitmapAsset->getAssetId()
        : StringTable->EmptyString();
}

void GuiContextMenuControl::onBackgroundBitmapAssetRefresh()
{
    if (mBackgroundBitmapAsset.notNull() &&
        mBackgroundBitmapAsset->getStatus() == ImageAsset::Ok)
    {
        mBackgroundBitmap = mBackgroundBitmapAsset->getTexture(&GFXDefaultGUIProfile);
    }
    else
    {
        mBackgroundBitmap = GFXTexHandle();
    }
    setUpdate();
}

//-----------------------------------------------------------------------------
// Highlight asset setter/getter & refresh
//-----------------------------------------------------------------------------
bool GuiContextMenuControl::_setHighlightBitmapAsset(void* obj, const char* idx, const char* data)
{
    GuiContextMenuControl* ctrl = static_cast<GuiContextMenuControl*>(obj);
    if (ctrl->mHighlightBitmapAsset.notNull())
        ctrl->mHighlightBitmapAsset->getChangedSignal()
        .remove(ctrl, &GuiContextMenuControl::onHighlightBitmapAssetRefresh);

    if (!data || !*data)
    {
        ctrl->mHighlightBitmapAsset.clear();
        ctrl->mHighlightBitmap = GFXTexHandle();
    }
    else
    {
        ctrl->mHighlightBitmapAsset = AssetPtr<ImageAsset>(StringTable->insert(data));
        ctrl->mHighlightBitmapAsset->getChangedSignal()
            .notify(ctrl, &GuiContextMenuControl::onHighlightBitmapAssetRefresh);
        ctrl->mHighlightBitmap = ctrl->mHighlightBitmapAsset
            ->getTexture(&GFXDefaultGUIProfile);
    }
    ctrl->setUpdate();
    return true;
}

const char* GuiContextMenuControl::_getHighlightBitmapAsset(void* obj, const char*)
{
    GuiContextMenuControl* ctrl = static_cast<GuiContextMenuControl*>(obj);
    return ctrl->mHighlightBitmapAsset.notNull()
        ? ctrl->mHighlightBitmapAsset->getAssetId()
        : StringTable->EmptyString();
}

void GuiContextMenuControl::onHighlightBitmapAssetRefresh()
{
    if (mHighlightBitmapAsset.notNull() &&
        mHighlightBitmapAsset->getStatus() == ImageAsset::Ok)
    {
        mHighlightBitmap = mHighlightBitmapAsset->getTexture(&GFXDefaultGUIProfile);
    }
    else
    {
        mHighlightBitmap = GFXTexHandle();
    }
    setUpdate();
}

//-----------------------------------------------------------------------------
// Glow asset setter/getter & refresh
//-----------------------------------------------------------------------------
bool GuiContextMenuControl::_setGlowBitmapAsset(void* obj, const char* idx, const char* data)
{
    GuiContextMenuControl* ctrl = static_cast<GuiContextMenuControl*>(obj);
    if (ctrl->mGlowBitmapAsset.notNull())
        ctrl->mGlowBitmapAsset->getChangedSignal()
        .remove(ctrl, &GuiContextMenuControl::onGlowBitmapAssetRefresh);

    if (!data || !*data)
    {
        ctrl->mGlowBitmapAsset.clear();
        ctrl->mGlowBitmap = GFXTexHandle();
    }
    else
    {
        ctrl->mGlowBitmapAsset = AssetPtr<ImageAsset>(StringTable->insert(data));
        ctrl->mGlowBitmapAsset->getChangedSignal()
            .notify(ctrl, &GuiContextMenuControl::onGlowBitmapAssetRefresh);
        ctrl->mGlowBitmap = ctrl->mGlowBitmapAsset
            ->getTexture(&GFXDefaultGUIProfile);
    }
    ctrl->setUpdate();
    return true;
}

const char* GuiContextMenuControl::_getGlowBitmapAsset(void* obj, const char*)
{
    GuiContextMenuControl* ctrl = static_cast<GuiContextMenuControl*>(obj);
    return ctrl->mGlowBitmapAsset.notNull()
        ? ctrl->mGlowBitmapAsset->getAssetId()
        : StringTable->EmptyString();
}

void GuiContextMenuControl::onGlowBitmapAssetRefresh()
{
    if (mGlowBitmapAsset.notNull() &&
        mGlowBitmapAsset->getStatus() == ImageAsset::Ok)
    {
        mGlowBitmap = mGlowBitmapAsset->getTexture(&GFXDefaultGUIProfile);
    }
    else
    {
        mGlowBitmap = GFXTexHandle();
    }
    setUpdate();
}

//-----------------------------------------------------------------------------
// Entry management
//-----------------------------------------------------------------------------
void GuiContextMenuControl::clearEntries()
{
    for (auto* e : mEntries) SAFE_DELETE(e);
    mEntries.clear();
}


void GuiContextMenuControl::addEntry(const char* text, const char* command)
{
    GuiContextMenuEntry* e = new GuiContextMenuEntry();
    e->text = text;
    e->command = command;
    mEntries.push_back(e);
}

//-----------------------------------------------------------------------------
// Calculate menu size
//-----------------------------------------------------------------------------
void GuiContextMenuControl::calculateMenuSize(S32& outW, S32& outH)
{
    Resource<GFont> font = mCustomFont
        ? mCustomFont
        : (mProfile ? mProfile->mFont : Resource<GFont>());

    outW = minMenuWidth;
    for (auto* e : mEntries)
    {
        S32 tw = minMenuWidth - horizontalPadding;
        if (font)
        {
            U32 fw = font->getStrWidth(e->text);
            tw = S32(getMin((U32)(maxMenuWidth - horizontalPadding), fw));
        }
        else
        {
            tw = S32(getMin(maxMenuWidth - horizontalPadding,
                (S32)e->text.length() * 10));
        }
        outW = getMax(outW, tw + horizontalPadding);
    }
    outW = getMax(minMenuWidth, getMin(outW, maxMenuWidth));

    outH = (S32)mEntries.size() * entryHeight + verticalPadding;
    outH = getMax(minMenuHeight, getMin(outH, maxMenuHeight));
}

//---------------------------------------------------------------------------- -
// Open the menu
//-----------------------------------------------------------------------------
void GuiContextMenuControl::openMenu(Point2I pos, const char* contextObjId)
{
    // Determine if this call is building a manual submenu
    SimObject* ctxObj = Sim::findObject(contextObjId);
    bool isManual = (ctxObj == this);

    if (!isManual)
    {
        // Store original context for dot‐commands
        mContextObjId = contextObjId ? String(contextObjId) : String();

        // Populate from cmenu_* fields
        clearEntries();
        if (ctxObj && ctxObj->getFieldDictionary())
        {
            for (SimFieldDictionaryIterator it(ctxObj->getFieldDictionary()); *it; ++it)
            {
                auto* f = *it;
                String name = f->slotName;
                if (name.startsWith("cmenu_"))
                    addEntry(name.substr(6), f->value);
            }
        }
    }

    // Fallback
    if (mEntries.empty())
        addEntry("No Actions Available", "");

    // Size & placement
    refreshFont();
    S32 w, h;
    calculateMenuSize(w, h);
    setExtent(Point2I(w, h));

    if (GuiCanvas* canvas = dynamic_cast<GuiCanvas*>(getRoot()))
    {
        Point2I ce = canvas->getExtent();
        pos.x = getMax(0, getMin(pos.x, ce.x - w));
        pos.y = getMax(0, getMin(pos.y, ce.y - h));
        canvas->pushDialogControl(this);
    }

    setPosition(pos);
    mVisible = true;
    mHoverIndex = -1;
    setUpdate();
}

//-----------------------------------------------------------------------------
// Hide menu
//-----------------------------------------------------------------------------
void GuiContextMenuControl::closeMenu()
{
    mVisible = false;
    clearEntries();
    setUpdate();
    if (auto canvas = dynamic_cast<GuiCanvas*>(getRoot()))
        canvas->popDialogControl(this);
}

//-----------------------------------------------------------------------------
// Load or reload font
//-----------------------------------------------------------------------------
void GuiContextMenuControl::refreshFont()
{
    if (*mFontName && mFontSize > 0)
    {
        if (auto f = GFont::create(mFontName, mFontSize))
            mCustomFont = f;
        else
            Con::warnf("GuiContextMenuControl::refreshFont - could not create font '%s' size %d",
                mFontName, mFontSize);
    }
    else
    {
        mCustomFont = NULL;
    }
}

bool GuiContextMenuControl::onAdd()
{
    return Parent::onAdd();
}

bool GuiContextMenuControl::onWake()
{
    if (!Parent::onWake())
        return false;

    refreshFont();

    // Only fetch texture if the asset pointer is valid and in the OK state
    if (mBackgroundBitmapAsset.notNull() &&
        mBackgroundBitmapAsset->getStatus() == ImageAsset::Ok)
    {
        mBackgroundBitmap = mBackgroundBitmapAsset->getTexture(&GFXDefaultGUIProfile);
    }
    else
    {
        mBackgroundBitmap = GFXTexHandle();
    }

    return true;
}

//-----------------------------------------------------------------------------
// Render
//-----------------------------------------------------------------------------
void GuiContextMenuControl::onRender(Point2I offset, const RectI& /*ur*/)
{
    if (!mVisible) return;

    Point2I ext = getExtent();
    RectI bgR(offset, ext);
    GFXDrawUtil* du = GFX->getDrawUtil();
    ColorI prevMod; du->getBitmapModulation(&prevMod);

    // background
    if (mBackgroundBitmap.isValid())
    {
        du->setBitmapModulation(mBackgroundColor);
        du->drawBitmapStretch(mBackgroundBitmap, bgR,
            GFXBitmapFlip_None, GFXTextureFilterLinear, false);
    }
    else
    {
        du->setBitmapModulation(mBackgroundColor);
        du->drawRectFill(bgR, mBackgroundColor);
    }

    // entries
    Resource<GFont> font = mCustomFont
        ? mCustomFont
        : (mProfile ? mProfile->mFont : Resource<GFont>());
    S32 y = offset.y + (verticalPadding / 2);

    for (S32 i = 0; i < (S32)mEntries.size(); ++i)
    {
        RectI er(offset.x + (horizontalPadding / 2), y,
            ext.x - horizontalPadding, entryHeight);

        // highlight area
        if (i == mHoverIndex)
        {
            if (mUseHighlightBitmap && mHighlightBitmap.isValid())
            {
                du->setBitmapModulation(ColorI(255, 255, 255, 255));
                du->drawBitmapStretch(mHighlightBitmap, er,
                    GFXBitmapFlip_None, GFXTextureFilterLinear, false);
            }
            else
            {
                du->setBitmapModulation(mHighlightColor);
                du->drawRectFill(er, mHighlightColor);
            }
        }

        // text frame
        const char* txt = mEntries[i]->text.c_str();
        S32 tw = font ? font->getStrWidth(txt) : (S32)dStrlen(txt) * 8;
        S32 th = font ? font->getHeight() : 12;
        Point2I tp(er.point.x + 4, er.point.y + ((entryHeight - th) / 2));
        if (mDrawTextFrame)
        {
            RectI fr(tp.x - mTextFramePadding, tp.y - mTextFramePadding,
                tw + 2 * mTextFramePadding, th + 2 * mTextFramePadding);
            du->setBitmapModulation(mTextFrameColor);
            du->drawRectFill(fr, mTextFrameColor);
        }

        // text
        du->setBitmapModulation(mTextColor);
        du->drawText(font, tp, txt);
        y += entryHeight;
    }

    // overlay glow
    if (mUseGlow)
    {
        du->setBitmapModulation(mGlowColor);
        if (mGlowBitmap.isValid())
        {
            du->drawBitmapStretch(mGlowBitmap, bgR,
                GFXBitmapFlip_None, GFXTextureFilterLinear, false);
        }
        else
        {
            du->drawRoundedRect((F32)mGlowThickness, bgR,
                ColorI(0, 0, 0, 0),
                (F32)(2 * mGlowThickness),
                mGlowColor);
        }
    }

    du->setBitmapModulation(prevMod);
}

//-----------------------------------------------------------------------------
// Mouse events
//-----------------------------------------------------------------------------
void GuiContextMenuControl::onMouseMove(const GuiEvent& ev)
{
    if (!mVisible)
        return;

    // Compute new hover index
    Point2I local = globalToLocalCoord(ev.mousePoint);
    S32 idx = (local.y - (verticalPadding / 2)) / entryHeight;
    mHoverIndex = (idx >= 0 && idx < (S32)mEntries.size()) ? idx : -1;

    setUpdate();
}

void GuiContextMenuControl::onMouseDown(const GuiEvent& ev)
{
    if (!mVisible)
        return;

    Point2I local = globalToLocalCoord(ev.mousePoint);
    S32 idx = (local.y - (verticalPadding / 2)) / entryHeight;
    if (idx < 0 || idx >= (S32)mEntries.size())
        return;

    GuiContextMenuEntry* e = mEntries[idx];
    String cmd = e->command;

    // Strip any leading semicolons
    while (!cmd.isEmpty() && cmd[0] == ';')
        cmd.erase(0, 1);

    // Prepend context object ID for dot-prefixed commands
    if (!cmd.isEmpty() && cmd[0] == '.')
        cmd = mContextObjId + cmd;

    // Execute the console command or spawn submenu
    if (!cmd.isEmpty())
        evaluate(cmd.c_str());

    // If this command was a submenu function, leave parent open
    if (dStrnicmp(cmd.c_str(), kSubmenuPrefix, dStrlen(kSubmenuPrefix)) == 0)
    {
        // Positioning of the submenu is handled by the script's x/y parameters.
        return;
    }

    // Otherwise, close this menu after action
    if (GuiCanvas* canvas = dynamic_cast<GuiCanvas*>(getRoot()))
        canvas->popDialogControl(this);
    mVisible = false;
}

void GuiContextMenuControl::onRightMouseDown(const GuiEvent&)
{
    closeMenu();
}

void GuiContextMenuControl::onLoseFirstResponder()
{
    closeMenu();
}

//-----------------------------------------------------------------------------
// Engine method binding
//-----------------------------------------------------------------------------
DefineEngineMethod(GuiContextMenuControl, openMenu, void,
    (S32 x, S32 y, const char* contextObjId), ,
    "@brief Open the context menu at (x,y) for the given context object.\n"
    "@param x X screen position\n@param y Y screen position\n@param contextObjId GUI control or object ID.")
{
    object->openMenu(Point2I(x, y), contextObjId);
}

DefineEngineMethod(GuiContextMenuControl, addEntry, void,
    (const char* text, const char* command), ,
    "@brief Add a manual menu entry.\n"
    "@param text Display text\n@param command Console command")
{
    object->addEntry(text, command);
}
