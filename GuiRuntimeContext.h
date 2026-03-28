#pragma once

#include "ShadowSampleMisc.h"

class CascadedShadowsManager;
class CFirstPersonCamera;

struct GuiRuntimeContext
{
    CascadedShadowsManager* pCascadedShadow;
    CascadeConfig* pCascadeConfig;
    bool* pVisualizeCascades;
    bool* pVisualizeVoxel;
    bool* pMoveLightTexelSize;
    CFirstPersonCamera** ppActiveCamera;
    CFirstPersonCamera* pViewerCamera;
    CFirstPersonCamera* pLightCamera;
};
