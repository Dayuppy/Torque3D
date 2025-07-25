//-----------------------------------------------------------------------------
// File: GuiBitmapItemExtended.h
// GUI control: square bitmap with optional rotation, tint, shadow, background, text overlay, and customizable font.
//-----------------------------------------------------------------------------
#pragma once

#include "gui/core/guiControl.h"
#include "assets/assetPtr.h"
#include "T3D/assets/ImageAsset.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gFont.h"


class GuiBitmapItemExtended : public GuiControl
{
   typedef GuiControl Parent;

public:
   GuiBitmapItemExtended();
   virtual ~GuiBitmapItemExtended() {}

   DECLARE_CONOBJECT(GuiBitmapItemExtended);
   DECLARE_CALLBACK(void, onMouseDragged, ());
   static void initPersistFields();

protected:
   // Asset & texture
   AssetPtr<ImageAsset> mBitmapAsset;
   GFXTexHandle         mBitmap;

   // Visual options
   ColorI               mColor;               ///< Tint & alpha
   F32                  mAngle;               ///< Rotation angle in degrees
   bool                 mDrawBackgroundRect;  ///< Draw filled rect behind bitmap
   ColorI               mBackgroundColor;     ///< Background fill color
   bool                 mBitmapShadow;        ///< Drop shadow flag
   ColorI               mBitmapShadowColor;   ///< Shadow color
   Point2I              mBitmapShadowOffset;  ///< Shadow offset

   // Overlay text
   StringTableEntry     mTitle;
   StringTableEntry     mBottomLeftText;
   StringTableEntry     mBottomRightText;
   F32                  mProgress;
   ColorI               mBarColor;
   ColorI               mTextColor;
   bool                 mDrawTextFrame;
   ColorI               mTextFrameColor;
   S32                  mTextFramePadding;

   // Font customization
   StringTableEntry     mFontName;
   S32                  mFontSize;
   Resource<GFont>      mCustomFont;

   // Field handlers
   static bool        _setBitmapAsset(void* obj, const char* index, const char* data);
   static const char* _getBitmapAsset(void* obj, const char* data);

   // Asset reload callback
   void onAssetRefresh();

   // Refresh custom font
   void refreshFont();

   // GuiControl overrides
   bool onAdd() override;
   bool onWake() override;
   void onRender(Point2I offset, const RectI& updateRect) override;
   void onMouseDragged(const GuiEvent& event) override;
};
