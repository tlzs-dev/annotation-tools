/*
 * da_panel.c
 * 
 * Copyright 2020 chehw <htc.chehw@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "utils.h"

#include "shell.h"
#include <gtk/gtk.h>

#include "common.h"
#include "da_panel.h"

static gboolean on_da_key_pressed(GtkWidget * da, GdkEventKey * event, da_panel_t * panel)
{
	return FALSE;
}
static gboolean on_da_key_released(GtkWidget * da, GdkEventKey * event, da_panel_t * panel)
{
	
	return FALSE;
}

static gboolean on_da_triple_clicks(GtkWidget * da, GdkEventButton * event, da_panel_t * panel)
{
	printf("%s()...\n", __FUNCTION__);
	return FALSE;
}

#include <math.h>
#define LINE_WIDTH_THRESHOLD ( 5.0 / 480.0 )
enum border_type pt_on_border(double x, double y, const annotation_data_t * bbox)
{
	enum border_type border = border_type_unknown;

	double x1 = bbox->x - bbox->width / 2.0;
	double y1 = bbox->y - bbox->height / 2.0;
	double x2 = x1 + bbox->width;
	double y2 = y1 + bbox->height;

	if(fabs(y - y1) <= LINE_WIDTH_THRESHOLD)
	{
		if(fabs(x - x1) <= LINE_WIDTH_THRESHOLD) return border_type_top_left;
		if(fabs(x - x2) <= LINE_WIDTH_THRESHOLD) return border_type_top_right;
		if(x > x1 && x < x2) return border_type_top;
	}

	if(fabs(y - y2) <= LINE_WIDTH_THRESHOLD)
	{
		if(fabs(x - x1) <= LINE_WIDTH_THRESHOLD) return border_type_bottom_left;
		if(fabs(x - x2) <= LINE_WIDTH_THRESHOLD) return border_type_bottom_right;
		if(x > x1 && x < x2) return border_type_bottom;
	}

	if( y > y1 && y < y2)
	{
		if(fabs(x - x1) <= LINE_WIDTH_THRESHOLD) return border_type_left;
		if(fabs(x - x2) <= LINE_WIDTH_THRESHOLD) return border_type_right;
	}

	return border;
}



static gboolean on_da_double_clicked(GtkWidget * da, GdkEventButton * event, da_panel_t * panel)
{
	button_state_t * button = &panel->buttons[event->button];
	printf("%s(x1=%d, y1 = %d)... ==>> delete annotation \n", __FUNCTION__, button->x1, button->y1);
	button->clicks = 0;

	int width = panel->width;
	int height = panel->height;

	double x = event->x;
	double y = event->y;

	annotation_list_t * list = panel->annotations;
	if(NULL == list) return FALSE;

	x /= (double)width;
	y /= (double)height;

	for(ssize_t i = 0; i < list->length; ++i)
	{
		annotation_data_t * bbox = list->data[i];
		assert(bbox);
		
		enum border_type border = pt_on_border(x, y, bbox);
		printf("\t== border: %d\n", border);
		if(border != border_type_unknown)
		{
			printf("\t== remove %d @ { %.3f, %.3f, %.3f, %.3f }, border=%d\n", (int)i,
				bbox->x, bbox->y,
				bbox->width, bbox->height,
				(int)border
			);
			list->remove(list, i);
			panel->cur_index = -1;
			gtk_widget_queue_draw(panel->da);
			shell_redraw(panel->shell);
			break;
		}
	}
	return FALSE;
}

static gboolean on_da_clicked(GtkWidget * da, GdkEventButton * event, da_panel_t * panel)
{
	printf("%s()...\n", __FUNCTION__);

	panel->cur_index = -1;
	
//	gdk_window_set_cursor(event->window, panel->cursors[panel->selected_border]);

	panel->mode = graphic_mode_none;
	panel->cur_index = -1;	// reset selection states
	button_state_t * button = &panel->buttons[event->button];
	
	button->x1 = (int)event->x;
	button->y1 = (int)event->y;

	int_rect * bbox = panel->selection;
	bbox->cx = 0;
	bbox->cy = 0;


	annotation_list_t * list = panel->annotations;
	if(list && panel->width > 1 && panel->height > 1 && panel->image_width > 0)
	{
		double x = event->x / (double)(panel->width);
		double y = event->y / (double)(panel->height);

		enum border_type border = border_type_unknown;
		int cur_index = -1;
		for(int i = 0; i < list->length; ++i)
		{
			annotation_data_t * bbox = list->data[i];
			assert(bbox);
			border = pt_on_border(x, y, bbox);
			if(border != border_type_unknown)
			{
				cur_index = i;
				shell_set_current_label(panel->shell, bbox->klass);
				break;
			}
		}
		panel->cur_index = cur_index;
		panel->selected_border = border;

		
	}

	gdk_window_set_cursor(event->window, panel->cursors[panel->selected_border]);
	return FALSE;
}

static gboolean on_da_button_pressed(GtkWidget * da, GdkEventButton * event, da_panel_t * panel)
{
	printf("\e[33m" "%s()..." "\e[39m" "\n", __FUNCTION__);
	
	int index = event->button;
	if(index < 1 || index > 3) return FALSE;
	button_state_t * button = &panel->buttons[index];

	panel->button_index = index;

	if(button->clicks++ == 0)
	{
		return on_da_clicked(da, event, panel);
	}

	int clicks = button->clicks;
	if(clicks == 3)	// not used
	{
		button->clicks = 0;	// 	reset
		return FALSE;
		return on_da_triple_clicks(da, event, panel);
	}

	on_da_double_clicked(da, event, panel);
	return FALSE;
}

static int add_annotation(da_panel_t * panel, const int_rect * bbox)
{
	int image_width = panel->width;
	int image_height = panel->height;
	if(image_width <= 0 || image_height <= 0) return -1;


	double x = (double)bbox->x / (double)image_width;
	double y = (double)bbox->y / (double)image_height;
	double cx = (double)bbox->cx / (double)image_width;
	double cy = (double)bbox->cy / (double)image_height;

	annotation_data_t data[1];
	memset(data, 0, sizeof(data));

	if(panel->def_class >= 0) data->klass = panel->def_class;
	data->x = x + cx / 2;		// center_x
	data->y = y + cy / 2;		// center_y
	data->width = cx;
	data->height = cy;

	annotation_list_t * annotations = panel->annotations;
	if(NULL == annotations)
	{
		annotations = shell_get_annotations(panel->shell);
		assert(annotations);

		panel->annotations = annotations;
	}

	

	int rc = annotations->update(annotations, panel->cur_index, data);
	assert(0 == rc);


	
	shell_redraw(panel->shell);
	
	return rc;
	
}

#define MIN_DISTANCE (3)		// 3 pixels
static gboolean on_da_button_released(GtkWidget * da, GdkEventButton * event, da_panel_t * panel)
{
	printf("\e[33m" "%s()..." "\e[39m" "\n", __FUNCTION__);
	gdk_window_set_cursor(event->window, panel->cursors[border_type_unknown]);

	int index = event->button;
	if(index < 1 || index > 3) return FALSE;
	button_state_t * button = &panel->buttons[index];
	button->clicks = 0;
	

	int_rect * bbox = panel->selection;
	if(panel->selected_border != border_type_unknown)
	{
		
		if(bbox->cx > MIN_DISTANCE && bbox->cy > MIN_DISTANCE)
		{
			add_annotation(panel, bbox);
		}
		
		panel->button_index = 0;
		panel->selected_border = border_type_unknown;

		bbox->cx = 0;
		bbox->cy = 0;
	//	panel->cur_index = -1;
		gtk_widget_queue_draw(da);

		annotation_list_t * list = panel->annotations;
		if(panel->cur_index >= 0 && panel->cur_index < list->length)
		{
			annotation_data_t * data = list->data[panel->cur_index];
			assert(data);
			shell_set_current_label(panel->shell, data->klass);
		}

		
	
		return FALSE;
	}

	
	panel->button_index = 0;	// reset
	panel->selected_border = border_type_unknown;
	
	

	

	// @todo:

	int x1 = button->x1;
	int y1 = button->y1;
	int x2 = event->x;
	int y2 = event->y;

	_xor_sort(x1, x2);
	_xor_sort(y1, y2);

	int cx = x2 - x1;
	int cy = y2 - y1;


	if(cx > MIN_DISTANCE && cy > MIN_DISTANCE)
	{
		panel->mode = graphic_mode_selected;
	}else
	{
		cx = 0; cy = 0;
	}

	if(cx <= 0 && cy <= 0) {
		panel->cur_index = -1;
		return FALSE;
	}
	
	bbox->x = x1;
	bbox->y = y1;
	bbox->cx = cx;
	bbox->cy = cy;

	add_annotation(panel, bbox);
	gtk_widget_queue_draw(da);

	
			
	
	panel->cur_index = -1;
	bbox->cx = 0;
	bbox->cy = 0;
	

	return FALSE;
}

static gboolean resize_bbox(da_panel_t * panel, GdkEventMotion * event, double x, double y)
{
	int index = panel->button_index;
	if(index < 1 || index > 3) return FALSE;

	button_state_t * button = &panel->buttons[index];
	assert(button);
	
	int cur_index = panel->cur_index;
	enum border_type border = panel->selected_border;

	gdk_window_set_cursor(event->window, panel->cursors[border]);
	annotation_list_t * list = panel->annotations;
	if(list)
	{
		assert(cur_index >= 0 && cur_index < list->length);
	}

	// @todo:
	annotation_data_t * data = list->data[cur_index];
	assert(data);
	int width = panel->width;
	int height = panel->height;
	
	int x1 = button->x1;
	int y1 = button->y1;
	int x2 = x;
	int y2 = y;

	double dx = 0;
	double dy = 0;

	int_rect * bbox = panel->selection;
	assert(bbox);
	bbox->x = (data->x - data->width / 2) * (double)width;
	bbox->y = (data->y - data->height / 2) * (double)height;
	bbox->cx = data->width * (double)width;
	bbox->cy = data->height * (double)height;
	
	switch(border)
	{
	case border_type_top: case border_type_bottom:
		dy = y2 - y1;
		if(border == border_type_top)
		{
				bbox->y += dy;
				bbox->cy -= dy;
		}else
		{
			bbox->cy += dy;
		}
		break;
	case border_type_left: case border_type_right:
		dx = x2 - x1;
		if(border == border_type_left)
		{
			bbox->x += dx;
			bbox->cx -= dx;
		}else
		{
			bbox->cx += dx;
		}
		break;
	default:
		dx = x2 - x1;
		dy = y2 - y1;
		if(border == border_type_top_left)
		{
			bbox->x += dx;
			bbox->y += dy;
			bbox->cx -= dx;
			bbox->cy -= dy;
		}else if(border == border_type_top_right)
		{
			bbox->cx += dx;
			bbox->y += dy;
			bbox->cy -= dy;
		}else if(border == border_type_bottom_left)
		{
			bbox->x += dx;
			bbox->cx -= dx;
			bbox->cy += dy;
		}else if(border == border_type_bottom_right)
		{
			bbox->cx += dx;
			bbox->cy += dy;
		}
		break;
	}
	
	gtk_widget_queue_draw(panel->da);
	gtk_main_iteration();
	return FALSE;
}

static gboolean on_da_motion_notify(GtkWidget * da, GdkEventMotion * event, da_panel_t * panel)
{
	int index = panel->button_index;
	gint x = event->x;
	gint y = event->y;
	GdkModifierType masks = event->state;

	if(event->is_hint)
	{
		gdk_window_get_device_position(event->window, event->device,
			&x, &y, &masks);
	}

	if(panel->cur_index >= 0 && panel->selected_border != border_type_unknown)
	{
		return resize_bbox(panel, event, x, y);
	}
	
	int width = panel->width;
	int height = panel->height;
	
	enum border_type border = border_type_unknown;
	annotation_list_t * list = panel->annotations;
	if(list)
	{
		double _x = (double)x / (double)width;
		double _y = (double)y / (double)height;
			
		for(int i = 0; i < list->length; ++i)
		{
			annotation_data_t * bbox = list->data[i];
			assert(bbox);

			border = pt_on_border(_x, _y, bbox);
			if(border != border_type_unknown)
			{
				
				break;
			}
			
		}
	}
	gdk_window_set_cursor(event->window, panel->cursors[border]);

	if(index < 1 || index > 3) return FALSE;

	gdk_window_set_cursor(event->window, panel->cursors[border_type_dragging]);
	
	button_state_t * button = &panel->buttons[index];
	int x1 = button->x1;
	int y1 = button->y1;
	int x2 = x;
	int y2 = y;

	//~ printf("before sort: %s(x1=%d, y1=%d, is_hint=%d, x2 = %d, y2 = %d)...\n", __FUNCTION__,
		//~ x1, y1, 
		//~ (int)event->is_hint,
		//~ x2, y2
	//~ );

	_xor_sort(x1, x2);
	_xor_sort(y1, y2);

	//~ printf("after sort: %s(x1=%d, y1=%d, is_hint=%d, x2 = %d, y2 = %d)...\n", __FUNCTION__,
		//~ x1, y1, 
		//~ (int)event->is_hint,
		//~ x2, y2
	//~ );

	int_rect * bbox = panel->selection;
	bbox->x = x1;
	bbox->y = y1;
	bbox->cx = x2 - x1;
	bbox->cy = y2 - y1;
	
	gtk_widget_queue_draw(da);

	gtk_main_iteration();
	
	
	
	return FALSE;
}

static void draw_annotations(da_panel_t * panel, cairo_t * cr)
{
	annotation_list_t * list = panel->annotations;
	if(NULL == list) return;

	int width = panel->width;
	int height = panel->height;

	const global_params_t * params = global_params_get_default();
	assert(params);
	// TODO: load color themes
	GdkRGBA line_color;
	double line_width = 0.005;

	line_color = params->fg_color;
	assert(height > 0);

	line_width = params->line_size / (double)height;	// scale to viewport size
	
	//~ line_color.red = 0;
	//~ line_color.green = 0;
	//~ line_color.blue = 1;
	//~ line_color.alpha = 1;

	cairo_set_line_width(cr, line_width);
	debug_printf("%s()::length = %d\n", __FUNCTION__, (int)list->length);


	cairo_save(cr);
	cairo_scale(cr, (double)width, (double)height);
	double dashes[2] = { 0.01, 0.005 };

	
	const char ** labels = params->labels;
	assert(labels && params->num_labels > 0);
	
	for(ssize_t i = 0; i < list->length; ++i)
	{
		annotation_data_t * data = list->data[i];
		assert(data);
		cairo_set_source_rgba(cr, line_color.red, line_color.green, line_color.blue, line_color.alpha);

		if(i == panel->cur_index) cairo_set_dash(cr, dashes, 2, 0);
		else cairo_set_dash(cr, NULL, 0, 0);
		
		cairo_rectangle(cr,
			data->x - data->width / 2,
			data->y - data->height / 2,
			data->width, data->height);
		cairo_stroke(cr);

		if(data->klass <= params->num_labels)
		{
			cairo_text_extents_t extents;
			memset(&extents, 0, sizeof(extents));
			
			GdkRGBA font_color = params->font_color;
			
			cairo_set_font_size(cr, params->font_size / (double)height);
			cairo_select_font_face(cr, params->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);


			const char * label = _(labels[data->klass]);
			cairo_text_extents(cr, label, &extents);

			double x = data->x - data->width / 2;
			double y = data->y - data->height / 2;
			
			cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
			cairo_rectangle(cr, x, y, extents.width, extents.height);
			cairo_fill(cr);

			cairo_set_source_rgb(cr, font_color.red, font_color.green, font_color.blue);
			cairo_move_to(cr, x, y + extents.height);
			cairo_show_text(cr, label);
		}
		
		printf("== draw_annotation[%d]: klass=%d, { %.3f, %.3f, %.3f, %.3f }, font_name=%s\n",
			(int)i,
			(int)data->klass,
			data->x, data->y, data->width, data->height,
			params->font_name
			);
	}
	

	cairo_restore(cr);
	return;
}


static gboolean on_da_draw(GtkWidget * da, cairo_t * cr, da_panel_t * panel)
{
	debug_printf("%s()...\n", __FUNCTION__);
	cairo_surface_t * surface = panel->surface;
	if(NULL == surface || panel->width < 1 || panel->height < 1 || panel->image_width < 1 || panel->image_height < 1)
	{
		cairo_set_source_rgb(cr, 0.2, 0.3, 0.4);
		cairo_paint(cr);

	}else
	{
		cairo_save(cr);
		double sx = 1.0;
		double sy = 1.0;
		if(panel->auto_scale)
		{
			sx = (double)panel->width / (double)panel->image_width;
			sy = (double)panel->height / (double)panel->image_height;
			cairo_scale(cr, sx, sy);
		}
		cairo_set_source_surface(cr, surface, 0, 0);
		cairo_paint(cr);

		cairo_restore(cr);

		// draw current annotations
		draw_annotations(panel, cr);
	}

	int_rect * bbox = panel->selection;
	if(bbox->cx > 0 && bbox->cy > 0)
	{
		char msg[200] = "";
		snprintf(msg, sizeof(msg), "bbox: { %d, %d, %d, %d }", bbox->x, bbox->y, bbox->cx, bbox->cy);

		global_params_t * params = global_params_get_default();
		GdkRGBA sel_color = params->sel_color;
		GdkRGBA font_color = params->font_color;
		GdkRGBA bg_color = params->bg_color;
		
		const char * font_face = params->font_name;
		double line_size = params->line_size;
		double font_size = params->font_size;
		assert(font_size > 0);
		assert(line_size > 0);
		assert(font_face && font_face[0]);

		
		cairo_set_source_rgb(cr, sel_color.red, sel_color.green, sel_color.blue);
		cairo_set_line_width(cr, line_size);
		cairo_rectangle(cr, bbox->x, bbox->y, bbox->cx, bbox->cy);
		cairo_stroke(cr);


		double x = 10;
		double y = 30;
		

		cairo_text_extents_t extents;
		memset(&extents, 0, sizeof(extents));

		cairo_select_font_face(cr, font_face, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, font_size);
		
		cairo_text_extents(cr, msg, &extents);
		cairo_set_source_rgba(cr, bg_color.red, bg_color.green, bg_color.blue, 0.4);

		fprintf(stderr, "text range: width=%.2f, height = %.2f\n", extents.width, extents.height);
		cairo_rectangle(cr, x - 5, y - 5 - extents.height, extents.width + 10, extents.height + 10);
		cairo_fill(cr);


		cairo_move_to(cr, x, y);
		cairo_set_source_rgb(cr, font_color.red, font_color.green, font_color.blue);
		
		cairo_show_text(cr, msg);
	}
	
	return FALSE;
}
static void on_da_realize(GtkWidget * da, da_panel_t * panel)
{
	debug_printf("%s()...\n", __FUNCTION__);
	return;
}

static void on_da_resize(GtkWidget * da, GdkRectangle * allocation, da_panel_t * panel)
{
	debug_printf("%s()...\n", __FUNCTION__);
	
	panel->width = allocation->width;
	panel->height = allocation->height;

	gtk_widget_queue_draw(da);
	return;
}



static void da_panel_draw(struct da_panel * panel, const bgra_image_t * frame)
{
	cairo_surface_t * surface = panel->surface;
	if(NULL == surface || frame->width != panel->image_width || frame->height != panel->height)
	{
		panel->surface = NULL;
		if(surface) cairo_surface_destroy(surface);

		bgra_image_t * image = bgra_image_init(panel->image, frame->width, frame->height, frame->data);
		assert(image && image->data && image->width == frame->width && image->height == frame->height);

		surface = cairo_image_surface_create_for_data(image->data, CAIRO_FORMAT_RGB24,
			image->width, image->height,
			(image->stride > 0)?image->stride:(image->width * 4)
		);
		assert(surface && cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
		panel->surface = surface;
		panel->image_width = image->width;
		panel->image_height = image->height;
	}else
	{
		bgra_image_t * image = bgra_image_init(panel->image, frame->width, frame->height, frame->data);
		assert(image && image->data && image->width == frame->width && image->height == frame->height);
		cairo_surface_mark_dirty(surface);
	}
	gtk_widget_queue_draw(panel->da);
	return;
}

static inline void clear_selections(struct da_panel * panel)
{
	int_rect * bbox = panel->selection;
	bbox->cx = 0;
	bbox->cy = 0;
	panel->cur_index = -1;
	return;
}

static int da_panel_load_image(struct da_panel * panel, const char * path_name)
{
	bgra_image_t image[1];
	memset(image, 0, sizeof(image));

	clear_selections(panel);
	
	int rc = bgra_image_load_from_file(image, path_name);

//	if(!rc) goto label_cleanup;
	assert(0 == rc);
	panel->draw(panel, image);
	
//~ label_cleanup:
	bgra_image_clear(image);

	shell_redraw(panel->shell);
	return 0;
}


da_panel_t * da_panel_new(int min_width, int min_height, shell_ctx_t * shell)
{
	da_panel_t * panel = calloc(1, sizeof(*panel));
	assert(panel);

	panel->draw = da_panel_draw;
	panel->load_image = da_panel_load_image;
	panel->annotations = shell_get_annotations(shell);
	
	GtkWidget * frame = gtk_frame_new(NULL);
	GtkWidget * da = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(frame), da);

	if(min_width > 0 && min_height > 0) gtk_widget_set_size_request(frame, min_width, min_height);

	gint events = gtk_widget_get_events(da);
	events |= GDK_BUTTON_PRESS_MASK	|
		GDK_BUTTON_RELEASE_MASK |
		GDK_POINTER_MOTION_MASK |
		GDK_POINTER_MOTION_HINT_MASK |
		GDK_KEY_PRESS_MASK |
		GDK_KEY_RELEASE_MASK |
		0;
	gtk_widget_set_events(da, events);
	g_signal_connect(da, "button-press-event", G_CALLBACK(on_da_button_pressed), panel);
	g_signal_connect(da, "button-release-event", G_CALLBACK(on_da_button_released), panel);
	g_signal_connect(da, "key-press-event", G_CALLBACK(on_da_key_pressed), panel);
	g_signal_connect(da, "key-release-event", G_CALLBACK(on_da_key_released), panel);
	g_signal_connect(da, "motion-notify-event", G_CALLBACK(on_da_motion_notify), panel);
	g_signal_connect(da, "size-allocate", G_CALLBACK(on_da_resize), panel);
	g_signal_connect(da, "draw", G_CALLBACK(on_da_draw), panel);
	g_signal_connect(da, "realize", G_CALLBACK(on_da_realize), panel);

	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);

	panel->frame = frame;
	panel->da = da;
	panel->shell = shell;
	panel->auto_scale = 1;

	GdkDisplay * display = gtk_widget_get_display(da);
	panel->cursors[border_type_unknown] 	= gdk_cursor_new_from_name(display, "default");
	panel->cursors[border_type_top_left]	= gdk_cursor_new_from_name(display, "nw-resize");
	panel->cursors[border_type_top] 		= gdk_cursor_new_from_name(display, "ns-resize");
	panel->cursors[border_type_top_right] 	= gdk_cursor_new_from_name(display, "ne-resize");
	panel->cursors[border_type_left] 		= gdk_cursor_new_from_name(display, "ew-resize");
	panel->cursors[border_type_right] 		= gdk_cursor_new_from_name(display, "ew-resize");
	panel->cursors[border_type_bottom_left] = gdk_cursor_new_from_name(display, "sw-resize");
	panel->cursors[border_type_bottom] 		= gdk_cursor_new_from_name(display, "ns-resize");
	panel->cursors[border_type_bottom_right] = gdk_cursor_new_from_name(display, "se-resize");

	panel->cursors[border_type_dragging] = gdk_cursor_new_from_name(display, "pointer");
	return panel;
}

void da_panel_free(da_panel_t * panel)
{
	if(NULL == panel) return;

	if(panel->surface)
	{
		cairo_surface_destroy(panel->surface);
		panel->surface = NULL;
	}
	bgra_image_clear(panel->image);
	free(panel);
}


void da_panel_set_annotation(da_panel_t * panel, int klass)
{
	int cur_index = panel->cur_index;
	panel->def_class = klass;

	annotation_list_t * list = panel->annotations;
	assert(list);

	if(cur_index < 0 || cur_index >= list->length)
	{
		fprintf(stderr, "NO SELECTION");
		return;
	}

	annotation_data_t * data = list->data[cur_index];
	assert(data);
	data->klass = klass;

	gtk_widget_queue_draw(panel->da);
	shell_redraw(panel->shell);
}
