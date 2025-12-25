# UNMANNED STARRY SKY - Real-time Black Hole Rendering

![Screenshot](docs/blackhole-screenrecord.gif)

A real-time black hole visualization using ray marching in OpenGL/GLSL. Features include gravitational lensing, accretion disk rendering with volumetric noise, bloom post-processing, and an orbiting satellite.

## Features

- **Gravitational Lensing**: Physically-based light bending around the black hole using Schwarzschild metric approximation
- **Accretion Disk**: Volumetric rendering with simplex noise for realistic appearance
- **Spacetime Curvature Grid**: Bézier surface visualization of gravity well distortion around the black hole
- **Bloom Effect**: Multi-pass Gaussian bloom for HDR glow effects
- **Satellite Model**: Procedurally generated 3D satellite with elliptical orbit and animated indicator lights
- **Professional HUD**: Real-time telemetry display including distance, time dilation, and gravitational force
- **Autopilot Camera**: Smooth Bézier curve camera animation (press `C` to toggle)
- **Tone Mapping**: ACES filmic tone mapping with gamma correction
- **Lens Flare**: Cinematic lens flare and vignette effects

## Tech Stack

- **C++17** with OpenGL 3.3+ / GLSL 330
- **GLFW** - Window and input management
- **GLEW** - OpenGL extension loading
- **GLM** - Mathematics library (matrices, vectors)
- **Dear ImGui** - Debug GUI (disabled by default)
- **stb_image** - Image loading
- **Conan** - Dependency management

## Prerequisites

- [CMake](https://cmake.org/) 3.5+
- [Conan](https://conan.io/) package manager (version 1.x recommended)[^1][^2]
- OpenGL 3.3+ compatible GPU

[^1]: You might need to configure [$HOME/.conan/conan.conf](https://docs.conan.io/en/latest/reference/config_files/conan.conf.html) and Conan [profiles](https://docs.conan.io/en/latest/reference/profiles.html) if the `default profile` is not generated due to different build environments on your distribution.
[^2]: Conan 1.x instead of Conan 2.x or higher is suggested to avoid unnecessary problems.

## Building

```bash
# Configure the project and generate a native build system
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build

# Compile/build the project
cmake --build build
```

## Running

```bash
# Run the executable from the build directory
./build/Blackhole
```

## Controls

| Key | Action |
|-----|--------|
| `C` | Toggle autopilot camera animation |
| `ESC` | Exit the application |
| Mouse | Control camera view (when autopilot is off) |

## Project Structure

```
├── src/                    # C++ source files
│   ├── main.cpp            # Main application and render loop
│   ├── render.cpp/h        # Framebuffer and render utilities
│   ├── shader.cpp/h        # Shader compilation
│   └── texture.cpp/h       # Texture loading
├── shader/                 # GLSL shaders
│   ├── blackhole_main.frag # Ray marching + gravitational lensing
│   ├── satellite.*         # Satellite rendering with PBR lighting
│   ├── grid.*              # Bézier surface spacetime curvature grid
│   ├── bloom_*.frag        # Bloom post-processing pipeline
│   └── tonemapping.frag    # ACES tone mapping
└── assets/                 # Skybox textures and color maps
```

## Acknowledgements

**Papers**

- Gravitational Lensing by Spinning Black Holes in Astrophysics, and in the Movie Interstellar
- Trajectory Around A Spherically Symmetric Non-Rotating Black Hole - Sumanta
- Approximating Light Rays In The Schwarzschild Field - O. Semerak
- Implementing a Rasterization Framework for a Black Hole Spacetime - Yoshiyuki Yamashita

<!-- https://arxiv.org/pdf/1502.03808.pdf -->
<!-- https://arxiv.org/pdf/1109.0676.pdf -->
<!-- https://arxiv.org/pdf/1412.5650.pdf -->
<!-- https://pdfs.semanticscholar.org/56ff/9c575c29ae8ed6042e23075ff0ca00031ccc.pdfhttps://pdfs.semanticscholar.org/56ff/9c575c29ae8ed6042e23075ff0ca00031ccc.pdf -->

**Articles**

- Physics of oseiskar.github.io/black-hole - https://oseiskar.github.io/black-hole/docs/physics.html
- Schwarzschild geodesics - https://en.wikipedia.org/wiki/Schwarzschild_geodesics
- Photons and black holes - https://flannelhead.github.io/posts/2016-03-06-photons-and-black-holes.html
- A real-time simulation of the visual appearance of a Schwarzschild Black Hole - http://spiro.fisica.unipd.it/~antonell/schwarzschild/
- Ray Tracing a Black Hole in C# by Mikolaj Barwicki - https://www.codeproject.com/Articles/994466/Ray-Tracing-a-Black-Hole-in-Csharp
- Ray Marching and Signed Distance Functions - http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/
- Einstein's Rings and the Fabric of Space - https://www.youtube.com/watch?v=Rl8H4XEs0hw)
- Opus 2, GLSL ray tracing tutorial - http://fhtr.blogspot.com/2013/12/opus-2-glsl-ray-tracing-tutorial.html
- Ray Tracing in One Weekend - https://raytracing.github.io/
- On ray casting, ray tracing, ray marching and the like - http://hugi.scene.org/online/hugi37/- hugi%2037%20-%20coding%20adok%20on%20ray%20casting,%20ray%20tracing,%20ray%20marching%20and%20the%20like.htm

**Other GitHub Projects**

- https://github.com/sirxemic/Interstellar
- https://github.com/ssloy/tinyraytracer
- https://github.com/RayTracing/raytracing.github.io
- https://awesomeopensource.com/projects/raytracing
- Ray-traced simulation of a black hole - https://github.com/oseiskar/black-hole
- Raytracing a blackhole - https://rantonels.github.io/starless/
- https://github.com/rantonels/schwarzschild
