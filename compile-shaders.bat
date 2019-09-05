mkdir shader-bin
%VULKAN_SDK%\Bin32\glslc.exe shaders\simple-draw.vert -o shader-bin\simple-draw-vert.spv
%VULKAN_SDK%\Bin32\glslc.exe shaders\simple-draw.frag -o shader-bin\simple-draw-frag.spv