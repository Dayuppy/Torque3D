//-----------------------------------------------------------------------------
// GuiRadialMenuCtrl.cpp
// Fully corrected implementation with hierarchical sub‐menus and safe asset handling.
//-----------------------------------------------------------------------------

#include "GuiRadialMenuCtrl.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/core/guiCanvas.h"
#include "console/engineAPI.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "core/strings/stringFunctions.h"
#include "sfx/sfxSystem.h"

IMPLEMENT_CONOBJECT(GuiRadialMenuCtrl);

//------------------------------------------------------------------------------
// Constructor / Destructor (must be defined to satisfy the linker)
//------------------------------------------------------------------------------
GuiRadialMenuCtrl::GuiRadialMenuCtrl()
    : mRadius(324),
    mThickness(176),
    mBaseIconSize(85.0f),
    mHoverScale(1.5f),
    mBackgroundColor(0, 0, 0, 200),
    mHighlightColor(255, 165, 0, 255),
    mHoveredIndex(-1),
    mCurrentIndex(0),
    mHoverSound(nullptr),
    mClickSound(nullptr)
{
    // set up the “Back” entry
    mBackItem.name = StringTable->insert("Back");
    mBackItem.funcName = StringTable->insert("");    // no script
    mBackItem.funcArg = StringTable->insert("");
    mBackItem.children.clear();
    mBackItem.isBackButton = true;
    mBackItem.iconAssetId = StringTable->insert("");
    mBackItem.iconAsset.clear();
    mBackItem.iconTexture = GFXTexHandle();

    // start from root
    clearItems();
    mHistory.clear();
}

GuiRadialMenuCtrl::~GuiRadialMenuCtrl()
{
}

//------------------------------------------------------------------------------
// Persist fields
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::initPersistFields()
{
    addGroup("RadialMenu");

    // Hover sound: an SFXProfile SimObject pointer
    addField("hoverSound",
        TypeSimObjectPtr,
        Offset(mHoverSound, GuiRadialMenuCtrl),
        "@brief SFXProfile to play when hovering.");

    // Click sound: an SFXProfile SimObject pointer
    addField("clickSound",
        TypeSimObjectPtr,
        Offset(mClickSound, GuiRadialMenuCtrl),
        "@brief SFXProfile to play when clicking.");

    endGroup("RadialMenu");

    Parent::initPersistFields();
}

//------------------------------------------------------------------------------
// onAdd
//------------------------------------------------------------------------------
bool GuiRadialMenuCtrl::onAdd()
{
    if (!Parent::onAdd())
        return false;
    // start at root level
    gatherCurrentLevel();
    return true;
}

//------------------------------------------------------------------------------
// Helper: try to acquire an ImageAsset; leave iconTexture invalid on failure
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::safeLoadIcon(Item& item, const char* assetId)
{
    item.iconAsset.clear();
    item.iconTexture = GFXTexHandle();
    item.iconAssetId = StringTable->insert(""); // always valid, even if no icon

    // Defensive: Only proceed if assetId is non-null and not empty
    if (!assetId || !*assetId)
        return; // Do nothing, use placeholder

    StringTableEntry id = StringTable->insert(assetId);

    // Only try to acquire asset if it's declared (registered)
    if (!AssetDatabase.isDeclaredAsset(id))
        return;

    AssetPtr<ImageAsset> tmp(id);

    // Only set up item icon fields if asset is truly valid
    if (tmp.notNull() && tmp->getStatus() == ImageAsset::Ok)
    {
        item.iconAsset = tmp;
        item.iconTexture = tmp->getTexture(&GFXDefaultGUIProfile);
        item.iconAssetId = id;
    }
}

const char* GuiRadialMenuCtrl::_getItemIconAssetId(const Item& item)
{
    if (item.iconAsset.notNull() && item.iconAsset->getStatus() == ImageAsset::Ok)
        return item.iconAsset->getAssetId();
    return StringTable->EmptyString();
}

