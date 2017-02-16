#pragma once

#include <SDL.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear.h>

enum nk_theme { NK_THEME_BLACK, NK_THEME_WHITE, NK_THEME_RED, NK_THEME_BLUE, NK_THEME_DARK };

NK_API struct nk_context *nk_sdl_init( SDL_Window *win );
NK_API void nk_sdl_font_stash_begin( struct nk_font_atlas **atlas );
NK_API void nk_sdl_font_stash_end( void );
NK_API int nk_sdl_handle_event( SDL_Event *evt );
NK_API void nk_sdl_render( enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer );
NK_API void nk_sdl_shutdown( void );
NK_API void nk_sdl_device_destroy( void );
NK_API void nk_sdl_device_create( void );
NK_API void nk_set_style( struct nk_context *ctx, enum nk_theme theme );