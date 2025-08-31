![Alt text](https://media.licdn.com/dms/image/v2/D4E22AQEx_V2r_tCL9Q/feedshare-shrink_2048_1536/B4EZgzGU41GwAM-/0/1753203959456?e=1759363200&v=beta&t=0WYfEpiDx2wIJKoBrA2CHIYh24YjAvgHfRm3dQ5VjQ8)

# Lux
Lux — Vulkan Engine / Project

Lux is an experimental, learning-focused rendering engine and small game framework built to explore modern rendering techniques using Vulkan.
The goal is to learn low-level graphics APIs, modern GPU workflows, and to build a small engine/demonstration using features such as PBR, global illumination, and realistic effects.

# Highlights / Project purpose

Learn and implement low-level Vulkan concepts: command buffers, synchronization, descriptor sets, pipeline creation, memory management, and validation layers.

Implement modern rendering techniques: clustered deferred shading, PBR surface shading, ray tracing (future).

Use a modular, ECS-driven design for scene and game logic.

Build a custom RHI abstraction over Vulkan to manage resources, pipelines, and device objects cleanly to make it API-independent.

Provide tooling (scene editor, JSON-based scene storage) for content creation.

# What’s already done
[x] PBR surface shading
[x] Custom RHI abstraction
[x] Basic ECS
[x] Clustered deferred rendering
[x] Camera
[x] GLTF model loader

# Building
To build and run the project, first of all clone this repository using --recursive flag:<br>

    git clone https://github.com/Shinkeys/Lux --recursive

Before going to the next step ensure Vulkan SDK is configured in your environment

Example of building and compilation with CMake, Ninja:

    mkdir build
    cmake build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++
    cd build
    ninja

Execute project via:

    ./build/Lux