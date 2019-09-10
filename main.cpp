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
	if (!gProgVert->loadFromFile("shader-bin/simple-draw-vert.spv"))
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR,
			"Failed to load vertex shader!\n");
		cleanup();
		return EXIT_FAILURE;
	}
	if (!gProgFrag->loadFromFile("shader-bin/simple-draw-frag.spv"))
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
	const int NUM_QUADS = 1000000;
	const int QUAD_ROWS = (int)sqrt(NUM_QUADS);
	const int QUAD_COLS = (int)sqrt(NUM_QUADS);
	const float QUAD_W = 2.f / QUAD_ROWS;
	const float QUAD_H = 2.f / QUAD_ROWS;
	for (int i = 0; i < NUM_QUADS; i++)
	{
		const int row = i / QUAD_COLS;
		const int col = i % QUAD_COLS;
		const float quadLeft = -1 + col * QUAD_W;
		const float quadTop  = -1 + row * QUAD_H;
		const float quadRight  = quadLeft + QUAD_W;
		const float quatBottom = quadTop  + QUAD_H;
		const vector<k10::Vertex> vertices = {
			{{quadLeft , quatBottom}, {1.f, 0.f, 0.f, 1.f}},
			{{quadLeft , quadTop   }, {1.f, 1.f, 0.f, 1.f}},
			{{quadRight, quadTop   }, {0.f, 0.f, 1.f, 1.f}},
			{{quadRight, quadTop   }, {0.f, 0.f, 1.f, 1.f}},
			{{quadRight, quatBottom}, {0.f, 1.f, 0.f, 1.f}},
			{{quadLeft , quatBottom}, {1.f, 0.f, 0.f, 1.f}}
		};
		const k10::QuadPool::QuadId testQuadId =
			renderWindow->getQuadPool().addQuad(vertices);
		SDL_assert(testQuadId != numeric_limits<k10::QuadPool::QuadId>::max());
	}
	if (!renderWindow->recordCommandBuffers(gGpi))
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR,
			"Failed to record command buffers!\n");
		cleanup();
		return EXIT_FAILURE;
	}
	std::chrono::time_point<std::chrono::high_resolution_clock> frameTimePointPrev =
		std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> frameAccumulator = 
		std::chrono::duration<double>(0);
	while (!exit)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EventType::SDL_WINDOWEVENT:
				renderWindow->onWindowEvent(event.window);
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
		const std::chrono::time_point<std::chrono::high_resolution_clock> now =
			std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> frameDelta =
			std::chrono::duration_cast<std::chrono::duration<double>>(
				now - frameTimePointPrev);
		frameTimePointPrev = now;
		frameAccumulator += frameDelta;
		int logicTicks = 0;
		while (frameAccumulator >= k10::FIXED_SECONDS_PER_FRAME)
		{
			///TODO: @fix-your-timestep
			///	previousState = currentState;
			// MAIN LOOP LOGIC //
			{
			}
			logicTicks++;
			frameAccumulator -= k10::FIXED_SECONDS_PER_FRAME;
		}
		///TODO: @fix-your-timestep
		///	const float interFrameRatio = 
		///		frameAccumulator.count() / 
		///			k10::FIXED_SECONDS_PER_FRAME.count();
		///	const state = previousState*(1 - interFrameRatio) + 
		///				  currentState*interFrameRatio
		if (!renderWindow->drawFrame())
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "drawFrame failure!\n");
			cleanup();
			return EXIT_FAILURE;
		}
		if (logicTicks >= 1)
		{
			SDL_Log("ms=%lf l=%i\n",
				std::chrono::duration_cast<std::chrono::duration<double,
					std::milli>>(frameDelta).count(),
				logicTicks);
		}
	}
	cleanup();
	return EXIT_SUCCESS;
}