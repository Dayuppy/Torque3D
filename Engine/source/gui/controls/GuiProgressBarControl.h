#pragma once

#ifndef _GUI_PROGRESSBAR_CTRL_H_
#define _GUI_PROGRESSBAR_CTRL_H_

#include "gui/core/guiControl.h"
#include "gfx/gfxStructs.h"

class GuiProgressBarControl : public GuiControl
{
   typedef GuiControl Parent;

   F32 mValue;
   F32 mMaxValue;
   F32 mSmoothValue;

   ColorI mBarColor;
   ColorI mBackColor;

   bool mDrawFrame;
   ColorI mFrameColor;

public:
   GuiProgressBarControl();

   DECLARE_CONOBJECT(GuiProgressBarControl);
   DECLARE_CATEGORY("Gui Game");
   DECLARE_DESCRIPTION("A basic progress bar with optional percentage display.");

   bool mDrawPercentage;

   static void initPersistFields();

   void onRender(Point2I offset, const RectI& updateRect) override;
   bool onAdd() override;
   void onRemove() override;

   void setValue(F32 v);
   F32 getValue() const { return mValue; }

   void setMaxValue(F32 v);
   F32 getMaxValue() const { return mMaxValue; }
};

#endif // _GUI_PROGRESSBAR_CTRL_H_
