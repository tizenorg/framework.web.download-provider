/*
 * Download Agent
 *
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jungki Kwak <jungki.kwak@samsung.com>, Keunsoon Lee <keunsoon.lee@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file		download-agent-mime-util.h
 * @brief
 * @author		Keunsoon Lee(keunsoon.lee@samsung.com)
 * @author		Jungki Kwak(jungki.kwak@samsung.com)
 ***/

#ifndef _Download_Agent_Mime_Table_H
#define _Download_Agent_Mime_Table_H

#include "download-agent-type.h"

#define NO_EXTENSION_NAME_STR "dat"

typedef struct {
	char *standard;
	char *normal;
} Ext_translation_table;

da_bool_t is_ambiguous_MIME_Type(const char *in_mime_type);
da_bool_t da_get_extension_name_from_url(char* url, char** ext);
da_result_t  da_mime_get_ext_name(char* mime, char **ext);
da_bool_t da_get_file_name_from_url(char* url, char** name) ;
void delete_prohibited_char(char* szTarget, int str_len);
#endif
