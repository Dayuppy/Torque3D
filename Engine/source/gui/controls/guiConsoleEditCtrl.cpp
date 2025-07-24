#include "console/consoleTypes.h"
#include "console/console.h"
#include "gui/core/guiCanvas.h"
#include "gui/controls/guiConsoleEditCtrl.h"
#include "core/frameAllocator.h"

IMPLEMENT_CONOBJECT(GuiConsoleEditCtrl);

ConsoleDocClass(GuiConsoleEditCtrl,
   "@brief Text entry element of a GuiConsole.\n\n"
   "@tsexample\n"
   "new GuiConsoleEditCtrl(ConsoleEntry)\n"
   "{\n"
   "   profile = \"ConsoleTextEditProfile\";\n"
   "   horizSizing = \"width\";\n"
   "   vertSizing = \"top\";\n"
   "   position = \"0 462\";\n"
   "   extent = \"640 18\";\n"
   "   minExtent = \"8 8\";\n"
   "   visible = \"1\";\n"
   "   altCommand = \"ConsoleEntry::eval();\";\n"
   "   helpTag = \"0\";\n"
   "   maxLength = \"255\";\n"
   "   historySize = \"40\";\n"
   "   password = \"0\";\n"
   "   tabComplete = \"0\";\n"
   "};\n"
   "@endtsexample\n\n"
   "@ingroup GuiCore"
);

GuiConsoleEditCtrl::GuiConsoleEditCtrl()
{
   // Removed all key sinking and sibling scroller support.
}

void GuiConsoleEditCtrl::initPersistFields()
{
   docsURL;

   // Removed useSiblingScroller persist field

   Parent::initPersistFields();
}

bool GuiConsoleEditCtrl::onKeyDown(const GuiEvent& event)
{
   setUpdate();

   if (event.keyCode == KEY_TAB)
   {
      FrameTemp<UTF8> tmpBuff(GuiTextCtrl::MAX_STRING_LENGTH);
      mTextBuffer.getCopy8(tmpBuff, GuiTextCtrl::MAX_STRING_LENGTH);

      bool forward = (event.modifier & SI_SHIFT) == 0;
      mCursorPos = Con::tabComplete(tmpBuff, mCursorPos, GuiTextCtrl::MAX_STRING_LENGTH, forward);

      mTextBuffer.set(tmpBuff);
      return true;
   }
   else if (event.keyCode == KEY_RETURN || event.keyCode == KEY_NUMPADENTER)
   {
      if ((event.modifier & SI_SHIFT) &&
         mTextBuffer.length() + dStrlen("echo();") <= GuiTextCtrl::MAX_STRING_LENGTH)
      {
         char buf[GuiTextCtrl::MAX_STRING_LENGTH];
         getText(buf);

         String text(buf);
         text.replace(";", "");
         text = String::ToString("echo(%s);", text.c_str());
         setText(text.utf8());
      }

      return Parent::dealWithEnter(false);
   }

   return Parent::onKeyDown(event);
}
