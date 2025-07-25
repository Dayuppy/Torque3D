#ifndef _GUI_RADAR_TRACKER_CTRL_H_
#define _GUI_RADAR_TRACKER_CTRL_H_

#include "gui/core/guiControl.h"
#include "scene/sceneObject.h"
#include "gfx/gfxDrawUtil.h"
#include "platform/platformTimer.h"

class GuiRadarTrackerCtrl : public GuiControl, public ITickable
{
    typedef GuiControl Parent;

public:
    GuiRadarTrackerCtrl();

    void onRender(Point2I offset, const RectI& updateRect) override;
    bool onWake() override;
    void onSleep() override;
    void onPreRender() override;
    void processTick() override;
    void advanceTime(F32 timeDelta) override;

    DECLARE_CONOBJECT(GuiRadarTrackerCtrl);

    void addTrackedObject(SceneObject* obj);
    void removeTrackedObject(SceneObject* obj);
    void clearTrackedObjects();
    void setCenterObject(SceneObject* obj);

    F32 getPlayerYawDegrees() const;

    void setRadarMode(U32 mode);
    void setZoom(F32 zoom);
    void setScanSpeed(F32 degPerSec);
    void setPulseSpeed(F32 metersPerSec);
    void setSweepTrail(F32 arcDegrees, U32 steps);

protected:
    enum RadarMode { Pulse = 0, Sweep = 1 };

    struct TrackedObject { SceneObject* object; };
    struct RadarContact
    {
        Point3F worldPos;
        F32 alpha;
        U32 lastSeen;
        bool updated; // Indicates if updated this scan
    };

    struct SweepMark {
        F32 angle;   // mScanAngle at the moment of capture
        F32 age;     // seconds since we captured it
    };

    Vector<SweepMark> mSweepHistory;
    F32               mSweepFadeTime;  // how long (in seconds) the trail remains

    Vector<TrackedObject> mTrackedObjects;
    Vector<RadarContact> mContacts;

    SceneObject* mCenterObject;

    F32 mZoom;
    F32 mScanAngle;
    F32 mScanSpeed;
    F32 mPulseSpeed;
    F32 mUpdateRateSec;
    F32 mUpdateTimer;

    RadarMode mRadarMode;
    U32 mLastRenderTime;

    // Sweep fade
    F32 mSweepTrailArc;
    U32 mSweepTrailSteps;

    // Pulse ring memory
    struct PulseRing
    {
        F32 radius;
        F32 alpha;
    };

    PulseRing mPulseRing;

    void updateTrackedObjects();
    void renderBlips(const RectI& bounds, const Point2I& origin);
    void renderGrid(const RectI& bounds, const Point2I& origin);
    void renderPulse(const RectI& bounds, const Point2I& origin);
    void renderSweep(const RectI& bounds, const Point2I& origin);
    Point2F worldToRadar(const Point3F& pos, const Point3F& center, const MatrixF& xf);

    // Inherited via ITickable
    void interpolateTick(F32 delta) override;
};

#endif
