//=============================================================================
// File: GuiTextEditColorCtrl.cpp
//=============================================================================

#include "platform/platform.h"
#include "gfx/gfxDrawUtil.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include <gui/core/guiDefaultControlRender.h>
#include "guiTextEditColorCtrl.h"

// Register the class with Torque's RTTI system
IMPLEMENT_CONOBJECT(GuiTextEditColorCtrl);

//-----------------------------------------------------------------------------
GuiTextEditColorCtrl::GuiTextEditColorCtrl()
{
   mUseShadow = true;
   mShadowColor.set(0, 0, 0, 255);
   mShadowOffset.set(2, 2);

   mUseOutline = true;
   mOutlineColor.set(0, 0, 0, 255);
   mOutlineThickness = 1;

   mAlignment = GuiControlProfile::CenterJustify;
   mFontName = StringTable->insert("Arial");
   mFontSize = 20;
   mFontColor.set(255, 255, 255, 255);

   mWordWrap = true;
}

//-----------------------------------------------------------------------------
void GuiTextEditColorCtrl::initPersistFields()
{
   addField("useShadow", TypeBool, Offset(mUseShadow, GuiTextEditColorCtrl), "Render a drop shadow");
   addField("shadowColor", TypeColorI, Offset(mShadowColor, GuiTextEditColorCtrl), "Shadow color");
   addField("shadowOffset", TypePoint2I, Offset(mShadowOffset, GuiTextEditColorCtrl), "Shadow offset");
   addField("useOutline", TypeBool, Offset(mUseOutline, GuiTextEditColorCtrl), "Render an outline");
   addField("outlineColor", TypeColorI, Offset(mOutlineColor, GuiTextEditColorCtrl), "Outline color");
   addField("outlineThickness", TypeS32, Offset(mOutlineThickness, GuiTextEditColorCtrl), "Outline thickness");
   addField("justify", TypeGuiAlignmentType, Offset(mAlignment, GuiTextEditColorCtrl), "Text justification");
   addField("wordWrap", TypeBool, Offset(mWordWrap, GuiTextEditColorCtrl), "Enable word wrapping");
   addField("fontName", TypeCaseString, Offset(mFontName, GuiTextEditColorCtrl), "Font face name");
   addField("fontSize", TypeS32, Offset(mFontSize, GuiTextEditColorCtrl), "Font size");
   addField("fontColor", TypeColorI, Offset(mFontColor, GuiTextEditColorCtrl), "Font color");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------
bool GuiTextEditColorCtrl::onWake()
{
   if (!Parent::onWake())
      return false;
   refreshFont();
   return true;
}

//-----------------------------------------------------------------------------
void GuiTextEditColorCtrl::refreshFont()
{
   if (mFontName && mFontName[0] && mFontSize > 0)
   {
      Resource<GFont> newFont = GFont::create(mFontName, mFontSize);
      if (newFont)
         mCustomFont = newFont;
      else
         Con::warnf("GuiTextEditColorCtrl::refreshFont - Failed to create font '%s' size %d", mFontName, mFontSize);
   }
   else
   {
      mCustomFont = nullptr;
   }
}

//=============================================================================
//=============================================================================
// Full onRender
//=============================================================================
void GuiTextEditColorCtrl::onRender(Point2I offset, const RectI& updateRect)
{
   GFXDrawUtil* du = GFX->getDrawUtil();
   RectI        ctrlRect = RectI(offset, getExtent());

   // Draw background & border
   if (mProfile->mOpaque)
      du->drawRectFill(ctrlRect, mProfile->mFillColor);
   if (mProfile->mBorder)
      renderBorder(ctrlRect, mProfile);

   // Prepare text
   const char* renderText = getScriptValue();
   Point2I     drawPoint = offset;
   drawPoint.y += (getExtent().y - mProfile->mFont->getHeight()) / 2;

   // Choose font
   GFont* font = mCustomFont ? mCustomFont : mProfile->mFont;
   if (font && renderText)
   {
      if (mUseShadow && mUseOutline)
      {
         // Shadow then outline
         du->drawTextShadowed(font, drawPoint, mShadowOffset, renderText, mFontColor, mShadowColor);
         du->drawTextOutlined(font, drawPoint, renderText, mFontColor, mOutlineColor, mOutlineThickness);
      }
      else if (mUseShadow)
      {
         du->drawTextShadowed(font, drawPoint, mShadowOffset, renderText, mFontColor, mShadowColor);
      }
      else if (mUseOutline)
      {
         du->drawTextOutlined(font, drawPoint, renderText, mFontColor, mOutlineColor, mOutlineThickness);
      }
      else
      {
         du->drawText(font, drawPoint, renderText, &mFontColor);
      }
   }

   // Parent draws cursor/selection
   Parent::onRender(offset, updateRect);
}
