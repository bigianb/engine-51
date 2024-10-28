![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/bigianb/engine-51/cmake-multi-platform.yml)

# What is this?

Some tooling to work with the Area 51 game from 2005.
Very experimental.

# How to build

Install Qt 6 and run cmake. Nothing special - works in a vscode cmake environment just fine.

Github actions build for winodws, linux and macos so you can always look at the workflow there to troubleshoot any issues you see locally.

## Specific windows guide
This assumes you have Qt 6.8.0 installed in c:\Qt, you have visual studio 2022 installed and you have the source checked out in c:\dev\engine-51. If your paths are different, just adjust any paths below as needed.

```
cd c:\dev\engine-51
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
```
This should create a Visual Studio solution in the build directory.
Open the Engine51.sln file in Visual Studio.
In Visual Studio, right click DFSViewer in the Solution Explorer and select 'set as startup project' from the context menu.

Select RelWithDbgInfo as the target in the tool bar and then click the outline green arrow (ctrl+f5) to build and run the exe without the debugger. You can run with the local debugger - it will just be a bit slower.

Now, drop a DFS file onto the running application window.

# Tools

## DFSViewer
Drag/Drop a DFS file onto the app. Left splitter panel will show contents and file sizes. Click a filename to see a preview in the right hand panel.
In the right hand pane switch between a text description and a 3d view (not yet implemented)
