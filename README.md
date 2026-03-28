# d11cascadedDemo
A base DX11 example using a shadow map can be modified to support cascade debugging quite effectively

## GUI improvements

The sample now includes an improved runtime GUI with separate categories for debug, voxelization, and cascade controls.

<img width="2555" height="1361" alt="image" src="https://github.com/user-attachments/assets/e7fb9cb2-83e5-4ad7-a13d-06ba2d48a5d8" />

#voxelization process

A static scene voxelization pipeline was implemented in Direct3D 11 by discretizing scene geometry inside a global scene AABB into a regular 3D grid. The voxelization stage stores occupancy and albedo data into RWTexture3D resources, and also builds a compact GPU-side list of occupied voxels through an AppendStructuredBuffer, avoiding the need to iterate over empty cells during visualization.

The voxel volume is generated through a VS + GS + PS pipeline. Scene geometry is projected onto the voxel grid axes, transformed from world space into discrete voxel coordinates (x, y, z), and written into the 3D voxel textures using atomic operations for occupancy. For visualization, the original mesh is no longer reused. Instead, one cube is rendered per occupied voxel using DrawInstancedIndirect. In the vertex shader, each voxel coordinate is unpacked and converted back into its world-space bounds using the voxel AABB and grid resolution, allowing the cube instance to be positioned correctly in the scene. This produces an explicit voxel-based representation of the scene, with color fetched directly from the voxel volume and face-based shading applied in the pixel shader.



<img width="2546" height="1087" alt="image" src="https://github.com/user-attachments/assets/9352f5b9-a326-49b4-b4a8-f5edf7e35194" />


This implementation is built on top of the DirectX 11 sample browser examples in order to enable rapid prototyping and iteration. Therefore, the codebase is primarily intended for educational and experimental purposes, and should not be considered production-ready.
