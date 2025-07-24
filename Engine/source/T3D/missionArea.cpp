//-----------------------------------------------------------------------------  
// Copyright (c) 2012 GarageGames, LLC  
//  
// Permission is hereby granted, free of charge, to any person obtaining a copy  
// of this software and associated documentation files (the "Software"), to  
// deal in the Software without restriction, including without limitation the  
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  
// sell copies of the Software, and to permit persons to whom the Software is  
// furnished to do so, subject to the following conditions:  
//  
// The above copyright notice and this permission notice shall be included in  
// all copies or substantial portions of the Software.  
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING  
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS  
// IN THE SOFTWARE.  
//-----------------------------------------------------------------------------  

#include "T3D/missionArea.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "math/mathIO.h"
#include "console/engineAPI.h"

IMPLEMENT_CO_NETOBJECT_V1(MissionArea);

ConsoleDocClass(MissionArea,
   "@brief Level object which defines the boundaries of the level.\n\n"
   "This is a simple box with starting points, width, depth, and height... "
   "Extended to support arbitrary polygons via polyString.\n"
   "@ingroup enviroMisc\n"
);

// Legacy static rect default
RectI MissionArea::smMissionArea(Point2I(768, 768), Point2I(512, 512));
// Singleton pointer
MissionArea* MissionArea::smServerObject = NULL;

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
MissionArea::MissionArea()
{
   // Legacy rectangle initialization
   const RectI defaultRect(Point2I(768, 768), Point2I(512, 512));
   mArea = smMissionArea = defaultRect;

   // Polygon default: mirror the rect
   {
      char buf[256];
      dSprintf(buf, sizeof(buf),
         "%d %d  %d %d  %d %d  %d %d",
         defaultRect.point.x, defaultRect.point.y,
         defaultRect.point.x + defaultRect.extent.x, defaultRect.point.y,
         defaultRect.point.x + defaultRect.extent.x, defaultRect.point.y + defaultRect.extent.y,
         defaultRect.point.x, defaultRect.point.y + defaultRect.extent.y
      );
      mPolyString = buf;
   }
   parsePolyString();

   mFlightCeiling = 2000;
   mFlightCeilingRange = 50;
   mNetFlags.set(Ghostable | ScopeAlways);
}

//------------------------------------------------------------------------------
// Legacy rectangle setter (must match the header exactly)
void MissionArea::setArea(const RectI& area)
{
   // Update both the member and the static
   mArea = smMissionArea = area;

   // Mark for network update on the server
   if (isServerObject())
      setMaskBits(UpdateMask);
}

//------------------------------------------------------------------------------
// Polygon setter
//------------------------------------------------------------------------------
void MissionArea::setPolyString(const String& poly)
{
   // Store the raw string locally
   mPolyString = poly;

   // Re-parse our internal vertex list
   parsePolyString();

   // Update our legacy AABB so the editor gizmo and network rects stay correct
   if (!mPoly.empty())
   {
      Point2I minPt = mPoly[0], maxPt = mPoly[0];
      for (U32 i = 1; i < mPoly.size(); ++i)
      {
         minPt.setMin(mPoly[i]);
         maxPt.setMax(mPoly[i]);
      }
      RectI aabb(minPt, maxPt - minPt);
      mArea = smMissionArea = aabb;
   }

   // If I’m on a client ghost, fire a remote call back to the server.
   if (isClientObject())
   {
      Con::executef(this, "setPolyString", mPolyString.c_str());
      return;
   }

   // On the server, mark dirty so ghosts get updated:
   inspectPostApply();
   setMaskBits(UpdateMask);
}

//------------------------------------------------------------------------------
// Simple parser: fills mPoly from mPolyString
//------------------------------------------------------------------------------
void MissionArea::parsePolyString()
{
   mPoly.clear();
   if (mPolyString.isEmpty())
      return;

   Vector<S32> vals;
   char buf[512];
   // Copy into a mutable buffer:
   dStrncpy(buf, mPolyString.c_str(), sizeof(buf));
   buf[sizeof(buf) - 1] = '\0';

   // Tokenize on any whitespace or commas:
   for (char* tok = dStrtok(buf, " \t,"); tok; tok = dStrtok(NULL, " \t,"))
      vals.push_back(dAtoi(tok));

   if (vals.size() < 6 || (vals.size() & 1))
   {
      Con::errorf(ConsoleLogEntry::General,
         "MissionArea::parsePolyString - polyString must have an even number (>=6) of integers.");
      return;
   }

   // Build the closed polygon (last point need not repeat)
   for (U32 i = 0; i + 1 < vals.size(); i += 2)
      mPoly.push_back(Point2I(vals[i], vals[i + 1]));
}

