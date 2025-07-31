//-----------------------------------------------------------------------------
// File: GuiContextMenuControl.h
// Purpose: Minimal, self-sizing context menu control for Torque 3D, with bitmap
//          background asset support, highlight bitmaps, overlay glow, text frame,
//          and optional sub-menu support.
//-----------------------------------------------------------------------------

#pragma once

#include "gui/core/guiControl.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gFont.h"
#include <vector>

//-----------------------------------------------------------------------------
// Single context menu entry
//-----------------------------------------------------------------------------
class GuiContextMenuEntry
{
public:
    String      text;        ///< Display text
    String      command;     ///< TorqueScript command
    bool        hasSubMenu;  ///< True if this entry should spawn a sub-menu

    GuiContextMenuEntry();
    ~GuiContextMenuEntry();
};

//-----------------------------------------------------------------------------
// Context menu control
//-----------------------------------------------------------------------------
class GuiContextMenuControl : public GuiControl
{
    typedef GuiControl Parent;

public:
    GuiContextMenuControl();
    virtual ~GuiContextMenuControl();

    static void initPersistFields();

    void addEntry(const char* text, const char* command);

    /// Open (or re-open) the menu at screen position pos for the given object ID
    void openMenu(Point2I pos, const char* contextObjId);

    /// Close and clear the menu
    void closeMenu();

    DECLARE_CONOBJECT(GuiContextMenuControl);

protected:
    struct Entry
    {
        String text;
        String command;
    };

    // Background bitmap asset
    AssetPtr<ImageAsset> mBackgroundBitmapAsset;
    GFXTexHandle         mBackgroundBitmap;
    static bool          _setBackgroundBitmapAsset(void* obj, const char* idx, const char* data);
    static const char* _getBackgroundBitmapAsset(void* obj, const char* data);
    void                  onBackgroundBitmapAssetRefresh();

    // Highlight bitmap asset (for hovered entries)
    AssetPtr<ImageAsset> mHighlightBitmapAsset;
    GFXTexHandle         mHighlightBitmap;
    static bool          _setHighlightBitmapAsset(void* obj, const char* idx, const char* data);
    static const char* _getHighlightBitmapAsset(void* obj, const char* data);
    void                  onHighlightBitmapAssetRefresh();

    // Overlay glow bitmap asset
    AssetPtr<ImageAsset> mGlowBitmapAsset;
    GFXTexHandle         mGlowBitmap;
    static bool          _setGlowBitmapAsset(void* obj, const char* idx, const char* data);
    static const char* _getGlowBitmapAsset(void* obj, const char* data);
    void                  onGlowBitmapAssetRefresh();

    // Colors and fonts
    ColorI               mBackgroundColor;
    ColorI               mTextColor;
    ColorI               mHighlightColor;
    ColorI               mTextFrameColor;
    StringTableEntry     mFontName;
    S32                  mFontSize;
    Resource<GFont>      mCustomFont;
    const char*          mContextObjId;

    // Text-frame settings
    bool                 mDrawTextFrame;
    S32                  mTextFramePadding;

    // Highlight & glow options
    bool                 mUseHighlightBitmap;
    bool                 mUseGlow;
    ColorI               mGlowColor;
    S32                  mGlowThickness;

    // Prefix for entries that spawn a sub-menu instead of executing immediately
    StringTableEntry     mSubMenuPrefix;

    // Dynamic entry list
    std::vector<GuiContextMenuEntry*> mEntries;

    // State
    bool                 mVisible;
    S32                  mHoverIndex;

    // Entry management
    void                 clearEntries();

    // Layout & sizing
    void                 calculateMenuSize(S32& outW, S32& outH);
    void                 refreshFont();

    // GuiControl overrides
    bool                 onAdd() override;
    bool                 onWake() override;
    void                 onRender(Point2I offset, const RectI& updateRect) override;

    void                 onMouseMove(const GuiEvent& event) override;
    void                 onMouseDown(const GuiEvent& event) override;
    void                 onRightMouseDown(const GuiEvent& event) override;
    void                 onLoseFirstResponder() override;
};