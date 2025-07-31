//=============================================================================
// File: GuiTextEditColorCtrl.h (Text Edit Version)
//=============================================================================
#ifndef _GUI_TEXT_EDIT_COLOR_CTRL_H_
#define _GUI_TEXT_EDIT_COLOR_CTRL_H_

#include "gui/controls/guiTextEditCtrl.h"
#include "gfx/gFont.h"
#include "core/color.h"
#include "gui/core/guiTypes.h"
#include "core/resourceManager.h"

class GuiTextEditColorCtrl : public GuiTextEditCtrl
{
   typedef GuiTextEditCtrl Parent;

protected:
   bool mUseShadow;
   ColorI mShadowColor;
   Point2I mShadowOffset;

   bool mUseOutline;
   ColorI mOutlineColor;
   S32 mOutlineThickness;

   GuiControlProfile::AlignmentType mAlignment;
   StringTableEntry mFontName;
   S32 mFontSize;
   ColorI mFontColor;
   bool mWordWrap;
   bool mDrawFrame;
   ColorI mFrameColor;
   ColorI mPlaceholderColor;
   bool mContainerOpaque;

   Resource<GFont> mCustomFont;

public:
   GuiTextEditColorCtrl();

   DECLARE_CONOBJECT(GuiTextEditColorCtrl);
   static void initPersistFields();

   virtual bool onWake() override;
   virtual void onRender(Point2I offset, const RectI& updateRect) override;
   virtual void onMouseDown(const GuiEvent& event) override;
   virtual void drawText(const RectI& drawRect, bool isFocused) override;
   virtual void onStaticModified(const char* slotName, const char* newValue = NULL) override;
   virtual GuiControl* findHitControl(const Point2I& pt, S32 initialLayer) override;

   void refreshFont();
};

#endif // _GUI_TEXT_COLOR_CTRL_H_
