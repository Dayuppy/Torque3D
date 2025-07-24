//=============================================================================
// File: GuiTextColorCtrl.cpp
//=============================================================================

#include "platform/platform.h"
#include "guiTextColorCtrl.h"
#include "gfx/gfxDrawUtil.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include <gui/core/guiDefaultControlRender.h>

// Register the class with Torque's RTTI system
IMPLEMENT_CONOBJECT(GuiTextColorCtrl);

//-----------------------------------------------------------------------------
GuiTextColorCtrl::GuiTextColorCtrl()
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
void GuiTextColorCtrl::initPersistFields()
{
   addField("useShadow", TypeBool, Offset(mUseShadow, GuiTextColorCtrl), "Render a drop shadow");
   addField("shadowColor", TypeColorI, Offset(mShadowColor, GuiTextColorCtrl), "Shadow color");
   addField("shadowOffset", TypePoint2I, Offset(mShadowOffset, GuiTextColorCtrl), "Shadow offset");
   addField("useOutline", TypeBool, Offset(mUseOutline, GuiTextColorCtrl), "Render an outline");
   addField("outlineColor", TypeColorI, Offset(mOutlineColor, GuiTextColorCtrl), "Outline color");
   addField("outlineThickness", TypeS32, Offset(mOutlineThickness, GuiTextColorCtrl), "Outline thickness");
   addField("justify", TypeGuiAlignmentType, Offset(mAlignment, GuiTextColorCtrl), "Text justification");
   addField("wordWrap", TypeBool, Offset(mWordWrap, GuiTextColorCtrl), "Enable word wrapping");
   addField("fontName", TypeCaseString, Offset(mFontName, GuiTextColorCtrl), "Font face name");
   addField("fontSize", TypeS32, Offset(mFontSize, GuiTextColorCtrl), "Font size");
   addField("fontColor", TypeColorI, Offset(mFontColor, GuiTextColorCtrl), "Font color");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------
bool GuiTextColorCtrl::onWake()
{
   if (!Parent::onWake())
      return false;
   refreshFont();
   return true;
}

//-----------------------------------------------------------------------------
void GuiTextColorCtrl::refreshFont()
{
   if (mFontName && mFontName[0] && mFontSize > 0)
   {
      Resource<GFont> newFont = GFont::create(mFontName, mFontSize);
      if (newFont)
         mCustomFont = newFont;
      else
         Con::warnf("GuiTextColorCtrl::refreshFont - Failed to create font '%s' size %d", mFontName, mFontSize);
   }
   else
   {
      mCustomFont = nullptr;
   }
}

//=============================================================================
// Rendering
//=============================================================================
void GuiTextColorCtrl::onRender(Point2I offset, const RectI& updateRect)
{
   GFXDrawUtil* du = GFX->getDrawUtil();
   GFont* font = mCustomFont ? mCustomFont : mProfile->mFont;
   if (!font) return;

   // Build lines for rendering (word wrap or single line)
   Vector<String> lines;
   if (mWordWrap)
   {
      Vector<String> words;
      String(getText()).split(" ", words);
      String current;
      S32    currentWidth = 0;
      for (const auto& part : words)
      {
         String w = part + String(" ");
         S32 wwid = font->getStrWidth(w);
         if (currentWidth + wwid > getExtent().x && current.length() > 0)
         {
            lines.push_back(current);
            current = w;
            currentWidth = wwid;
         }
         else
         {
            current += w;
            currentWidth += wwid;
         }
      }
      if (current.length() > 0)
         lines.push_back(current);
   }
   else
   {
      lines.push_back(getText());
   }

   // Draw each line
   S32 lineHeight = font->getHeight();
   for (S32 i = 0; i < (S32)lines.size(); ++i)
   {
      const String& textLine = lines[i];
      Point2I       drawPoint = offset;
      drawPoint.y += i * lineHeight;

      // Horizontal alignment
      S32 textWidth = font->getStrWidth(textLine);
      switch (mAlignment)
      {
      case GuiControlProfile::CenterJustify:
         drawPoint.x += (getExtent().x - textWidth) / 2;
         break;
      case GuiControlProfile::RightJustify:
         drawPoint.x += getExtent().x - textWidth;
         break;
      default:
         break;
      }

      // Shadow + Outline + Plain using GFXDrawUtil helpers
      if (mUseShadow && mUseOutline)
      {
         du->drawTextShadowed(font, drawPoint, mShadowOffset, textLine.c_str(), mFontColor, mShadowColor);
         du->drawTextOutlined(font, drawPoint, textLine.c_str(), mFontColor, mOutlineColor, mOutlineThickness);
      }
      else if (mUseShadow)
      {
         du->drawTextShadowed(font, drawPoint, mShadowOffset, textLine.c_str(), mFontColor, mShadowColor);
      }
      else if (mUseOutline)
      {
         du->drawTextOutlined(font, drawPoint, textLine.c_str(), mFontColor, mOutlineColor, mOutlineThickness);
      }
      else
      {
         du->drawText(font, drawPoint, textLine.c_str(), &mFontColor);
      }
   }

   // Render child controls (cursor, selection, etc.)
   renderChildControls(offset, updateRect);
}
