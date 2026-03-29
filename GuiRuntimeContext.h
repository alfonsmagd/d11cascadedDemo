#pragma once

#include "ShadowSampleMisc.h"

class CascadedShadowsManager;
class CFirstPersonCamera;
class ISceneMesh;

struct GuiRuntimeContext
{
    CascadedShadowsManager* pCascadedShadow;
    CascadeConfig* pCascadeConfig;
    bool* pVisualizeCascades;
    bool* pVisualizeVoxel;
    bool* pMoveLightTexelSize;
    SCENE_SELECTION* pSelectedScene;
    CFirstPersonCamera** ppActiveCamera;
    CFirstPersonCamera* pViewerCamera;
    CFirstPersonCamera* pLightCamera;
};
