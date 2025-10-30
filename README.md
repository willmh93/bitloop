# Overview

Bitloop is a modular cross-platform CMake project manager tailored toward scientific simulations/visualizations and rapid prototyping.

[Web Demo](https://bitloop.dev)

For help getting started, view the Wiki:

- Building projects
  - [Building from source on Windows / Linux](https://github.com/willmh93/bitloop/wiki/Build-library-from-source) (todo: update)
  - [Building with Emscripten](https://github.com/willmh93/bitloop/wiki/Build-library-with-Emscripten) (todo: update)
  - [Creating a standalone project with the command-line tool](https://github.com/willmh93/bitloop/wiki/Creating-a-new-project-(CLI)) (todo: update)
- Tutorials (todo):
  - Creating a simple "Hello World" project
  - Using ImGui to control Scene variables (safely with multithreading)
  - User input and world navigation

### Main Features
- Projects are modular and can include each other as dependencies in a tree structure
  - Child projects can be opened and compiled as standalone projects
  - Cross-platform:  Windows, Linux, Emscripten
  - Designed for heavy per‑frame computations (e.g. Mandelbrot zoom)
- C++ NanoVG wrapper with integrated 128-bit float support (**f128** class)
  - If you have prior experience with HTML Canvas drawing, this should feel second-nature to you
- FFmpeg video recording without blocking the UI (Desktop only)
- WebP snapshots / animations (all platforms).

### Design Principles
- Organization
  - Using CMake, you can create an empty bitloop project (e.g. "Science"), and include several subprojects ("Physics", "Biology") where you can create further subprojects
  - You can build any subtree of projects (e.g. just "Biology") in to a single final executable
  - All project data (images, audio, etc) are bundled into the final executable automatically
- Library Management
  - All bitloop projects use vcpkg as a package manager (vcpkg is fetched automatically for the current workspace)
  - Shared binaries - all projects in a workspace will share the same vcpkg instance and binaries to prevent unnecessary rebuilds
    - To create a workspace, simply add a "**.bitloop-workspace**" file in a project directory.
      - For the root workspace folder, add:  "**root=TRUE**"
      - For child workspaces, add: "**root=FALSE**"
    - vcpkg will be automatically cloned in to the highest level folder containing a "**.bitloop-workspace**" file (but not above the root workspace)

### Layout
By default, projects contain a simple built-in viewport and sidebar for custom ImGui controls, along with a handy collection of tools/libraries.

<img src="https://i.imgur.com/vqsZn3m.png" width="700">

## Development progress

### Building

|                                                                      | Windows                  | Linux                   
| -------------------------------------------------------------------- | ------------------------ | ------------------------
| Modular nested projects                                              | :heavy_check_mark:       | :heavy_check_mark:      
| Standalone projects (with child projects as dependencies)            | :heavy_check_mark:       | :heavy_check_mark:      
| C++23 support                                                        | :heavy_check_mark:       | :heavy_check_mark:    
| Visual Studio                                                        | :heavy_check_mark:       | :heavy_check_mark:    
| Visual Studio Code                                                   | partial                  | partial    

### Architecture

|                                           | Windows                  | Linux                    | Web (Emscripten)                                          |
| ----------------------------------------- | ------------------------ | ------------------------ | --------------------------------------------------------- |
| Multithreading                            | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark: (pthreads with COOP/COEP Headers)      |
| 128-bit float support (f128)              | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:                                        |

### Graphics

|                                           | Windows                  | Linux                    | Web (Emscripten)                                          |
| ----------------------------------------- | ------------------------ | ------------------------ | --------------------------------------------------------- |
| High-DPI support                          | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:                                        |
| Mulitple Viewports (Split Screen)         | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:                                        |
| 2D Viewport (NanoVG wrapper)              | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:                                        |
| 3D Viewport                               | N/A                      | N/A                      | N/A                                                       |

### Recording / Image Capture

|                                           | Windows                  | Linux                    | Web (Emscripten)                                          |
| ----------------------------------------- | ------------------------ | ------------------------ | --------------------------------------------------------- |
| Custom-resolution image renders           | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:                                        |
| FFmpeg recording                          | :heavy_check_mark:       | :heavy_check_mark:       | N/A                                                       |
| WEBP animations                           | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:                                        |

### Other planned features

|                                             | Windows                  | Linux                    | Web (Emscripten)                                          |
| ------------------------------------------- | ------------------------ | ------------------------ | --------------------------------------------------------- |
| Timeline (keyframed variables, tweens, etc) | N/A                      | N/A                      | N/A                                                       |
