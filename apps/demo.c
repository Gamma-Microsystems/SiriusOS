/**
 * @brief Simple executable that was used during initial testing of kSir.
 *
 * @copyright
 * This file is part of SiriusOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2021 K. Lange
 * Copyright (C) 2024 Gamma Microsystems
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <syscall.h>
#include <sys/time.h>
#include <sys/utsname.h>

#include <sirius/graphics.h>
#include <sirius/text.h>

static sprite_t wallpaper;
static struct TT_Font * _tt_font_thin;
static struct tm * timeinfo;
static gfx_context_t * ctx;

static void redraw(void) {
	draw_sprite(ctx, &wallpaper, 0, 0);
	struct utsname u;
	uname(&u);

	char string[1024];
	snprintf(string, 1024,
		"SiriusOS %s %s %s",
		u.release, u.version, u.machine);
	tt_draw_string_shadow(ctx, _tt_font_thin, string, 15, 30, 30, rgb(255,255,255), rgb(0,0,0), 4);

	strftime(string,1024,"%a %d %b %Y %T %Z",timeinfo);

	tt_draw_string_shadow(ctx, _tt_font_thin, string, 15, 30, 60, rgb(255,255,255), rgb(0,0,0), 4);

	flip(ctx);
}

int main(int argc, char * argv[]) {
	fprintf(stderr, "open() = %ld\n", syscall_open("/dev/null", 0, 0));
	fprintf(stderr, "open() = %ld\n", syscall_open("/dev/null", 1, 0));
	fprintf(stderr, "open() = %ld\n", syscall_open("/dev/null", 1, 0));

	ctx = init_graphics_fullscreen_double_buffer();
	draw_fill(ctx, rgb(120,120,120));
	flip(ctx);

	_tt_font_thin = tt_font_from_file("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	load_sprite(&wallpaper, "/usr/share/wallpaper.jpg");

	draw_fill(ctx, rgb(0,0,0));
	flip(ctx);

	struct timeval now;
	gettimeofday(&now, NULL);

	int forked = 0;
	while (1) {
		time_t last = now.tv_sec;
		timeinfo = localtime(&last);
		redraw();

		while (1) {
			gettimeofday(&now, NULL);
			if (now.tv_sec != last) break;
		}

		if (!forked) {
			forked = 1;
			system("uname -a");
		}
	}

	return 0;
}
