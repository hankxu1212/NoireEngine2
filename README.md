# Noire Engine 2: A Realtime Hybrid Rendering Engine in Vulkan and C++20
Noire Engine 2 supports a Forward+ rasterization pipeline, carrying a small compressed G-buffer, into a RTX-powered realtime ray tracing pipeline.

## Core Features
- Ray Traced Reflections
- Compute-driven Ray Traced Ambience Occlusion
- Physically-Based Rendering
- Image-Based Lighting
- Parallax Occlusion Mapping
- Analytical Lights: Directional, Spot, Point
- Multi-threaded Shadow Mapping
- Percentage-Closer Soft Shadows
- Cascaded Shadow Maps
- Omni-directional Shadow Maps
- ImGui-based UI and Scene Hierarchy
- Entity-Component System
- Physically-Based Bloom

## Upcoming Features
- Tile-based Clustered Light Culling
- Compute-based Culling and Depth Pyramids

## Showcase

![alt text](docs/AO.png)
<p>
    <img src="docs/AO-AO.png" alt="drawing" width="200"/>
    <em>AO Map</em>
<p>

<p>
    <img src="docs/AO-Color.png" alt="drawing" width="200"/>
    <em>Direct Lighting and Color Map</em>
<p>

<p>
    <img src="docs/AO-HDR.png" alt="drawing" width="200"/>
    <em>Emission Map</em>
<p>

<p>
    <img src="docs/AO-PosN.png" alt="drawing" width="200"/>
    <em>Compressed G Buffer</em>
<p>

<p>
    <img src="docs/AO-Reflection.png" alt="drawing" width="200"/>
    <em>Reflection Map (Lambertian Materials are not reflective)</em>
<p>

![alt text](docs/Bloom.png)
![alt text](docs/Reflection.png)
![alt text](docs/library.png)
