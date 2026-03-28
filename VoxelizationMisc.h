#pragma once

#include <D3DCompiler.h>
#include <xnamath.h>
#include <DXUT.h>

#if defined(_MSC_VER)
#define ALIGN16 __declspec(align(16))
#elif defined(__GNUC__) || defined(__clang__)
#define ALIGN16 __attribute__((aligned(16)))
#else
#include <cstddef>
#define ALIGN16 alignas(16)
#endif


#define GRID_SIZE_X 256
#define GRID_SIZE_Y 256
#define GRID_SIZE_Z 256

#define DYNAMIC_GRID_SIZE_X 128
#define DYNAMIC_GRID_SIZE_Y 128
#define DYNAMIC_GRID_SIZE_Z 128



ALIGN16 struct CB_VOXELIZATION_PARAMS
{
    D3DXVECTOR3 gVoxelMin;
    UINT32      gGridSizeX;

    D3DXVECTOR3 gVoxelMax;
    UINT32      gGridSizeY;

    UINT32      gGridSizeZ;
    FLOAT       gHeightWarp;
    FLOAT       gXYFootprintScale;
    FLOAT       gYZFootprintScale;
    FLOAT       _padding;
};

ALIGN16 struct CB_VISUALIZE_VOXELS
{
    D3DXMATRIX  gWorldViewProj;
    D3DXMATRIX  gWorld;

    D3DXVECTOR3 gStaticVoxelMin;
    FLOAT       gOpacity;

    D3DXVECTOR3 gStaticVoxelMax;
    FLOAT       gStaticHeightWarp;

    UINT32      gStaticGridSizeX;
    UINT32      gStaticGridSizeY;
    UINT32      gStaticGridSizeZ;
    FLOAT       gSurfaceSnap;

    D3DXVECTOR3 gLightDir;
    FLOAT       gPadding1;
};
