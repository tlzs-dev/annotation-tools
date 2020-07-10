#ifndef _PROPERTY_LIST_H_
#define _PROPERTY_LIST_H_

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif


#include <gtk/gtk.h>
#include "common.h"

typedef struct property_list
{
	void * user_data;		// shell
	GtkWidget * scrolled_win;
	GtkWidget * treeview;
	void * shell;
	annotation_list_t annotations[1];
}property_list_t;

property_list_t * property_list_new(void * user_data);
void property_list_free(property_list_t * list);
void property_list_redraw(property_list_t * props);

#ifdef __cplusplus
}
#endif
#endif
