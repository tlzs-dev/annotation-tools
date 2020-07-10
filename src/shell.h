#ifndef _SHELL_H_
#define _SHELL_H_

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"		// annotation_data_t

typedef struct shell_ctx
{
	void * user_data;
	void * priv;

	int quit;
	int (* init)(struct shell_ctx * shell, void * settings);
	int (* run)(struct shell_ctx * shell);
}shell_ctx_t;
shell_ctx_t * shell_new(int argc, char ** argv, void * user_data);
void shell_cleanup(shell_ctx_t * shell);


int shell_add_annotation(shell_ctx_t * shell, int index, const annotation_data_t * data);
annotation_list_t * shell_get_annotations(shell_ctx_t * shell);
void shell_redraw(shell_ctx_t * shell);
void shell_set_current_label(shell_ctx_t * shell, int klass);

#ifdef __cplusplus
}
#endif
#endif
