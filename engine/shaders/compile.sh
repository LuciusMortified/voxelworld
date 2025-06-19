#!/bin/bash

# Компилируем vertex shader
glslc voxel.vert -o voxel_vert.spv
if [ $? -eq 0 ]; then
    echo "Vertex shader compiled successfully"
else
    echo "Failed to compile vertex shader"
    exit 1
fi

# Компилируем fragment shader
glslc voxel.frag -o voxel_frag.spv
if [ $? -eq 0 ]; then
    echo "Fragment shader compiled successfully"
else
    echo "Failed to compile fragment shader"
    exit 1
fi

echo "All shaders compiled successfully!"