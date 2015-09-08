/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _Download_Agent_Plugin_Conf_H
#define _Download_Agent_Plugin_Conf_H

#include "download-agent-type.h"
#include "download-agent-interface.h"

da_ret_t get_user_agent_string(char **uagent_str);
char *get_proxy_address(void);
#ifdef _RAF_SUPPORT
//test code
void get_smart_bonding_vconf();
#endif

#endif
