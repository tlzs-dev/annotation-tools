/*
 * shell.c
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
#include "shell.h"
#include <gtk/gtk.h>

#include "common.h"
#include "da_panel.h"
#include "property-list.h"
#include "utils.h"


typedef struct shell_private
{
	shell_ctx_t base[1];

	GtkWidget * window;
	GtkWidget * content_area;
	GtkWidget * header_bar;
	GtkWidget * statusbar;
	GtkWidget * combo;

	da_panel_t * panels[1];		// TODO: multi-viewport support
	property_list_t * properties;
	
	char image_file[PATH_MAX];
	char label_file[PATH_MAX];

	guint timer_id;
	int (* on_update)(struct shell_ctx * shell);
}shell_private_t;

static int shell_stop(shell_ctx_t * shell)
{
	if(!shell->quit)
	{
		shell->quit = 1;
		gtk_main_quit();
	}
	return 0;
}

static void on_load_image(GtkFileChooserButton * chooser, shell_private_t * priv)
{
	guint msgid = gtk_statusbar_get_context_id(GTK_STATUSBAR(priv->statusbar), "info");
	const char * path_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
	gtk_statusbar_push(GTK_STATUSBAR(priv->statusbar), msgid, path_name);
	int rc = 0;

	priv->image_file[0] = '\0';
	priv->label_file[0] = '\0';

	rc = check_file(path_name);
	assert(0 == rc);
	strncpy(priv->image_file, path_name, sizeof(priv->image_file));
	
	char annotation_file[PATH_MAX] = "";
	strncpy(annotation_file, path_name, sizeof(annotation_file));

	char * p_ext = strrchr(annotation_file, '.');
	strcat(annotation_file, ".txt");
	rc = check_file(annotation_file);
	if(rc && p_ext)
	{
		strcpy(p_ext, ".txt");
		rc = check_file(annotation_file);
	}

	if(0 == rc)
	{
		strncpy(priv->label_file, annotation_file, sizeof(priv->label_file));
	}else
	{
		snprintf(priv->label_file, sizeof(priv->label_file), "%s.txt", path_name);
	}
	
	da_panel_t * panel = priv->panels[0];
	assert(panel->load_image);
	if(panel->load_image)
	{
		annotation_list_t * list = priv->properties->annotations;
		assert(list);
		annotation_list_reset(list);

		if(0 == rc)
		{
			printf("annotation_file: '%s'", priv->label_file);
			ssize_t count = list->load(list, priv->label_file);
			assert(count >= 0);
			annotation_list_dump(list);
		}
		panel->load_image(panel, path_name);
	}
	return;
}

static void on_save_annotation(GtkWidget * button, shell_private_t * priv)
{
	annotation_list_t * list = priv->properties->annotations;
	assert(list);

	int rc = list->save(list, priv->label_file);
	assert(0 == rc);
	return;
}

static void on_selchanged_labels(GtkComboBox * combo, shell_private_t * priv)
{
	gint id = gtk_combo_box_get_active(combo);
	char msg[200] = "";

	GtkHeaderBar * header_bar = GTK_HEADER_BAR(priv->header_bar);
	assert(header_bar);

	snprintf(msg, sizeof(msg), "active id: %d", (int)id);
	gtk_header_bar_set_subtitle(header_bar, msg);


	da_panel_t * panel = priv->panels[0];
	assert(panel);

	da_panel_set_annotation(panel, id);
	return;
}


static int shell_init(shell_ctx_t * shell, void * settings)
{
	shell_private_t * priv = (shell_private_t *)shell;
	assert(priv);

	GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget * header_bar = gtk_header_bar_new();
	GtkWidget * grid = gtk_grid_new();
	GtkWidget * uri_entry = gtk_entry_new();
	GtkWidget * nav_button = gtk_button_new_from_icon_name("go-jump", GTK_ICON_SIZE_BUTTON);
	GtkWidget * hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget * statusbar = gtk_statusbar_new();

	GtkWidget * hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	property_list_t * list = property_list_new(shell);
	assert(list);
	priv->properties = list;

	da_panel_t * panel = da_panel_new(640, 480, shell);
	assert(panel);
	priv->panels[0] = panel;

	
	
	gtk_grid_attach(GTK_GRID(grid), list->scrolled_win, 0, 0, 1, 1);
	
	gtk_paned_add1(GTK_PANED(hpaned), panel->frame);
	gtk_paned_add2(GTK_PANED(hpaned), grid);
	gtk_paned_set_position(GTK_PANED(hpaned), 640);

	gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), _("Annotation Tools"));

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, TRUE, 0);

	


	GtkWidget * filechooser = gtk_file_chooser_button_new(_("load image"), GTK_FILE_CHOOSER_ACTION_OPEN);
	GtkFileFilter * filter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type(filter, "image/jpeg");
	gtk_file_filter_add_mime_type(filter, "image/png");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(filechooser), filter);
	g_signal_connect(filechooser, "file-set", G_CALLBACK(on_load_image), shell);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), filechooser);

	GtkWidget * save_btn = gtk_button_new_from_icon_name("document-save", GTK_ICON_SIZE_BUTTON);
	assert(save_btn);
	g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_annotation), shell);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), save_btn);


	global_params_t * params = global_params_get_default();
	assert(params->num_labels > 0 && params->labels);

	GtkListStore * store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
	assert(store);
	for(int i = 0; i < params->num_labels; ++i)
	{
		GtkTreeIter iter;
		printf("add %d: %s(%s)\n", i, params->labels[i], _(params->labels[i]));
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, i,
			1, _(params->labels[i]),
			-1);
	}
	
	//~ GtkWidget * combo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
	GtkWidget * combo = gtk_combo_box_new_with_entry();
	
	gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(store));
	g_object_unref(store);

	GtkCellRenderer * cr = NULL;

	cr = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cr, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cr, "text", 0, NULL);
	
	
	//~ cr = gtk_cell_renderer_text_new();
	//~ gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cr, TRUE);
	//~ gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cr, "text", 1, NULL);


	gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(combo), 1);
	gtk_combo_box_set_id_column(GTK_COMBO_BOX(combo), 0);
	g_signal_connect(combo, "changed", G_CALLBACK(on_selchanged_labels), shell);

	GtkWidget * label = gtk_label_new(_("Classes"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 1);
	priv->combo = combo;


	gtk_box_pack_start(GTK_BOX(hbox), uri_entry, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), nav_button, FALSE, TRUE, 0);
	gtk_widget_set_hexpand(uri_entry, TRUE);
	
	g_signal_connect_swapped(window, "destroy", G_CALLBACK(shell_stop), shell);

	json_object * jshell = NULL;
	if(settings)
	{
		json_object * jconfig = settings;
		
		json_bool ok = FALSE;

		ok = json_object_object_get_ex(jconfig, "shell", &jshell);
		assert(ok && jshell);
	}

	priv->window = window;
	priv->header_bar = header_bar;
	priv->content_area = vbox;
	priv->statusbar = statusbar;

	gtk_window_set_default_size(GTK_WINDOW(window), 1280, 800);

	gtk_window_set_role(GTK_WINDOW(window), "debug-test");

	if(NULL == jshell) return 0;


	// TODO: load config
	
	return 0;
}

static int shell_run(shell_ctx_t * shell)
{
	shell->quit = 0;


	gtk_widget_show_all(((shell_private_t *)shell)->window);
	
	gtk_main();
	shell->quit = 1;
	return 0;
}

static shell_private_t g_shell[1] = {{
	.base = {{ 	.init = shell_init,
				.run = shell_run,
			}},
}};

shell_ctx_t * shell_new(int argc, char ** argv, void * user_data)
{
	static int init_flags = 0;
	if(!init_flags)
	{
		init_flags = 1;
		gtk_init(&argc, &argv);
	}
	
	shell_ctx_t * shell = (shell_ctx_t *)g_shell;
	shell->user_data = user_data;

	return shell;
}
void shell_cleanup(shell_ctx_t * shell)
{
	return;
}

annotation_list_t * shell_get_annotations(shell_ctx_t * shell)
{
	shell_private_t * priv = (shell_private_t *)shell;
	property_list_t * props = priv->properties;
	if(props) return props->annotations;
	return NULL;
}

void shell_redraw(shell_ctx_t * shell)
{
	shell_private_t * priv = (shell_private_t *)shell;
	property_list_t * props = priv->properties;
	property_list_redraw(props);
	return;
}

void shell_set_current_label(shell_ctx_t * shell, int klass)
{
	shell_private_t * priv = (shell_private_t *)shell;
	global_params_t	* params = global_params_get_default();
	assert(klass >= 0 && klass <= params->num_labels);

	GtkComboBox * combo = GTK_COMBO_BOX(priv->combo);
	gtk_combo_box_set_active(combo, klass);
	return;
}
