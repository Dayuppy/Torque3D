#include "GuiBitmapItemExtended.h"
#include "console/engineAPI.h"
#include "gfx/gfxTextureProfile.h"

IMPLEMENT_CONOBJECT(GuiBitmapItemExtended);
ConsoleDocClass(GuiBitmapItemExtended,
   "@brief GUI control: square bitmap with title, labels, and progress bar.\n"
   "@ingroup GuiControls"
);

GuiBitmapItemExtended::GuiBitmapItemExtended()
{
   // Defaults
   mTitle = StringTable->insert("Title");
   mBottomLeftText = StringTable->insert("");
   mBottomRightText = StringTable->insert("");
   mProgress = 0.0f;
   mBarColor.set(0, 200, 0, 255);
   mTextColor.set(255, 255, 255, 255);
}

void GuiBitmapItemExtended::initPersistFields()
{
   addGroup("BitmapItem");
   addProtectedField("bitmapAsset", TypeImageAssetPtr, Offset(mBitmapAsset, GuiBitmapItemExtended), _setBitmapAsset, &defaultProtectedGetFn, "Image asset ID");
   addField("title", TypeString, Offset(mTitle, GuiBitmapItemExtended), "Title text");
   addField("bottomLeft", TypeString, Offset(mBottomLeftText, GuiBitmapItemExtended), "Bottom-left label");
   addField("bottomRight", TypeString, Offset(mBottomRightText, GuiBitmapItemExtended), "Bottom-right label");
   addField("progress", TypeF32, Offset(mProgress, GuiBitmapItemExtended), "Progress [0..1]");
   addField("barColor", TypeColorI, Offset(mBarColor, GuiBitmapItemExtended), "Bar fill color");
   addField("textColor", TypeColorI, Offset(mTextColor, GuiBitmapItemExtended), "Text color");
   endGroup("BitmapItem");

   Parent::initPersistFields();
}

bool GuiBitmapItemExtended::onAdd()
{
   if (!Parent::onAdd())
      return false;
   return true;
}

bool GuiBitmapItemExtended::onWake()
{
   if (!Parent::onWake())
      return false;

   return true;
}

void GuiBitmapItemExtended::onRender(Point2I offset, const RectI& updateRect)
{
   Point2I ext = getExtent();
   if (ext.x <= 0 || ext.y <= 0 || !mProfile || !mProfile->mFont)
      return;

   // Square extents
   S32 size = ext.x < ext.y ? ext.x : ext.y;
   RectI bounds(offset, Point2I(size, size));
   GFXDrawUtil* drawer = GFX->getDrawUtil();

   // Placeholder background (more transparent)
   drawer->drawRectFill(bounds, ColorI(20, 20, 20, 150));

   // Draw valid asset
   if (mBitmapAsset.notNull() && mBitmapAsset->getStatus() == ImageAsset::Ok)
   {
      GFXTexHandle tex = mBitmapAsset->getTexture(&GFXStaticTextureProfile);
      if (tex.isValid())
      {
         RectI texRect = bounds;
         texRect.inset(2, 2);
         drawer->drawBitmapStretch(tex, texRect);
      }
   }

   GFont* font = mProfile->mFont;

   // Title
   if (mTitle && mTitle[0])
   {
      S32 w = font->getStrWidth(mTitle);
      drawer->drawText(font, bounds.point + Point2I((size - w) / 2, 4), mTitle, &mTextColor);
   }

   // Bottom-left
   if (mBottomLeftText && mBottomLeftText[0])
   {
      drawer->drawText(font, bounds.point + Point2I(4, size - font->getHeight() - 8), mBottomLeftText, &mTextColor);
   }

   // Bottom-right
   if (mBottomRightText && mBottomRightText[0])
   {
      S32 w = font->getStrWidth(mBottomRightText);
      drawer->drawText(font, bounds.point + Point2I(size - w - 4, size - font->getHeight() - 8), mBottomRightText, &mTextColor);
   }

   // Progress bar lowered below text
   {
      // Calculate Y position below corner text
      const S32 textH = font->getHeight();
      const S32 barH = 6;
      const S32 margin = 16;

      S32 barY = size - textH - 8 + margin; // places bar a few pixels below the text
      RectI barBg(bounds.point + Point2I(4, barY), Point2I(size - 8, barH));
      drawer->drawRectFill(barBg, ColorI(40, 40, 40, 150));
      if (mProgress > 0.0f)
      {
         RectI fill = barBg;
         fill.extent.x = S32(fill.extent.x * mProgress);
         drawer->drawRectFill(fill, mBarColor);
      }
   }

   renderChildControls(offset, updateRect);
}
