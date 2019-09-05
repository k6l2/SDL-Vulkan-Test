// Using code from post about SDL2 + Vulkan:
//	https://www.gamedev.net/forums/topic/699117-vulkan-with-sdl2-getting-started/
// And following the tutorial located at:
//	https://vulkan-tutorial.com
#include "RenderWindow.h"
#include "GfxProgram.h"
k10::RenderWindow* renderWindow = nullptr;
k10::GfxProgram* gProgVert = nullptr;
k10::GfxProgram* gProgFrag = nullptr;
k10::GfxPipelineIndex gGpi;
void cleanup()
{
	if (renderWindow)
	{
		renderWindow->waitForOperationsToFinish();
	}
	if (gProgVert)
	{
		delete gProgVert;
		gProgVert = nullptr;
	}
	if (gProgFrag)
	{
		delete gProgFrag;
		gProgFrag = nullptr;
	}
	if (renderWindow)
	{
		delete renderWindow;
		renderWindow = nullptr;
	}
	SDL_Quit();
}
int main(int argc, char** argv)
{
	bool exit = false;
	SDL_Event event;
	if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO) != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to init SDL! error='%s'\n", SDL_GetError());
		return EXIT_FAILURE;
	}
#ifndef NDEBUG
	SDL_LogSetPriority(SDL_LOG_CATEGORY_VIDEO, SDL_LOG_PRIORITY_DEBUG);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_DEBUG);
#endif
	renderWindow = k10::RenderWindow::createRenderWindow("SDL-Vulkan-Test", 1280, 720);
	if (!renderWindow)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, 
			"Failed to create RenderWindow!\n");
		cleanup();
		return EXIT_FAILURE;
	}
	gProgVert = renderWindow->createGfxProgram(k10::GfxProgram::ShaderType::VERTEX);
	SDL_assert(gProgVert);
	gProgFrag = renderWindow->createGfxProgram(k10::GfxProgram::ShaderType::FRAGMENT);
	SDL_assert(gProgFrag);
	if (!gProgVert->loadFromFile("shader-bin/simple-draw-tri-vert.spv"))
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR,
			"Failed to load vertex shader!\n");
		cleanup();
		return EXIT_FAILURE;
	}
	if (!gProgFrag->loadFromFile("shader-bin/simple-draw-tri-frag.spv"))
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR,
			"Failed to load fragment shader!\n");
		cleanup();
		return EXIT_FAILURE;
	}
	gGpi = renderWindow->createGfxPipeline(gProgVert, gProgFrag);
	if (gGpi == k10::RenderWindow::MAX_PIPELINES)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR,
			"Failed to create gfx pipeline!\n");
		cleanup();
		return EXIT_FAILURE;
	}
	if (!renderWindow->recordCommandBuffers(gGpi))
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR,
			"Failed to record command buffers!\n");
		cleanup();
		return EXIT_FAILURE;
	}
	while (!exit)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EventType::SDL_WINDOWEVENT:
				switch (event.window.windowID)
				{
				case SDL_WINDOWEVENT_RESIZED:
					renderWindow->onResized();
					break;
				}
				break;
			case SDL_EventType::SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					exit = true;
					break;
				}
				break;
			case SDL_EventType::SDL_QUIT:
				exit = true;
				break;
			}
		}
		if (exit)
		{
			break;
		}
		if (!renderWindow->drawFrame())
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "drawFrame failure!\n");
			cleanup();
			return EXIT_FAILURE;
		}
	}
	cleanup();
	return EXIT_SUCCESS;
}