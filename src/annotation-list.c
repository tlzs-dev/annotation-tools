/*
 * annotation-list.c
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



#define ANNOTATION_LIST_ALLOCATION_SIZE	(64)
static int annotation_list_resize(struct annotation_list * list, ssize_t new_size)
{
	if(new_size == 0) new_size = ANNOTATION_LIST_ALLOCATION_SIZE;
	if(new_size <= list->max_size) return 0;

	new_size = (new_size + ANNOTATION_LIST_ALLOCATION_SIZE - 1) / ANNOTATION_LIST_ALLOCATION_SIZE * ANNOTATION_LIST_ALLOCATION_SIZE;
	assert(new_size > list->max_size);

	annotation_data_t ** data = realloc(list->data, new_size * sizeof(*data));
	assert(data);
	memset(data + list->max_size, 0, (new_size - list->max_size) * sizeof(*data));
	list->data = data;
	list->max_size = new_size;
	return 0;
}
static int annotation_list_update(struct annotation_list * list, int index, const annotation_data_t * data)	// index: -1 ==> add
{
	int rc = 0;
	if(index < 0 || index >= list->length)
	{
		index = list->length;
		rc = list->resize(list, index + 1);
		assert(0 == rc);
		
		++list->length;
	}

	assert(index >= 0 && index <= list->length);
	annotation_data_t * dst = calloc(1, sizeof(*dst));
	assert(dst);
	memcpy(dst, data, sizeof(*dst));

	list->data[index] = dst;
	return 0;
}

static int annotation_list_remove(struct annotation_list * list, int index)
{
	if(index < 0 || index >= list->length) return -1;

	annotation_data_t * data = list->data[index];
	list->data[index] = NULL;
	--list->length;
	if(index < list->length)
	{
		list->data[index] = list->data[list->length];
		list->data[list->length] = NULL;
	}
	
	if(data) free(data);
	return 0;
}

void annotation_list_reset(annotation_list_t * list)
{
	if(list->data)
	{
		for(ssize_t i = 0; i < list->length; ++i)
		{
			free(list->data[i]);
			list->data[i] = NULL;
		}
	}
	list->length = 0;
	return;
}

static ssize_t annotation_list_load(annotation_list_t * list, const char * filename)
{
	assert(filename);
	FILE *fp = fopen(filename, "r");
	if(NULL == fp) return -1;

	annotation_list_reset(list);
	
	char buf[200] = "";
	ssize_t count = 0;
	
	char * line = NULL;

	#define delim " \t"
	while((line = fgets(buf, sizeof(buf), fp)))
	{
		int rc = 0;
		int index = -1;
		double values[4];
		ssize_t num_fields = 0;
		
		char * tok = NULL;
		char * field = NULL;
		field = strtok_r(line, delim, &tok);
		if(NULL == field) goto label_err;
		index = atoi(field);
		++num_fields;

		field = strtok_r(NULL, delim, &tok);
		while(field)
		{
			assert(num_fields <= 5);
			values[num_fields - 1] = atof(field);
			++num_fields;
			
			field = strtok_r(NULL, delim, &tok);
		}
		
		if(num_fields != 5) {
			fprintf(stderr, "[ERROR]::%s(%d)::%s()::invalid annotation format.", __FILE__, __LINE__, __FUNCTION__);
			goto label_err;
		}

		rc = list->resize(list, list->length + 1);
		assert(0 == rc);
		
		annotation_data_t * data = calloc(1, sizeof(*data));
		assert(data);
		list->data[list->length++] = data;
		
		data->klass = index;
		data->x = values[0];
		data->y = values[1];
		data->width = values[2];
		data->height = values[3];
		++count;
		continue;
	label_err:
		count = -1;
		break;
	}
	fclose(fp);

	if(count >= 0) {
		assert(count == list->length);
	}
	return count;
}


static int annotation_list_save(annotation_list_t * list, const char * filename)
{
	int rc = 0;
	assert(list);
	
	if(list->length <= 0)
	{
		remove(filename);
		return 0;
	}
	
	FILE * fp = fopen(filename, "w+");
	if(NULL == fp) return -1;

	for(ssize_t i = 0; i < list->length; ++i)
	{
		annotation_data_t * data = list->data[i];
		assert(data);

		fprintf(fp, "%d %.6f %.6f %.6f %.6f\n", data->klass, data->x, data->y, data->width, data->height);
	}

	fclose(fp);
	return rc;
}

annotation_list_t * annotation_list_init(annotation_list_t * _list, ssize_t max_size)
{
	annotation_list_t * list = _list;
	if(NULL == list)
	{
		list = calloc(1, sizeof(*list));
	}
	assert(list);

	list->load = annotation_list_load;
	list->save = annotation_list_save;
	
	list->resize = annotation_list_resize;
	list->update = annotation_list_update;
	list->remove = annotation_list_remove;

	int rc = list->resize(list, max_size);
	assert(0 == rc);
	if(rc)
	{
		annotation_list_cleanup(list);
		if(NULL == _list) free(list);
		return NULL;
	}
	
	return list;
}
void annotation_list_cleanup(annotation_list_t * list)
{
	if(NULL == list) return;

	if(list->data)
	{
		for(ssize_t i = 0; i < list->length; ++i)
		{
			free(list->data[i]);
			list->data[i] = NULL;
		}
		free(list->data);
		list->data = NULL;
	}
	list->length = 0;
	list->max_size = 0;
	return;
}


void annotation_list_dump(const annotation_list_t * list)
{
	assert(list);
	for(ssize_t i = 0; i < list->length; ++i)
	{
		const annotation_data_t * data = list->data[i];
		assert(data);

		printf("list[%d]: %d - { %.3f, %.3f, %.3f, %.3f }\n", (int)i,
			data->klass,
			data->x, data->y, data->width, data->height);
	}
	return;
}

#if defined(_TEST_ANNOTATION_LIST) && defined(_STAND_ALONE)


int main(int argc, char ** argv, char ** envs)
{
	annotation_list_t * list = annotation_list_init(NULL, 0);
	assert(list);

	const char * filename = "data/1.jpg.txt";
	int rc = 0;

	if(argc > 1) {
		for(int i = 1; i < argc; ++i)
		{
			filename = argv[i];
			assert(filename && filename[0]);
			ssize_t count = list->load(list, filename);
			assert(count >= 0);

			annotation_list_dump(list);
		}
	}else
	{
		ssize_t count = list->load(list, filename);
		assert(count >= 0);

		annotation_list_dump(list);
	}

	annotation_list_cleanup(list);
	free(list);
	return rc;
}


#endif
