mkdir shader-bin
%VULKAN_SDK%\Bin32\glslc.exe shaders\simple-draw-tri.vert -o shader-bin\simple-draw-tri-vert.spv
%VULKAN_SDK%\Bin32\glslc.exe shaders\simple-draw-tri.frag -o shader-bin\simple-draw-tri-frag.spv