//------------------------------------------------------------------------------
// Draw a filled wedge by drawing radiating lines from the inner ring to the rim
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::drawWedgeFill(const Point2I& center,
    F32 radius,
    F32 startAng,
    F32 endAng,
    const ColorI& col)
{
    // how many segments we subdivide the arc into
    const F32 step = (endAng - startAng) / (F32)kWedgeSteps;

    // compute inner radius
    const F32 innerR = radius - (F32)mThickness;

    for (F32 ang = startAng; ang <= endAng; ang += step)
    {
        // outer point on the rim
        Point2I pOuter(
            center.x + (S32)(radius * mCos(ang)),
            center.y + (S32)(radius * mSin(ang))
        );
        // inner point on the inner ring
        Point2I pInner(
            center.x + (S32)(innerR * mCos(ang)),
            center.y + (S32)(innerR * mSin(ang))
        );
        // draw only the segment of the wedge between inner and outer
        GFX->getDrawUtil()->drawLine(pInner, pOuter, col);
    }
}

//------------------------------------------------------------------------------
// onRender: backdrop, rings, wedge highlight, icons, center, text
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::onRender(Point2I offset, const RectI& /*updateRect*/)
{
    Point2I center = offset + getExtent() / 2;

    // 1) Outer backdrop with border
    RectI outerRect(center - Point2I(mRadius, mRadius), Point2I(mRadius * 2, mRadius * 2));
    GFX->getDrawUtil()->drawCircleFill(
        outerRect,
        mBackgroundColor,
        (F32)mRadius,
        2.0f,
        mHighlightColor
    );

    // 1b) Inner contrast circle with border
    S32 innerR = mRadius - mThickness;
    RectI innerRect(center - Point2I(innerR, innerR), Point2I(innerR * 2, innerR * 2));
    GFX->getDrawUtil()->drawCircleFill(
        innerRect,
        ColorI(0, 0, 0, 150),
        (F32)innerR,
        2.0f,
        mHighlightColor
    );

    // 2) Wedge highlight
    if (mHoveredIndex >= 0 && !mCurrentItems.empty())
    {
        const F32 slice = (2 * M_PI_F) / mCurrentItems.size();
        const F32 midAng = -M_PI_F / 2 + slice * (mHoveredIndex + 0.5f);
        const F32 half = slice * 0.5f;
        drawWedgeFill(center, mRadius, midAng - half, midAng + half, mHighlightColor);
    }

    // 3) Icons + shadows
    renderItems(center);

    // 4) Enlarged center icon if hovered
    if (mHoveredIndex >= 0 && mHoveredIndex < (S32)mCurrentItems.size())
    {
        Item* sel = mCurrentItems[mHoveredIndex];
        F32 size = mBaseIconSize * mHoverScale * 1.5f;
        RectI r(center - Point2I((S32)(size / 2), (S32)(size / 2)), Point2I((S32)size, (S32)size));

        if (sel->iconTexture.isValid())
        {
            GFX->getDrawUtil()->clearBitmapModulation();
            GFX->getDrawUtil()->setBitmapModulation(ColorI::WHITE);
            GFX->getDrawUtil()->drawBitmapStretch(
                sel->iconTexture,
                r,
                GFXBitmapFlip_None,
                GFXTextureFilterLinear,
                false,
                0.f
            );
        }
        else if (sel->isBackButton)
        {
            // fallback X
            GFX->getDrawUtil()->drawLine(r.point, r.point + r.extent, mHighlightColor);
            GFX->getDrawUtil()->drawLine(
                Point2I(r.point.x + r.extent.x, r.point.y),
                Point2I(r.point.x, r.point.y + r.extent.y),
                mHighlightColor
            );
        }
        else if (!sel->children.empty())
        {
            // fallback square
            GFX->getDrawUtil()->drawRectFill(r, mHighlightColor, 2.0f, mBackgroundColor);
        }
        else
        {
            // fallback circle
            GFX->getDrawUtil()->drawCircleFill(r, mHighlightColor, size * 0.5f, 2.0f, mBackgroundColor);
        }
    }

    // 5) Bottom text label with frame under icon
    renderBottomTextWithFrame(center);

    Parent::onRender(offset, RectI());
}