//------------------------------------------------------------------------------
// onAdd/onRemove
//------------------------------------------------------------------------------
bool MissionArea::onAdd()
{
   if (isServerObject())
   {
      if (smServerObject)
      {
         Con::errorf(ConsoleLogEntry::General,
            "MissionArea::onAdd - MissionArea already instantiated!");
         return false;
      }
      smServerObject = this;
   }

   if (!Parent::onAdd())
      return false;

   // ensure our fields are in sync
   setArea(mArea);
   return true;
}

void MissionArea::onRemove()
{
   if (smServerObject == this)
      smServerObject = NULL;
   Parent::onRemove();
}

//------------------------------------------------------------------------------
// Called when user edits fields in the inspector
//------------------------------------------------------------------------------
void MissionArea::inspectPostApply()
{
   Parent::inspectPostApply();
   // re‐parse poly if they changed the string
   parsePolyString();
   setMaskBits(UpdateMask);
}

//------------------------------------------------------------------------------
// Expose fields to script
//------------------------------------------------------------------------------
void MissionArea::initPersistFields()
{
   docsURL;
   addGroup("Dimensions");

   // legacy box
   addField("area", TypeRectI, Offset(mArea, MissionArea),
      "Four corners (X1, Y1, Width, Height) defining the level’s rectangle");

   // new poly
   addField("polyString", TypeString, Offset(mPolyString, MissionArea),
      "Space‐separated list of X Y pairs defining a polygon");

   addField("flightCeiling", TypeF32, Offset(mFlightCeiling, MissionArea),
      "Top of the mission area (Z height)");
   addField("flightCeilingRange", TypeF32, Offset(mFlightCeilingRange, MissionArea),
      "Range below ceiling before thrust cut‐off");

   endGroup("Dimensions");

   Parent::initPersistFields();
}

//------------------------------------------------------------------------------
// Net sync
//------------------------------------------------------------------------------
void MissionArea::unpackUpdate(NetConnection*, BitStream* stream)
{
   if (stream->readFlag())
   {
      mathRead(*stream, &mArea);
      stream->read(&mPolyString);
      stream->read(&mFlightCeiling);
      stream->read(&mFlightCeilingRange);

      // Re-parse the polygon on both client and server
      parsePolyString();
   }
}

U32 MissionArea::packUpdate(NetConnection*, U32 mask, BitStream* stream)
{
   if (stream->writeFlag(mask & UpdateMask))
   {
      mathWrite(*stream, mArea);
      stream->write(mPolyString);
      stream->write(mFlightCeiling);
      stream->write(mFlightCeilingRange);
   }
   return mask;
}

//------------------------------------------------------------------------------
// Engine functions
//------------------------------------------------------------------------------
DefineEngineFunction(getMissionAreaServerObject, MissionArea*, (), ,
   "Get the MissionArea object, if any.\n\n@ingroup enviroMisc")
{
   return MissionArea::getServerObject();
}

// legacy getArea
DefineEngineMethod(MissionArea, getArea, const char*, (), ,
   "Returns 4 fields: X1 Y1 Width Height.\n")
{
   const RectI a = object->getArea();
   char* buf = Con::getReturnBuffer(32);
   dSprintf(buf, 32, "%d %d %d %d", a.point.x, a.point.y, a.extent.x, a.extent.y);
   return buf;
}

// legacy setArea
DefineEngineMethod(MissionArea, setArea, void, (S32 x, S32 y, S32 w, S32 h), ,
   "@brief Defines the size of the MissionArea (legacy rect)\n")
{
   if (object->isClientObject())
   {
      Con::errorf(ConsoleLogEntry::General,
         "MissionArea::cSetArea - cannot alter client object!");
      return;
   }
   RectI r(Point2I(x, y), Point2I(w, h));
   object->setArea(r);
}

// new polyString accessors
DefineEngineMethod(MissionArea, getPolyString, const char*, (), ,
   "Returns the raw polyString defining your vertices.")
{
   return object->getPolyString().c_str();
}

//------------------------------------------------------------------------------
// Script binding
//------------------------------------------------------------------------------
DefineEngineMethod(MissionArea, setPolyString, void, (const char* str), ,
   "@brief Define a new polygon (space‐separated x y pairs).")
{
   object->setPolyString(String(str));
}
