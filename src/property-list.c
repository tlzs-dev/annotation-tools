/*
 * property-list.c
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

#include "common.h"
#include "property-list.h"
#include "utils.h"
#include "shell.h"


enum property_column
{
	property_column_index,
	property_column_key,
	property_column_value,
	property_column_data_ptr,
	property_columns_count
};

static void init_treeview(GtkTreeView * treeview)
{
	debug_printf("%s()...", __FUNCTION__);
	GtkCellRenderer * cr = NULL;
	GtkTreeViewColumn * col = NULL;

	cr = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("index"), cr,
		"text", property_column_index, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(treeview, col);

	cr = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("key"), cr,
		"text", property_column_key, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(treeview, col);

	cr = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("value"), cr,
		"text", property_column_value, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(treeview, col);

	gtk_tree_view_set_grid_lines(treeview, GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);
//	gtk_tree_view_set_hover_expand(treeview, TRUE);

	GtkTreeStore * store = gtk_tree_store_new(property_columns_count, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
}


property_list_t * property_list_new(void * user_data)
{
	GtkWidget * scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget * treeview = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scrolled_win), treeview);

	

	property_list_t * list = calloc(1, sizeof(*list));
	assert(list);
	
	list->user_data = user_data;
	annotation_list_init(list->annotations, 0);

	list->scrolled_win = scrolled_win;
	list->treeview = treeview;

	gtk_widget_set_hexpand(scrolled_win, TRUE);
	gtk_widget_set_vexpand(scrolled_win, TRUE);

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_win), GTK_SHADOW_ETCHED_OUT);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	init_treeview(GTK_TREE_VIEW(treeview));

	return list;
}


void property_list_free(property_list_t * props)
{
	
}

void property_list_redraw(property_list_t * props)
{
	GtkTreeStore * store = gtk_tree_store_new(property_columns_count, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	annotation_list_t * list = props->annotations;
	assert(list);

	//~ GtkTreeIter root;
	//~ gtk_tree_store_append(store, &root, NULL);
	//~ gtk_tree_store_set(store, &root, property_column_key, _("annotations"), -1);
	const global_params_t * params = global_params_get_default();
	assert(params);
	assert(params->num_labels > 0);
	assert(params->labels);
	
	for(int i = 0; i < list->length; ++i)
	{
		annotation_data_t * data = list->data[i];
		assert(data);

		char sz_value[200] = "";
		char sz_index[200] = "";
		
		GtkTreeIter parent, iter;
		snprintf(sz_index, sizeof(sz_index), "%d", i);
		
		const char * label = (data->klass>=0 && data->klass < params->num_labels)?_(params->labels[data->klass]):_("unknown");
		snprintf(sz_value, sizeof(sz_value), "%d: %s {%.3f, %.3f, %.3f, %.3f}", data->klass, label,
			data->x, data->y, data->width, data->height
		);
		
		gtk_tree_store_append(store, &parent, NULL);
		gtk_tree_store_set(store, &parent,
			property_column_index, sz_index,
			property_column_key, "Class",
			property_column_value, sz_value,
			property_column_data_ptr, data,
			-1);

		
		gtk_tree_store_append(store, &iter, &parent);
		snprintf(sz_value, sizeof(sz_value), "%.6f", data->x);
		gtk_tree_store_set(store, &iter, property_column_key, "x",
			property_column_value, sz_value,
			-1);

		gtk_tree_store_append(store, &iter, &parent);
		snprintf(sz_value, sizeof(sz_value), "%.6f", data->y);
		gtk_tree_store_set(store, &iter, property_column_key, "y",
			property_column_value, sz_value,
			-1);

		gtk_tree_store_append(store, &iter, &parent);
		snprintf(sz_value, sizeof(sz_value), "%.6f", data->width);
		gtk_tree_store_set(store, &iter, property_column_key, "width",
			property_column_value, sz_value,
			-1);
			
		gtk_tree_store_append(store, &iter, &parent);
		snprintf(sz_value, sizeof(sz_value), "%.6f", data->height);
		gtk_tree_store_set(store, &iter, property_column_key, "height",
			property_column_value, sz_value,
			-1);
	}

	GtkTreeView * treeview = GTK_TREE_VIEW(props->treeview);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
	gtk_tree_view_expand_all(treeview);
}