//------------------------------------------------------------------------------
// renderItems: each entry with drop‐shadow and shape border
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::renderItems(const Point2I& center)
{
    if (mCurrentItems.empty())
        return;

    const F32 slice = (2 * M_PI_F) / mCurrentItems.size();
    for (U32 i = 0; i < mCurrentItems.size(); ++i)
    {
        Item* it = mCurrentItems[i];
        bool hover = (S32)i == mHoveredIndex;
        F32 iconSz = mBaseIconSize * (hover ? mHoverScale : 1.0f);
        ColorI col = hover ? mHighlightColor : ColorI(255, 255, 255, 255);
        ColorI shadowCol(0, 0, 0, 100);

        // compute position
        F32 ang = -M_PI_F / 2 + slice * (i + 0.5f);
        Point2I pos(
            center.x + (S32)((mRadius - mThickness * 0.5f) * mCos(ang)),
            center.y + (S32)((mRadius - mThickness * 0.5f) * mSin(ang))
        );
        RectI rect(pos - Point2I((S32)(iconSz / 2), (S32)(iconSz / 2)), Point2I((S32)iconSz, (S32)iconSz));
        RectI srect = rect; srect.point += Point2I(2, 2);

        // A) Drop‐shadow
        if (it->iconTexture.isValid())
        {
            GFX->getDrawUtil()->clearBitmapModulation();
            GFX->getDrawUtil()->setBitmapModulation(shadowCol);
            GFX->getDrawUtil()->drawBitmapStretch(
                it->iconTexture,
                srect,
                GFXBitmapFlip_None,
                GFXTextureFilterLinear,
                false,
                0.f
            );
        }
        else if (it->isBackButton)
        {
            GFX->getDrawUtil()->drawLine(srect.point, srect.point + srect.extent, shadowCol);
            GFX->getDrawUtil()->drawLine(
                Point2I(srect.point.x + srect.extent.x, srect.point.y),
                Point2I(srect.point.x, srect.point.y + srect.extent.y),
                shadowCol
            );
        }
        else if (!it->children.empty())
        {
            GFX->getDrawUtil()->drawRectFill(srect, shadowCol);
        }
        else
        {
            GFX->getDrawUtil()->drawCircleFill(srect, shadowCol, iconSz * 0.5f);
        }

        // B) Actual icon or fallback shape
        if (it->iconTexture.isValid())
        {
            GFX->getDrawUtil()->clearBitmapModulation();
            GFX->getDrawUtil()->setBitmapModulation(col);
            GFX->getDrawUtil()->drawBitmapStretch(
                it->iconTexture,
                rect,
                GFXBitmapFlip_None,
                GFXTextureFilterLinear,
                false,
                0.f
            );
        }
        else if (it->isBackButton)
        {
            GFX->getDrawUtil()->drawLine(rect.point, rect.point + rect.extent, col);
            GFX->getDrawUtil()->drawLine(
                Point2I(rect.point.x + rect.extent.x, rect.point.y),
                Point2I(rect.point.x, rect.point.y + rect.extent.y),
                col
            );
        }
        else if (!it->children.empty())
        {
            GFX->getDrawUtil()->drawRectFill(rect, col, 2.0f, mBackgroundColor);
        }
        else
        {
            GFX->getDrawUtil()->drawCircleFill(rect, col, iconSz * 0.5f, 2.0f, mBackgroundColor);
        }
    }
}

