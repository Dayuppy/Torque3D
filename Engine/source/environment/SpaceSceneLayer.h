//-----------------------------------------------------------------------------
// SpaceSceneLayer.h
//-----------------------------------------------------------------------------

#pragma once
#ifndef _SPACESCENELAYER_H_
#define _SPACESCENELAYER_H_

#include "scene/sceneObject.h"
#include "gfx/gfxShader.h"
#include "gfx/gfxVertexBuffer.h"
#include "gfx/gfxPrimitiveBuffer.h"
#include "gfx/gfxStateBlock.h"
#include "math/mPoint3.h"
#include "gfx/gfxVertexTypes.h"

//-----------------------------------------------------------------------------
// Planet vertex format
//-----------------------------------------------------------------------------
GFXDeclareVertexFormat(GFXPlanetVert)
{
    Point3F point;
    Point3F normal;
    Point3F binormal;
    Point3F tangent;
    Point2F texCoord;
};

GFXDeclareVertexFormat(GFXStarVert)
{
    Point3F position;
    Point2F texCoord;
};

//-----------------------------------------------------------------------------
// SpaceSceneLayer
//-----------------------------------------------------------------------------
class SpaceSceneLayer : public SceneObject
{
    typedef SceneObject Parent;

    GFXShaderRef            mShader;
    GFXShaderConstBufferRef mShaderConsts;

    // --- planet mesh
    GFXVertexBufferHandle<GFXPlanetVert>  mPlanetVB;
    GFXPrimitiveBufferHandle             mPlanetPB;

    // --- nebula dome
    GFXVertexBufferHandle<GFXPlanetVert>  mNebulaVB;
    GFXPrimitiveBufferHandle              mNebulaPB;

    // --- planet shader
    GFXShaderRef            mPlanetShader;
    GFXShaderConstBufferRef mPlanetConsts;
    GFXShaderConstHandle* mPlanetMVPSC;
    GFXShaderConstHandle* mPlanetEyeSC;
    GFXShaderConstHandle* mPlanetColorSC;
    GFXShaderConstHandle* mPlanetRenderSC;
    GFXStateBlockRef        mPlanetState;

    // --- sky shader (stars + nebula)
    GFXShaderRef            mStarsShader;
    GFXShaderConstBufferRef mStarsConsts;
    GFXShaderConstHandle* mModelViewProjSC;
    GFXShaderConstHandle* mEyePosWorldSC;
    GFXShaderConstHandle* mHazeIntensitySC;
    GFXShaderConstHandle* mNebulaIntensitySC;
    GFXShaderConstHandle* mTimeSC;
    GFXShaderConstHandle* mStarColorSC;
    GFXShaderConstHandle* mRenderPlanetSC;

    // --- full-screen quad for sky
    GFXVertexBufferHandle<GFXVertexPT>  mQuadVB;
    GFXPrimitiveBufferHandle           mQuadPB;

    // --- shared depth/blend state
    GFXStateBlockRef        mStateblock;

    // --- settings
    F32   mHazeIntensity;
    F32   mNebulaIntensity;
    F32   mPlanetSize;
    F32   mPlanetDistance;
    bool  mRenderPlanet;
    bool  mRenderStars;
    LinearColorF mStarColor;
    LinearColorF mPlanetColor;

    // initialization helpers
    void _initShader();
    void _initPlanetGeometry();
    void _initQuad();
    void _initNebulaGeometry();

public:
    SpaceSceneLayer();
    static void initPersistFields();
    virtual bool onAdd() override;
    virtual void onRemove() override;
    virtual void prepRenderImage(SceneRenderState* state) override;

    virtual U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream) override;
    virtual void unpackUpdate(NetConnection* conn, BitStream* stream) override;

    void renderStars(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* matInst);
    void renderPlanet(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* matInst);
    void renderNebula(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* matInst);


    DECLARE_CONOBJECT(SpaceSceneLayer);
    DECLARE_CATEGORY("Environment");
    DECLARE_DESCRIPTION("Procedurally renders a space background with stars, haze, and optional planet.");
};

#endif // _SPACESCENELAYER_H_
