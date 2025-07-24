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

   Resource<GFont> mCustomFont;

   void drawTextWithOutline(GFont* font, const Point2I& pos, const char* text,
      const ColorI& textColor, const ColorI& outlineColor,
      S32 thickness);

public:
   GuiTextEditColorCtrl();

   DECLARE_CONOBJECT(GuiTextEditColorCtrl);
   static void initPersistFields();

   bool onWake() override;
   void onRender(Point2I offset, const RectI& updateRect) override;

   void refreshFont();
};

#endif // _GUI_TEXT_COLOR_CTRL_H_