void GuiRadialMenuCtrl::renderBottomTextWithFrame(const Point2I& center)
{
    if (mHoveredIndex < 0 || mHoveredIndex >= (S32)mCurrentItems.size())
        return;

    const char* txt = mCurrentItems[mHoveredIndex]->name;
    GFont* font = mProfile->mFont;
    U32 w = font->getStrWidth(txt);
    S32 h = font->getHeight();

    // --- Position just below the big icon ---
    // The icon size in the center
    F32 iconSize = mBaseIconSize * mHoverScale * 1.5f;
    S32 textY = center.y + (S32)(iconSize * 0.5f) + 10; // 10px gap below icon

    // Padding for frame around text
    const S32 padX = 12, padY = 6, radius = 8;
    RectI frame(
        center.x - (S32)(w / 2) - padX,
        textY - padY,
        w + padX * 2,
        h + padY * 2
    );

    // Draw dark rounded frame (in front of icon)
    GFX->getDrawUtil()->drawRectFill(frame, ColorI(30, 30, 30, 220), radius);

    // Draw white centered text, with shadow for visibility
    Point2I pos(center.x - (S32)(w / 2), textY);

    GFX->getDrawUtil()->drawTextShadowed(
        font, pos, Point2I(2, 2), txt,
        ColorI(255, 255, 255, 255), // text color
        ColorI(0, 0, 0, 180)        // shadow color
    );
}

//------------------------------------------------------------------------------
// hitTestSegment against currentItems
//------------------------------------------------------------------------------
S32 GuiRadialMenuCtrl::hitTestSegment(const Point2I& local) const
{
    if (mCurrentItems.empty()) return -1;

    Point2I c = getExtent() / 2;
    Point2I d = local - c;
    F32 dist = mSqrt((F32)(d.x * d.x + d.y * d.y));
    if (dist < (mRadius - mThickness) || dist > mRadius) return -1;

    F32 ang = mAtan2((F32)d.y, (F32)d.x) + M_PI_F / 2;
    if (ang < 0) ang += 2 * M_PI_F;
    return (S32)(ang / ((2 * M_PI_F) / mCurrentItems.size()));
}

void GuiRadialMenuCtrl::centerCursor()
{
    // Grab the Canvas…
    GuiCanvas* cv = dynamic_cast<GuiCanvas*>(Sim::findObject("Canvas"));
    if (!cv)
        return;

    // Compute the exact middle of the Canvas
    Point2I canvasCenter = cv->getExtent() / 2;
    // Warp the OS cursor there
    cv->setCursorPos(canvasCenter);
}

//------------------------------------------------------------------------------
// onMouseMove: lock cursor to center and angle-select by vector
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::onMouseMove(const GuiEvent& evt)
{
    // 1) Compute the wheel’s center in global screen coords
    Point2I localCenter = getExtent() / 2;
    Point2I globalCenter = localToGlobalCoord(localCenter);

    // 2) Vector from center → current mouse
    Point2I delta = evt.mousePoint - globalCenter;

    // 3) Determine segment purely by angle (ignore distance)
    S32 newHit = -1;
    if (!mCurrentItems.empty())
    {
        F32 ang = mAtan2((F32)delta.y, (F32)delta.x) + M_PI_F / 2;
        if (ang < 0) ang += 2 * M_PI_F;
        const F32 slice = (2 * M_PI_F) / mCurrentItems.size();
        newHit = (S32)(ang / slice);
    }

    // 4) If changed, play sound + redraw
    if (newHit != mHoveredIndex)
    {
        mHoveredIndex = newHit;
        if (mHoveredIndex >= 0 && mHoverSound)
            SFX->playOnce(mHoverSound);
        setUpdate();
    }
}

bool GuiRadialMenuCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    // Reset to root level & redraw
    gatherCurrentLevel();
    setUpdate();

    // Center the cursor once
    centerCursor();

    // Lock all mouse input to this control
    if (GuiCanvas* cv = dynamic_cast<GuiCanvas*>(Sim::findObject("Canvas")))
        cv->mouseLock(this);

    return true;
}

