Libs: D3DCompiler.lib; d3d12.lib; dxgi.lib; dxguid.lib;
Custom Build Steps: copy /y $(ProjectDir)External\dxcompiler\*.dll $(OutDir) >nul