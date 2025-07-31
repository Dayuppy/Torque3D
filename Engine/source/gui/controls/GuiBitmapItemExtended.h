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

   static void initPersistFields();

   void setForceGlow(bool enabled);

   DECLARE_CONOBJECT(GuiBitmapItemExtended);
   DECLARE_CALLBACK(void, onMouseDragged, ());
   DECLARE_CALLBACK(void, onMouseUp, ());
   DECLARE_CALLBACK(void, onMouseEnter, ());
   DECLARE_CALLBACK(void, onMouseLeave, ());

protected:
   // Icon Asset & texture
   AssetPtr<ImageAsset> mBitmapAsset;
   GFXTexHandle         mBitmap;

   // Frame Asset & texture
   AssetPtr<ImageAsset> mBitmapFrameAsset;
   GFXTexHandle         mBitmapFrame;

   // Glow Asset & texture
   AssetPtr<ImageAsset> mBitmapGlowAsset;
   GFXTexHandle         mBitmapGlow;

   bool mUseBitmapFrame;
   bool mUseBitmapGlow;

   // Asset reload handlers
   void onAssetRefresh();
   void onFrameAssetRefresh();
   void onGlowAssetRefresh();

   // Asset field handlers (for persistence)
   static bool        _setBitmapAsset(void* obj, const char* index, const char* data);
   static const char* _getBitmapAsset(void* obj, const char* data);

   static bool        _setBitmapFrameAsset(void* obj, const char* index, const char* data);
   static const char* _getBitmapFrameAsset(void* obj, const char* data);

   static bool        _setBitmapGlowAsset(void* obj, const char* index, const char* data);
   static const char* _getBitmapGlowAsset(void* obj, const char* data);

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

   // Glow outline on hover or focus
   bool    mGlowEnabled;
   ColorI  mGlowColor;
   S32     mGlowThickness;
   bool    mHovering;
   bool    mForceGlow;

   // Font refresh
   void refreshFont();

   // GuiControl overrides
   bool onAdd() override;
   bool onWake() override;

   virtual void onRender(Point2I offset, const RectI& updateRect) override;

   virtual void onMouseDragged(const GuiEvent& event) override;
   virtual void onMouseUp(const GuiEvent&) override;
   virtual void onMouseEnter(const GuiEvent&) override;
   virtual void onMouseLeave(const GuiEvent&) override;

   /// Called when this control gains keyboard focus
   virtual void onGainFirstResponder() override;

   /// Called when this control loses keyboard focus
   virtual void onLoseFirstResponder() override;
};