void GuiRadialMenuCtrl::onSleep()
{
    // Unlock mouse so other controls can receive clicks again
    if (GuiCanvas* cv = dynamic_cast<GuiCanvas*>(Sim::findObject("Canvas")))
        cv->mouseUnlock(this);

    Parent::onSleep();
}

//------------------------------------------------------------------------------
// onMouseDown navigates submenus or executes and closes
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::onMouseDown(const GuiEvent& evt)
{
    // 1) Hit‐test
    Point2I loc = globalToLocalCoord(evt.mousePoint);

    // find which wedge we’re over right now
    S32 hit = hitTestSegment(loc);

    // if we’re not over any wedge, fall back to the last hover
    if (hit < 0 || hit >= (S32)mCurrentItems.size())
        hit = mHoveredIndex;

    // still nothing hovered? bail
    if (hit < 0 || hit >= (S32)mCurrentItems.size())
        return;

    // now confirm that selection
    mHoveredIndex = hit;
    Item* sel = mCurrentItems[hit];

    // 2) Back‐button handling
    if (sel->isBackButton && !mHistory.empty())
    {
        mCurrentItems = mHistory.last();
        mHistory.pop_back();
        mHoveredIndex = -1;
        setUpdate();
        centerCursor();         // <-- and here
        return;
    }

    // 3) Execute exactly one script call (function + one argument)
    if (sel->funcName && *sel->funcName)
    {
        if (mClickSound)
            SFX->playOnce(mClickSound);

        const char* argv[2] = { sel->funcName, sel->funcArg };
        Con::execute(2, argv);
    }

    // 4) Drill into a group
    if (!sel->children.empty())
    {
        mHistory.push_back(mCurrentItems);

        mCurrentItems.clear();
        mCurrentItems.push_back(&mBackItem);
        for (Item& c : sel->children)
            mCurrentItems.push_back(&c);

        mHoveredIndex = -1;
        setUpdate();
        centerCursor();         // <-- re-center here
        return;
    }

    // 5) Leaf: execute & close the wheel entirely
    if (GuiCanvas* cv = dynamic_cast<GuiCanvas*>(Sim::findObject("Canvas")))
    {
        // find the dialog (the parent of this control)
        if (GuiControl* dlg = dynamic_cast<GuiControl*>(getParent()))
        {
            // 1) pop it off the Canvas dialog stack
            cv->popDialogControl(dlg);
            // 2) hide it in case it's re-used
            dlg->setVisible(false);
        }
    }

    // 6) reset for next open
    mHistory.clear();
    clearItems();
    return;
}

//------------------------------------------------------------------------------
// onMouseUp: swallow
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::onMouseUp(const GuiEvent&)
{
    // consume
}

//------------------------------------------------------------------------------
// clearItems and gatherCurrentLevel
//------------------------------------------------------------------------------
void GuiRadialMenuCtrl::clearItems()
{
    mRootItems.clear();
    gatherCurrentLevel();
}

void GuiRadialMenuCtrl::gatherCurrentLevel()
{
    mCurrentItems.clear();
    for (Item& it : mRootItems)
        mCurrentItems.push_back(&it);
    mCurrentIndex = 0;
    mHoveredIndex = -1;
    setUpdate();
}

// Helper: returns true if assetId is non-null, non-empty, and not just whitespace
static bool isValidAssetId(const char* assetId)
{
    if (!assetId) return false;
    // Skip leading whitespace
    while (*assetId == ' ' || *assetId == '\t') ++assetId;
    return *assetId != 0;
}

//------------------------------------------------------------------------------
// addItemToTree builds the hierarchical tree
//------------------------------------------------------------------------------

void GuiRadialMenuCtrl::addItemToTree(const char* path,
    const char* name,
    const char* funcName,
    const char* funcArg)
{
    addItemToTree(path, name, funcName, funcArg, "");
}

