#ifndef _DA_PANEL_H_
#define _DA_PANEL_H_

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif


#include "img_proc.h"
#include <gtk/gtk.h>


enum border_type
{
	border_type_unknown,
	border_type_top_left,
	border_type_top,
	border_type_top_right,
	border_type_left,
	border_type_right,
	border_type_bottom_left,
	border_type_bottom,
	border_type_bottom_right,

	border_type_dragging,
	border_types_count
};

enum graphic_mode
{
	graphic_mode_none,
	graphic_mode_dragging,
	graphic_mode_selected,
};

typedef struct button_state
{
	int clicks;		// 1: click, 2: double-clicks, 3: triple-clicks
	guint32 time_ms;	// press-release interval (milli-seconds),  
	int x1, y1;		// begin pos
//	int x2, y2;		// end pos
}button_state_t;

typedef struct int_rect
{
	int x, y, cx, cy;
}int_rect;

typedef struct da_panel
{
	GtkWidget * frame;
	GtkWidget * da;
	void * shell;
	int width;		// viewport width
	int height;		// viewport height

	cairo_surface_t * surface;
	int image_width;
	int image_height;

	int auto_scale;
	int keep_ratio;


	bgra_image_t image[1];
	void (* draw)(struct da_panel * panel, const bgra_image_t * frame);
	int (* load_image)(struct da_panel * panel, const char * path_name);

	enum border_type selected_border;
	GdkCursor * cursors[border_types_count];

	int cur_index;
	enum graphic_mode mode;
	int_rect selection[1];	// dragging or selecting bounding-box
	
	annotation_list_t * annotations;		// pointer to property_list->annotations

	int button_index;
	button_state_t buttons[4];	// 0: not used, 1: left-button; 2: middle-button or (shift+left_btn); 3: right-button

	int def_class;
}da_panel_t;

da_panel_t * da_panel_new(int min_width, int min_height, shell_ctx_t * shell);
void da_panel_free(da_panel_t * panel);

void da_panel_set_annotation(da_panel_t * panel, int klass);

#ifndef _xor_sort
#define _xor_sort(a, b)	do { if(a > b) { a^=b; b^=a; a^=b; } } while(0)
#endif

#ifdef __cplusplus
}
#endif
#endif
