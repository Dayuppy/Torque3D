// GuiBitmapItemExtended.h
#pragma once

#include "gui/core/guiControl.h"
#include "console/consoleTypes.h"
#include "assets/assetPtr.h"
#include "T3D/assets/ImageAsset.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/containers/guiPanel.h"

class GuiBitmapItemExtended : public GuiControl
{
   typedef GuiControl Parent;

public:
   GuiBitmapItemExtended();
   virtual ~GuiBitmapItemExtended() {}

   DECLARE_CONOBJECT(GuiBitmapItemExtended);
   static void initPersistFields();

   bool onAdd() override;
   bool onWake() override;
   void onRender(Point2I offset, const RectI& updateRect) override;

protected:
   AssetPtr<ImageAsset> mBitmapAsset;      ///< Asset reference
   StringTableEntry     mTitle;
   StringTableEntry     mBottomLeftText;
   StringTableEntry     mBottomRightText;
   F32                  mProgress;
   ColorI               mBarColor;
   ColorI               mTextColor;

   // Custom setter for the bitmapAsset field
   static bool _setBitmapAsset(void* obj, const char* index, const char* data)
   {
      GuiBitmapItemExtended* ctrl = static_cast<GuiBitmapItemExtended*>(obj);
      ctrl->mBitmapAsset = AssetPtr<ImageAsset>(StringTable->insert(data));
      return false;  // we handled assignment
   }
};