void GuiRadialMenuCtrl::addItemToTree(
    const char* path,
    const char* name,
    const char* funcName,
    const char* funcArg,
    const char* iconAssetId)
{
    Vector<StringTableEntry> parts;
    if (path && *path)
    {
        char* dup = dStrdup(path);
        for (char* token = dStrtok(dup, "/"); token; token = dStrtok(NULL, "/"))
            parts.push_back(StringTable->insert(token));
        dFree(dup);
    }

    Vector<Item>* branch = &mRootItems;
    Item* lastCategory = nullptr;

    for (U32 idx = 0; idx < parts.size(); ++idx)
    {
        StringTableEntry groupName = parts[idx];
        bool found = false;
        for (Item& it : *branch)
        {
            if (it.name == groupName && !it.isBackButton)
            {
                lastCategory = &it;
                // ONLY assign/clear icon for the last path part IF we're not adding a leaf
                if (idx == parts.size() - 1 && (!name || !*name))
                {
                    if (isValidAssetId(iconAssetId))
                        safeLoadIcon(it, iconAssetId);
                    else
                    {
                        it.iconAssetId = StringTable->insert("");
                        it.iconAsset.clear();
                        it.iconTexture = GFXTexHandle();
                    }
                }
                branch = &it.children;
                found = true;
                break;
            }
        }
        if (!found)
        {
            Item cat;
            cat.name = groupName;
            cat.funcName = StringTable->insert("");
            cat.funcArg = StringTable->insert("");
            cat.isBackButton = false;
            // ONLY assign/clear icon for the last path part IF we're not adding a leaf
            if (idx == parts.size() - 1 && (!name || !*name) && isValidAssetId(iconAssetId))
                safeLoadIcon(cat, iconAssetId);
            else
            {
                cat.iconAssetId = StringTable->insert("");
                cat.iconAsset.clear();
                cat.iconTexture = GFXTexHandle();
            }
            branch->push_back(cat);
            lastCategory = &branch->last();
            branch = &lastCategory->children;
        }
    }

    // Add leaf only if name is non-empty (never updates group icon)
    if (name && *name)
    {
        Item leaf;
        leaf.name = StringTable->insert(name);
        leaf.funcName = StringTable->insert(funcName);
        leaf.funcArg = StringTable->insert(funcArg);
        leaf.isBackButton = false;
        if (isValidAssetId(iconAssetId))
            safeLoadIcon(leaf, iconAssetId);
        else
        {
            leaf.iconAssetId = StringTable->insert("");
            leaf.iconAsset.clear();
            leaf.iconTexture = GFXTexHandle();
        }
        AssertFatal(branch != nullptr, "GuiRadialMenuCtrl::addItemToTree: branch is nullptr!");
        branch->push_back(leaf);
    }

    if (parts.empty())
        gatherCurrentLevel();
}



//------------------------------------------------------------------------------
// Script bindings
//------------------------------------------------------------------------------
DefineEngineMethod(GuiRadialMenuCtrl, clearItems, void, (), ,
    "Remove every entry from the wheel.")
{
    object->clearItems();
}

DefineEngineMethod(GuiRadialMenuCtrl, addItem, void,
    (const char* path, const char* name, const char* funcName, const char* funcArg, const char* iconAssetId),
    ("", "", "", "", ""),
    "@brief Add a segment to the radial menu, optionally with an ImageAsset icon.\n"
    "@param path        Slash-delimited category path (\"\" = root)\n"
    "@param name        Display name\n"
    "@param funcName    Script function to call on click\n"
    "@param funcArg     Single string argument to pass\n"
    "@param iconAssetId Asset ID of an ImageAsset to show instead of the default shape\n")
{
    object->addItemToTree(path, name, funcName, funcArg,
        (iconAssetId && *iconAssetId) ? iconAssetId : "");
}
