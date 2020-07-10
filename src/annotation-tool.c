/*
 * annotation-tool.c
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

#include <json-c/json.h>

//~ #include "utils.h"
//~ #include "img_proc.h"

#include "shell.h"

#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

#include "shell.h"
#include "common.h"

#include <unistd.h>
#include <getopt.h>
#include "utils.h"

static global_params_t g_params[1];

void show_help(const char * exe_name)
{
	fprintf(stderr, "%s  [--conf=conf_file] [--help]\n"
		"%s [-c conf_file] [-h]\n",
		exe_name,
		exe_name);
	return;
}

static int load_labels(global_params_t * params, const char * labels_file)
{
	json_object * jconfig = json_object_from_file(labels_file);
	if(jconfig)	// json format
	{
		json_object * jlabels = NULL;
		json_bool ok = FALSE;
		ok = json_object_object_get_ex(jconfig, "labels", &jlabels);
		assert(ok && jlabels);
		
		int num_labels = json_object_array_length(jlabels);
		assert(num_labels > 0);

		const char ** labels = calloc(num_labels, sizeof(*labels));
		assert(labels);
		
		for(int i = 0; i < num_labels; ++i)
		{
			labels[i] = json_object_get_string(json_object_array_get_idx(jlabels, i));
			assert(labels[i] && labels[i][0]);
		}

		params->num_labels = num_labels;
		params->labels = labels;
	}else // plain text
	{
		FILE * fp = fopen(labels_file, "r");
		assert(fp);

		int max_size = 1024;
		int num_labels = 0;
		char buf[200] = "";
		char * line = NULL;

		char ** labels = calloc(max_size, sizeof(*labels));
		assert(labels);

		while((line = fgets(buf, sizeof(buf), fp)))
		{
			char * p_comment = strchr(line, '#');
			if(p_comment) *p_comment = '\0';	// skip comment lines
			
			int cb = strlen(line);
			while(cb > 0 && (line[cb - 1] == '\n' || line[cb - 1] == '\r')) line[--cb] = '\0';
			if(cb == 0) continue;	// skip empty lines

			line = trim(line, line + cb);
			cb = strlen(line);
			if(cb == 0) continue;	// skip white lines 
			
			assert(num_labels <= max_size);
			labels[num_labels++] = strdup(line);
		}
		fclose(fp);

		params->num_labels = num_labels;
		params->labels = realloc(labels, num_labels * sizeof(char *));
	}

	
	return 0;
}


void global_params_dump(global_params_t * params)
{
	printf("==== %s(%p) ====\n", __FUNCTION__, params);
	for(int i = 0; i < params->num_labels; ++i)
	{
		const char * label = params->labels[i];
		assert(label);

		printf("\t%.3d: [%s]\n", i, label);
	}
	return;
}

int global_params_load_config(global_params_t * params, json_object * jconfig)
{
	const char * labels_file = json_get_value_default(jconfig, string, labels_file, "conf/labels.json");
	assert(labels_file && labels_file[0]);

	int rc = load_labels(params, labels_file);
	assert(0 == rc);

	const char * fg_color = json_get_value_default(jconfig, string, fg-color, "blue");
	const char * bg_color = json_get_value_default(jconfig, string, bg-color, "white");
	const char * sel_color = json_get_value_default(jconfig, string, selection-color, "yellow");
	double line_size = json_get_value_default(jconfig, double, line-size, 3);
	double font_size = json_get_value_default(jconfig, double, font-size, 18);
	const char * font_color = json_get_value_default(jconfig, string, font-color, "#ff00ff");
	const char * ext_name = json_get_value_default(jconfig, string, ext_name, ".txt");
	const char * working_path = json_get_value_default(jconfig, string, working_path, ".");
	const char * font_name = json_get_value_default(jconfig, string, font-name, "DejaVu Sans Mono");

	assert(fg_color && bg_color && sel_color && font_color && ext_name && working_path);

	gboolean ok = FALSE;
	ok = gdk_rgba_parse(&params->fg_color, fg_color); 	assert(ok);
	ok = gdk_rgba_parse(&params->bg_color, bg_color);	assert(ok);
	ok = gdk_rgba_parse(&params->sel_color, sel_color); assert(ok);
	ok = gdk_rgba_parse(&params->font_color, font_color); assert(ok);

	params->line_size = line_size;
	params->font_size = font_size;
	params->font_name = font_name;

	global_params_dump(params);
	return 0;
}

global_params_t * global_params_get_default()
{
	return g_params;
}

int global_params_parse_args(global_params_t * params, int argc, char ** argv)
{
	assert(argc > 0 && argv);
	const char * conf_file = "conf/annotation-tools.json";

	static struct option options[] = {
		{ "conf", required_argument, 0, 'c' },
		{ "help", no_argument, 0, 'h' },
	};

	while(1)
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "c:h", options, &option_index);
		if(c == -1) break;

		switch(c)
		{
		case 'c': conf_file = optarg; break;
		case 'h': show_help(argv[0]); exit(0); break;
		default:
			fprintf(stderr, "unknown commandline args '%c'.", c);
			exit(1); 
		}
	}

	assert(conf_file && conf_file[0]);
	json_object * jconfig = json_object_from_file(conf_file);
	assert(jconfig);
	params->jconfig = jconfig;

	if(jconfig)
	{
		global_params_load_config(params, jconfig);
	}


	debug_printf("config: %s\n", json_object_to_json_string_ext(jconfig, JSON_C_TO_STRING_PRETTY));
	return 0;
}


int main(int argc, char **argv)
{
	int rc = 0;
	setlocale(LC_ALL, "");

	// getrealpath
	char realpath[PATH_MAX] = "";
	ssize_t cb = readlink("/proc/self/exe", realpath, sizeof(realpath));
	assert(cb > 0);

	char * lang_dir = dirname(realpath);
	assert(lang_dir);

	char * p_bin_dir = strstr(lang_dir, "/bin");
	if(p_bin_dir) *p_bin_dir = '\0';

	strcat(lang_dir, "/lang");
	printf("text_domain_path: %s\n", lang_dir);

	global_params_t * params = g_params;
	rc = global_params_parse_args(params, argc, argv);
	assert(0 == rc);
	
	shell_ctx_t * shell = shell_new(argc, argv, params);
	assert(shell);
	const char * path = bindtextdomain(TEXT_DOMAIN, lang_dir);
	textdomain(TEXT_DOMAIN);

	printf("%s = %s\n", _("binding path"),  path);

	
	rc = shell->init(shell, NULL);
	assert(0 == rc);

	rc = shell->run(shell);

	shell_cleanup(shell);
	return rc;
}

