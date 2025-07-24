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

#ifndef _MISSIONAREA_H_
#define _MISSIONAREA_H_

#ifndef _NETOBJECT_H_
#include "sim/netObject.h"
#endif
#ifndef _MPOINT2_H_
#include "math/mPoint2.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif

/// MissionArea defines a 2D polygonal boundary for the level.
class MissionArea : public NetObject
{
protected:
   typedef NetObject Parent;

   /// Polygon vertices in 2D (X,Y)
   Vector<Point2I> mPoly;
   /// Raw string of "x1 y1 x2 y2 ..."
   

   /// Legacy rectangular area (for editor compatibility)
   RectI mArea;

   F32 mFlightCeiling;
   F32 mFlightCeilingRange;

   static MissionArea* smServerObject;

public:
   MissionArea();

   static RectI smMissionArea;

   /// Get the server-side singleton
   static MissionArea* getServerObject() { return smServerObject; }

   /// Retrieves the current polygon vertex list
   const Vector<Point2I>& getPoly() const { return mPoly; }
   /// Retrieves the raw polygon string
   const String& getPolyString() const { return mPolyString; }
   /// Sets the raw polygon string; parses internally
   void setPolyString(const String& poly);

   /// Legacy rectangular accessors
   const RectI& getArea() const { return mArea; }
   void setArea(const RectI& area);
   static const RectI& getMissionAreaRect() { return smMissionArea; }

   /// Flight ceiling altitude
   F32 getFlightCeiling() const { return mFlightCeiling; }
   F32 getFlightCeilingRange() const { return mFlightCeilingRange; }

   /// SimObject overrides
   bool onAdd() override;
   void onRemove() override;
   void inspectPostApply() override;
   static void initPersistFields();

   /// NetObject overrides
   enum NetMaskBits {
      UpdateMask = BIT(0)
   };
   U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream) override;
   void unpackUpdate(NetConnection* conn, BitStream* stream) override;

   DECLARE_CONOBJECT(MissionArea);

   String mPolyString;

   /// Parse mPolyString into mPoly
   void parsePolyString();
};

#endif // _MISSIONAREA_H_
