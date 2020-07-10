#ifndef _ANNOTATION_TOOLS_COMMON_H_
#define _ANNOTATION_TOOLS_COMMON_H_

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <json-c/json.h>

#include <libintl.h>
#include <locale.h>

#ifndef _
#define _(str)	gettext(str)
#endif

#define TEXT_DOMAIN	"annotation-tools"

#include <gtk/gtk.h>

struct shell_ctx;
typedef struct global_params
{
	json_object * jconfig;
	struct shell_ctx * shell;

	int num_labels;
	const char ** labels;

	GdkRGBA fg_color;
	GdkRGBA bg_color;
	GdkRGBA sel_color;
	double line_size;
	
	GdkRGBA font_color;
	double font_size;
	const char * font_name;
}global_params_t;
global_params_t * global_params_get_default();

typedef struct annotation_data
{
	int klass;
	double x;		// center.x
	double y;		// center.y
	double width;
	double height;
}annotation_data_t;

typedef struct annotation_list
{
	ssize_t max_size;
	ssize_t length;
	annotation_data_t ** data;
	ssize_t (* load)(struct annotation_list * list, const char * filename);
	int (* save)(struct annotation_list * list, const char * filename);
	int (* resize)(struct annotation_list * list, ssize_t new_size);
	int (* update)(struct annotation_list * list, int index, const annotation_data_t * data);	// index: -1 ==> add
	int (* remove)(struct annotation_list * list, int index);
}annotation_list_t;
annotation_list_t * annotation_list_init(annotation_list_t * list, ssize_t max_size);
void annotation_list_cleanup(annotation_list_t * list);
void annotation_list_reset(annotation_list_t * list);
void annotation_list_dump(const annotation_list_t * list);

#ifdef __cplusplus
}
#endif
#endif
