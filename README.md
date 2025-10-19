# Overview

Bitloop is a modular cross-platform CMake project manager tailored toward scientific simulations/visualizations and rapid prototyping.

By default, projects contain a simple built-in Viewport and GUI, along with a handy collection of tools/libraries.

[Demo](https://bitloop.dev)

### Main Features
- Projects are modular and can include each other as dependencies in a tree structure.
  - Child projects can still be opened and compiled as standalone projects.
  - Supported platforms:  Windows, Linux, Emscripten.
- (Native only) Frame-by-frame video recording without blocking the UI.
  - Great for computationally expensive simulations (e.g. Mandelbrot zoom).

### Design Principles
- Organization
  - You can create an empty project (e.g. "Science"), and include several subprojects ("Physics", "Biology") where you can create further subprojects.
  - Any individual project can be compiled as the "root" project, so that only it's subtree of projects is included.
    - Child projects can still be selected in the GUI and viewed (but can optionally be hidden by the parent)
  - Create new projects on the fly with the command-line tool, and avoid cluttering up your main build tree.
- Building
  - You can build any collection of projects (e.g. just "Biology") into a single final executable.
- Resource Management
  - All project data (images, audio, etc) are bundled into the final executable automatically.
- Library Management
  - Bitloop projects use vcpkg as a package manager.
  - Each project has it's own vcpkg.json, allowing you to grab only the libraries you need for your current project.

### Layout
<img src="https://i.imgur.com/vqsZn3m.png" width="700">

For more help getting started, view the Wiki:

- [Building from source on Windows / Linux](https://github.com/willmh93/bitloop/wiki/Build-library-from-source)
- [Building with Emscripten](https://github.com/willmh93/bitloop/wiki/Build-library-with-Emscripten)
- [Creating a standalone project with the command-line tool](https://github.com/willmh93/bitloop/wiki/Creating-a-new-project-(CLI))

## Development progress

### Building

|                                                                      | Windows                  | Linux                   
| -------------------------------------------------------------------- | ------------------------ | ------------------------
| Modular nested projects                                              | :heavy_check_mark:       | :heavy_check_mark:      
| Building standalone projects (with child projects as dependencies)   | :heavy_check_mark:       | :heavy_check_mark:      
| Command-line tools for projects                                      | :heavy_check_mark:       | :heavy_check_mark:      
| C++23 support                                                        | :heavy_check_mark:       | :heavy_check_mark:    
| Visual Studio                                                        | :heavy_check_mark:       | :heavy_check_mark:    
| Visual Studio Code                                                   | partial                  | partial    

### Architecture

|                                           | Windows                  | Linux                    | Web (Emscripten)                                          |
| ----------------------------------------- | ------------------------ | ------------------------ | --------------------------------------------------------- |
| Multithreading                            | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark: (pthreads with COOP/COEP Headers)      |
| 128-bit floating point support (wip)      | :hourglass:              | :hourglass:              | :hourglass:                                               |

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
| Custom-resolution image renders           | N/A                      | N/A                      | N/A                                                       |
| FFmpeg recording                          | N/A                      | N/A                      | N/A                                                       |

### Other features

|                                           | Windows                  | Linux                    | Web (Emscripten)                                          |
| ----------------------------------------- | ------------------------ | ------------------------ | --------------------------------------------------------- |
| Timeline support                          | N/A                      | N/A                      | N/A                                                       |
