#pragma once
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL2_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl2.h"

struct nk_context* ctx;
struct nk_colorf bg;
SDL_Window* MainWindow;
void InitMenu(SDL_Window* window)
{
	MainWindow = window;
	ctx = nk_sdl_init(MainWindow);
	{
		struct nk_font_atlas* atlas;
		nk_sdl_font_stash_begin(&atlas);
		struct nk_font* roboto = nk_font_atlas_add_from_file(atlas, "C:/Windows/Fonts/segoeui.ttf", 25, 0);
		nk_sdl_font_stash_end();
		nk_style_load_all_cursors(ctx, atlas->cursors);
		nk_style_set_font(ctx, &roboto->handle);
	}
	//SDL_SetWindowPosition(window, xPos, yPos);
	//SDL_SetWindowSize(window, 500, 300);
}

void RenderMenu()
{
	SDL_Event evt;
	nk_input_begin(ctx);
	while (SDL_PollEvent(&evt))
	{
		if (evt.type == SDL_QUIT) ExitProcess(EXIT_SUCCESS);
		nk_sdl_handle_event(&evt);
	}
	nk_input_end(ctx);
	int WindowWidth, WindowHeight;
	SDL_GetWindowSize(MainWindow, &WindowWidth, &WindowHeight);
	if (nk_begin(ctx, "My pMenu", nk_rect(0, 0, WindowWidth, WindowHeight), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row_dynamic(ctx, 25, 2);
		nk_label(ctx, "Hello", NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 35, 1);
		nk_button_label(ctx, "Single player");
		nk_button_label(ctx, "Multiplayer");
		nk_button_label(ctx, "Options");
		nk_button_label(ctx, "Exit");
	}
	nk_end(ctx);

	//GL_Clear(GL_COLOR_BUFFER_BIT, NULL, 1.0f, 0);
	nk_sdl_render(NK_ANTI_ALIASING_OFF);
	//SDL_GL_SwapWindow(MainWindow);
}