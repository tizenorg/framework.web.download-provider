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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <dlfcn.h> // dlopen

#include <sys/smack.h>
#include "download-provider.h"
#include "download-provider-log.h"
#include "download-provider-pthread.h"
#include "download-provider-socket.h"
#include "download-provider-db.h"
#include "download-provider-queue.h"
#include "download-provider-notification.h"
#include "download-provider-request.h"

#include "download-agent-defs.h"
#include "download-agent-interface.h"

static void *g_da_handle = NULL;
static int (*download_agent_init)(da_client_cb_t *) = NULL; // int da_init(da_client_cb_t *da_client_callback);
static int (*download_agent_deinit)() = NULL; //  int da_deinit();
static int (*download_agent_is_alive)(int) = NULL;  // int da_is_valid_download_id(int download_id);
static int (*download_agent_suspend)(int) = NULL; // int da_suspend_download(int download_id);
static int (*download_agent_resume)(int) = NULL; // int da_resume_download(int download_id);
static int (*download_agent_cancel)(int) = NULL; // int da_cancel_download(int download_id);
static int (*download_agent_start)(const char *, extension_data_t *, int *) = NULL; // int da_start_download_with_extension(const char *url, extension_data_t *ext_data, int *download_id);


int dp_is_file_exist(const char *file_path)
{
	struct stat file_state;
	int stat_ret;

	if (file_path == NULL) {
		TRACE_ERROR("[NULL-CHECK] file path is NULL");
		return -1;
	}

	stat_ret = stat(file_path, &file_state);

	if (stat_ret == 0)
		if (file_state.st_mode & S_IFREG)
			return 0;

	return -1;
}

static int __change_error(int err)
{
	int ret = DP_ERROR_NONE;
	switch (err) {
	case DA_RESULT_OK:
		ret = DP_ERROR_NONE;
		break;
	case DA_ERR_INVALID_ARGUMENT:
		ret = DP_ERROR_INVALID_PARAMETER;
		break;
	case DA_ERR_FAIL_TO_MEMALLOC:
		ret = DP_ERROR_OUT_OF_MEMORY;
		break;
	case DA_ERR_UNREACHABLE_SERVER:
		ret = DP_ERROR_NETWORK_UNREACHABLE;
		break;
	case DA_ERR_HTTP_TIMEOUT:
		ret = DP_ERROR_CONNECTION_TIMED_OUT;
		break;
	case DA_ERR_DISK_FULL:
		ret = DP_ERROR_NO_SPACE;
		break;
	case DA_ERR_INVALID_STATE:
		ret = DP_ERROR_INVALID_STATE;
		break;
	case DA_ERR_NETWORK_FAIL:
		ret = DP_ERROR_CONNECTION_FAILED;
		break;
	case DA_ERR_INVALID_URL:
		ret = DP_ERROR_INVALID_URL;
		break;
	case DA_ERR_INVALID_INSTALL_PATH:
		ret = DP_ERROR_INVALID_DESTINATION;
		break;
	case DA_ERR_ALREADY_MAX_DOWNLOAD:
		ret = DP_ERROR_TOO_MANY_DOWNLOADS;
		break;
	case DA_ERR_FAIL_TO_CREATE_THREAD:
	case DA_ERR_FAIL_TO_OBTAIN_MUTEX:
	case DA_ERR_FAIL_TO_ACCESS_FILE:
	case DA_ERR_FAIL_TO_GET_CONF_VALUE:
	case DA_ERR_FAIL_TO_ACCESS_STORAGE:
	default:
		ret = DP_ERROR_IO_ERROR;
		break;
	}
	return ret;
}

static void __download_info_cb(user_download_info_t *info, void *user_data)
{
	if (!info) {
		TRACE_ERROR("[NULL-CHECK] Agent info");
		return ;
	}
	dp_request_slots *request_slot = (dp_request_slots *) user_data;
	if (request_slot == NULL) {
		TRACE_ERROR("[NULL-CHECK] request req_id:%d", info->download_id);
		return ;
	}
	CLIENT_MUTEX_LOCK(&request_slot->mutex);
	if (request_slot->request == NULL) {
		TRACE_ERROR("[NULL-CHECK] request req_id:%d", info->download_id);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}
	dp_request *request = request_slot->request;
	if (request->id < 0 || (request->agent_id != info->download_id)) {
		TRACE_ERROR("[NULL-CHECK] agent_id : %d req_id %d",
			request->agent_id, info->download_id);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}

	int request_id = request->id;

	// update info before sending event
	if (info->tmp_saved_path) {
		TRACE_SECURE_INFO("[STARTED][%d] [%s]", request_id, info->tmp_saved_path);
		if (dp_db_replace_column(request_id, DP_DB_TABLE_DOWNLOAD_INFO,
				DP_DB_COL_TMP_SAVED_PATH, DP_DB_COL_TYPE_TEXT,
				info->tmp_saved_path) == 0) {
			if (info->file_type) {
				TRACE_SECURE_INFO("[MIME-TYPE][%d] [%s]", request_id, info->file_type);
				if (dp_db_set_column(request_id, DP_DB_TABLE_DOWNLOAD_INFO,
						DP_DB_COL_MIMETYPE, DP_DB_COL_TYPE_TEXT,
						info->file_type) < 0)
					TRACE_ERROR("[ERROR][%d][SQL]", request_id);
			}

			if (info->file_size > 0) {
				TRACE_SECURE_INFO
					("[FILE-SIZE][%d] [%lld]", request_id, info->file_size);
				request->file_size = info->file_size;
				if (dp_db_set_column
						(request_id, DP_DB_TABLE_DOWNLOAD_INFO,
						DP_DB_COL_CONTENT_SIZE,
						DP_DB_COL_TYPE_INT64, &info->file_size) < 0)
					TRACE_ERROR("[ERROR][%d][SQL]", request_id);
			}

			if (info->content_name) {
				TRACE_SECURE_INFO
					("[CONTENTNAME][%d] [%s]", request_id, info->content_name);
				if (dp_db_set_column
						(request_id, DP_DB_TABLE_DOWNLOAD_INFO,
						DP_DB_COL_CONTENT_NAME,
						DP_DB_COL_TYPE_TEXT, info->content_name) < 0)
					TRACE_ERROR("[ERROR][%d][SQL]", request_id);
			}
			if (info->etag) {
				TRACE_SECURE_INFO("[ETAG][%d] [%s]", request_id, info->etag);
				if (dp_db_replace_column
						(request_id, DP_DB_TABLE_DOWNLOAD_INFO, DP_DB_COL_ETAG,
						DP_DB_COL_TYPE_TEXT, info->etag) < 0)
					TRACE_ERROR("[ERROR][%d][SQL]", request_id);
			}
		} else {
			TRACE_ERROR
				("[ERROR][%d][SQL] failed to insert downloadinfo",
				request_id);
		}
	}

	request->state = DP_STATE_DOWNLOADING;
	if (dp_db_set_column(request->id, DP_DB_TABLE_LOG, DP_DB_COL_STATE,
			DP_DB_COL_TYPE_INT, &request->state) < 0)
		TRACE_ERROR("[ERROR][%d][SQL]", request->id);

	if (request->group && request->group->event_socket >= 0 &&
		request->state_cb)
		dp_ipc_send_event(request->group->event_socket,
			request->id, DP_STATE_DOWNLOADING, DP_ERROR_NONE, 0);

	if (request->auto_notification)
		request->noti_priv_id =
			dp_set_downloadinginfo_notification
				(request->id, request->packagename);

	CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
}

static void __progress_cb(user_progress_info_t *info, void *user_data)
{
	if (!info) {
		TRACE_ERROR("[NULL-CHECK] Agent info");
		return ;
	}
	dp_request_slots *request_slot = (dp_request_slots *) user_data;
	if (request_slot == NULL) {
		TRACE_ERROR("[NULL-CHECK] request req_id:%d", info->download_id);
		return ;
	}
	CLIENT_MUTEX_LOCK(&request_slot->mutex);
	if (request_slot->request == NULL) {
		TRACE_ERROR("[NULL-CHECK] request req_id:%d", info->download_id);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}
	dp_request *request = request_slot->request;
	if (request->id < 0 || (request->agent_id != info->download_id)) {
		TRACE_ERROR("[NULL-CHECK] agent_id : %d req_id %d",
			request->agent_id, info->download_id);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}

	if (request->state == DP_STATE_DOWNLOADING) {
		request->received_size = info->received_size;
		time_t tt = time(NULL);
		struct tm *localTime = localtime(&tt);
		// send event every 1 second.
		if (request->progress_lasttime != localTime->tm_sec) {
			request->progress_lasttime = localTime->tm_sec;
			if (request->progress_cb && request->group &&
				request->group->event_socket >= 0 &&
				request->received_size > 0)
				dp_ipc_send_event(request->group->event_socket,
					request->id, request->state, request->error,
					request->received_size);
			if (request->auto_notification)
				dp_update_downloadinginfo_notification
					(request->noti_priv_id,
					(double)request->received_size,
					(double)request->file_size);
		}
	}
	CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
}

static void __finished_cb(user_finished_info_t *info, void *user_data)
{
	if (!info) {
		TRACE_ERROR("[NULL-CHECK] Agent info");
		return ;
	}
	TRACE_INFO("Agent ID[%d] err[%d] http_status[%d]",
		info->download_id, info->err, info->http_status);
	dp_request_slots *request_slot = (dp_request_slots *) user_data;
	if (request_slot == NULL) {
		TRACE_ERROR("[NULL-CHECK] request req_id:%d", info->download_id);
		return ;
	}
	CLIENT_MUTEX_LOCK(&request_slot->mutex);
	if (request_slot->request == NULL) {
		TRACE_ERROR("[NULL-CHECK] request req_id:%d", info->download_id);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}
	dp_request *request = request_slot->request;
	if (request->id < 0 || (request->agent_id != info->download_id)) {
		TRACE_ERROR("[NULL-CHECK] agent_id : %d req_id %d",
			request->agent_id, info->download_id);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}

	int request_id = request->id;
	dp_credential cred = request->credential;
	dp_state_type state = DP_STATE_NONE;
	dp_error_type errorcode = DP_ERROR_NONE;

	// update info before sending event
	if (dp_db_update_date
			(request_id, DP_DB_TABLE_LOG, DP_DB_COL_ACCESS_TIME) < 0)
		TRACE_ERROR("[ERROR][%d][SQL]", request_id);

	if (info->http_status > 0)
		if (dp_db_replace_column(request_id, DP_DB_TABLE_DOWNLOAD_INFO,
				DP_DB_COL_HTTP_STATUS,
				DP_DB_COL_TYPE_INT, &info->http_status) < 0)
			TRACE_ERROR("[ERROR][%d][SQL]", request_id);

	if (info->err == DA_RESULT_OK) {
		if (info->saved_path) {
			char *str = NULL;
			char *content_name = NULL;
			char *dir_path = NULL;
			char *dir_label = NULL;
			char *smack_label = request->credential.smack_label;;
			size_t len = 0;
			int ret = 0;
			str = strrchr(info->saved_path, '/');
			if (str) {
				str++;
				content_name = dp_strdup(str);
				TRACE_SECURE_INFO("[PARSE][%d] content_name [%s]",
					request_id, content_name);
			}
			TRACE_INFO
				("[chown][%d] [%d][%d]", request_id, cred.uid, cred.gid);
			if (chown(info->saved_path, cred.uid, cred.gid) < 0)
				TRACE_STRERROR("[ERROR][%d] chown", request_id);
			if (chmod(info->saved_path,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0)
				TRACE_STRERROR("[ERROR][%d] chmod", request_id);

			len = str - (info->saved_path) -1;
			dir_path = (char *)calloc(len + 1, sizeof(char));
			if (dir_path) {
				strncpy(dir_path, info->saved_path, len);
				TRACE_SECURE_INFO("[PARSE][%d] dir path [%s]",
									request_id, dir_path);
				ret = smack_getlabel(dir_path, &dir_label,
						SMACK_LABEL_TRANSMUTE);
				if (ret == 0 && dir_label) {
					ret = strncmp(dir_label, "TRUE", strlen(dir_label));
					TRACE_SECURE_INFO("[SMACK][%d] transmute label [%s]",
							request_id, dir_path);
					if (ret == 0) {
						free(dir_label);
						ret = smack_getlabel(dir_path, &dir_label,
								SMACK_LABEL_ACCESS);
						if (ret == 0) {
							ret = smack_have_access(
									request->credential.smack_label,
									dir_label, "t");
							if (ret > 0) {
								smack_label = dir_label;
							} else {
								TRACE_SECURE_ERROR("[%d][SMACK ERROR]ret[%d]",
										request_id, ret);
								errorcode = DP_ERROR_PERMISSION_DENIED;
							}
						} else {
							TRACE_SECURE_ERROR("[%d][SMACK ERROR]", request_id);
							errorcode = DP_ERROR_PERMISSION_DENIED;
						}
					}
				}

			}
			ret = smack_setlabel(info->saved_path, smack_label, SMACK_LABEL_ACCESS);
			if (ret != 0) {
				TRACE_SECURE_ERROR("[%d][SMACK ERROR]", request_id);
				errorcode = DP_ERROR_PERMISSION_DENIED;
			}

			free(dir_label);
			free(dir_path);
			smack_label = NULL;

			if (dp_db_replace_column
					(request_id, DP_DB_TABLE_DOWNLOAD_INFO,
					DP_DB_COL_SAVED_PATH,
					DP_DB_COL_TYPE_TEXT, info->saved_path) == 0) {
				if (content_name != NULL) {
					if (dp_db_set_column
							(request_id, DP_DB_TABLE_DOWNLOAD_INFO,
							DP_DB_COL_CONTENT_NAME,
							DP_DB_COL_TYPE_TEXT, content_name) < 0)
						TRACE_ERROR("[ERROR][%d][SQL]", request_id);
				}
			} else {
				TRACE_ERROR("[ERROR][%d][SQL]", request_id);
			}
			if (content_name != NULL)
				free(content_name);
			if (errorcode == DP_ERROR_NONE) {
				state = DP_STATE_COMPLETED;
			} else {
				state = DP_STATE_FAILED;
			}
			TRACE_SECURE_INFO("[COMPLETED][%d] saved to [%s]",
					request_id, info->saved_path);
		} else {
			TRACE_ERROR("Cannot enter here");
			TRACE_ERROR("[ERROR][%d] No SavedPath", request_id);
			errorcode = DP_ERROR_INVALID_DESTINATION;
			state = DP_STATE_FAILED;
		}
		if (request->file_size == 0) {
			request->file_size = request->received_size;
			if (dp_db_replace_column
					(request_id, DP_DB_TABLE_DOWNLOAD_INFO,
					DP_DB_COL_CONTENT_SIZE,
					DP_DB_COL_TYPE_INT64, &request->file_size ) < 0)
				TRACE_ERROR("[ERROR][%d][SQL]", request_id);
		}
	} else if (info->err == DA_RESULT_USER_CANCELED) {
		state = DP_STATE_CANCELED;
		errorcode = DP_ERROR_NONE;
		TRACE_INFO("[CANCELED][%d]", request_id);
	} else {
		state = DP_STATE_FAILED;
		errorcode = __change_error(info->err);
		TRACE_INFO("[FAILED][%d][%s]", request_id,
				dp_print_errorcode(errorcode));
	}

	if (dp_db_set_column
			(request_id, DP_DB_TABLE_LOG, DP_DB_COL_STATE,
			DP_DB_COL_TYPE_INT, &state) < 0)
		TRACE_ERROR("[ERROR][%d][SQL]", request_id);

	if (errorcode != DP_ERROR_NONE) {
		if (dp_db_set_column(request_id, DP_DB_TABLE_LOG,
				DP_DB_COL_ERRORCODE, DP_DB_COL_TYPE_INT,
				&errorcode) < 0)
			TRACE_ERROR("[ERROR][%d][SQL]", request_id);
	}

	request->state = state;
	request->error = errorcode;

	// stay on memory till called destroy by client or timeout
	if (request->group != NULL && request->group->event_socket >= 0) {
		/* update the received file size.
		* The last received file size cannot update
		* because of reducing update algorithm*/
		if (request->received_size > 0)
			dp_ipc_send_event(request->group->event_socket,
				request->id, DP_STATE_DOWNLOADING, request->error,
				request->received_size);
		if (request->state_cb)
			dp_ipc_send_event(request->group->event_socket,
				request->id, request->state, request->error, 0);
		request->group->queued_count--;
	}

	// to prevent the crash . check packagename of request
	if (request->auto_notification && request->packagename != NULL)
		request->noti_priv_id =
			dp_set_downloadedinfo_notification(request->noti_priv_id,
				request->id, request->packagename, request->state);

	request->stop_time = (int)time(NULL);

	CLIENT_MUTEX_UNLOCK(&request_slot->mutex);

	dp_thread_queue_manager_wake_up();
}

static void __paused_cb(user_paused_info_t *info, void *user_data)
{
	TRACE_INFO("");
	dp_request_slots *request_slot = (dp_request_slots *) user_data;
	if (request_slot == NULL) {
		TRACE_ERROR("[NULL-CHECK] request req_id:%d", info->download_id);
		return ;
	}
	CLIENT_MUTEX_LOCK(&request_slot->mutex);
	if (request_slot->request == NULL) {
		TRACE_ERROR("[NULL-CHECK] request req_id:%d", info->download_id);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}
	dp_request *request = request_slot->request;
	if (request->id < 0 || (request->agent_id != info->download_id)) {
		TRACE_ERROR("[NULL-CHECK] agent_id : %d req_id %d",
			request->agent_id, info->download_id);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}

	if (request->state != DP_STATE_PAUSE_REQUESTED) {
		TRACE_ERROR("[CHECK] now status:%d", request->state);
		CLIENT_MUTEX_UNLOCK(&request_slot->mutex);
		return ;
	}

	int request_id = request->id;

	if (dp_db_update_date
			(request_id, DP_DB_TABLE_LOG, DP_DB_COL_ACCESS_TIME) < 0)
		TRACE_ERROR("[ERROR][%d][SQL]", request_id);

	request->state = DP_STATE_PAUSED;

	if (dp_db_set_column
			(request->id, DP_DB_TABLE_LOG, DP_DB_COL_STATE,
			DP_DB_COL_TYPE_INT, &request->state) < 0) {
		TRACE_ERROR("[ERROR][%d][SQL]", request->id);
	}

	if (request->group &&
		request->group->event_socket >= 0 && request->state_cb)
		dp_ipc_send_event(request->group->event_socket,
			request->id, request->state, request->error, 0);

	if (request->group)
		request->group->queued_count--;

	CLIENT_MUTEX_UNLOCK(&request_slot->mutex);

	dp_thread_queue_manager_wake_up();
}

int dp_init_agent()
{

	g_da_handle = dlopen("/usr/lib/libdownloadagent2.so", RTLD_LAZY | RTLD_GLOBAL);
	if (!g_da_handle) {
		TRACE_ERROR("[dlopen] %s", dlerror());
		g_da_handle = NULL;
		return DP_ERROR_OUT_OF_MEMORY;
	}
	dlerror();    /* Clear any existing error */

	*(void **) (&download_agent_init) = dlsym(g_da_handle, "da_init");
	if (download_agent_init == NULL ) {
		TRACE_ERROR("[dlsym] da_init:%s", dlerror());
		dlclose(g_da_handle);
		g_da_handle = NULL;
		return DP_ERROR_OUT_OF_MEMORY;
	}

	*(void **) (&download_agent_deinit)  = dlsym(g_da_handle, "da_deinit");
	if (download_agent_deinit == NULL ) {
		TRACE_ERROR("[dlsym] da_deinit:%s", dlerror());
		dlclose(g_da_handle);
		g_da_handle = NULL;
		return DP_ERROR_OUT_OF_MEMORY;
	}

	*(void **) (&download_agent_is_alive)  = dlsym(g_da_handle, "da_is_valid_download_id");
	if (download_agent_is_alive == NULL ) {
		TRACE_ERROR("[dlsym] da_is_valid_download_id:%s", dlerror());
		dlclose(g_da_handle);
		g_da_handle = NULL;
		return DP_ERROR_OUT_OF_MEMORY;
	}

	*(void **) (&download_agent_suspend)  = dlsym(g_da_handle, "da_suspend_download");
	if (download_agent_suspend == NULL ) {
		TRACE_ERROR("[dlsym] da_suspend_download:%s", dlerror());
		dlclose(g_da_handle);
		g_da_handle = NULL;
		return DP_ERROR_OUT_OF_MEMORY;
	}

	*(void **) (&download_agent_resume)  = dlsym(g_da_handle, "da_resume_download");
	if (download_agent_resume == NULL ) {
		TRACE_ERROR("[dlsym] da_resume_download:%s", dlerror());
		dlclose(g_da_handle);
		g_da_handle = NULL;
		return DP_ERROR_OUT_OF_MEMORY;
	}

	*(void **) (&download_agent_cancel)  = dlsym(g_da_handle, "da_cancel_download");
	if (download_agent_cancel == NULL ) {
		TRACE_ERROR("[dlsym] da_cancel_download:%s", dlerror());
		dlclose(g_da_handle);
		g_da_handle = NULL;
		return DP_ERROR_OUT_OF_MEMORY;
	}

	*(void **) (&download_agent_start)  = dlsym(g_da_handle, "da_start_download_with_extension");
	if (download_agent_start == NULL ) {
		TRACE_ERROR("[dlsym] da_start_download_with_extension:%s", dlerror());
		dlclose(g_da_handle);
		g_da_handle = NULL;
		return DP_ERROR_OUT_OF_MEMORY;
	}

	int da_ret = -1;
	da_client_cb_t da_cb = {
		__download_info_cb,
		__progress_cb,
		__finished_cb,
		__paused_cb
	};
	da_ret = (*download_agent_init)(&da_cb);
	if (da_ret != DA_RESULT_OK) {
		return DP_ERROR_OUT_OF_MEMORY;
	}
	return DP_ERROR_NONE;
}

void dp_deinit_agent()
{
	if (g_da_handle != NULL) {
		if (download_agent_deinit != NULL)
			(*download_agent_deinit)();

		dlclose(g_da_handle);
		g_da_handle = NULL;
	}
}

// 1 : alive
// 0 : not alive
int dp_is_alive_download(int req_id)
{
	int da_ret = 0;
	if (req_id < 0) {
		TRACE_ERROR("[NULL-CHECK] req_id");
		return 0;
	}
	if (download_agent_is_alive != NULL)
		da_ret = (*download_agent_is_alive)(req_id);
	return da_ret;
}

// 0 : success
// -1 : failed
dp_error_type dp_cancel_agent_download(int req_id)
{
	if (req_id < 0) {
		TRACE_ERROR("[NULL-CHECK] req_id");
		return -1;
	}
	if (dp_is_alive_download(req_id) == 0) {
		TRACE_ERROR("[CHECK agent-id:%d] dead request", req_id);
		return -1;
	}
	if (download_agent_cancel != NULL) {
		if ((*download_agent_cancel)(req_id) == DA_RESULT_OK)
			return 0;
	}
	return -1;
}

// 0 : success
// -1 : failed
dp_error_type dp_pause_agent_download(int req_id)
{
	if (req_id < 0) {
		TRACE_ERROR("[NULL-CHECK] req_id");
		return -1;
	}
	if (dp_is_alive_download(req_id) == 0) {
		TRACE_ERROR("[CHECK agent-id:%d] dead request", req_id);
		return -1;
	}
	if (download_agent_suspend != NULL) {
		if ((*download_agent_suspend)(req_id) == DA_RESULT_OK)
			return 0;
	}
	return -1;
}


// 0 : success
// -1 : failed
// -2 : pended
dp_error_type dp_start_agent_download(dp_request_slots *request_slot)
{
	int da_ret = -1;
	int req_dl_id = -1;
	dp_error_type errorcode = DP_ERROR_NONE;
	extension_data_t ext_data = {0,};
	char *etag = NULL;

	TRACE_INFO("");
	if (request_slot == NULL) {
		TRACE_ERROR("[NULL-CHECK] download_clientinfo_slot");
		return DP_ERROR_INVALID_PARAMETER;
	}

	if (request_slot->request == NULL) {
		TRACE_ERROR("[NULL-CHECK] download_clientinfo_slot");
		return DP_ERROR_INVALID_PARAMETER;
	}

	dp_request *request = request_slot->request;

	char *url = dp_request_get_url(request->id, &errorcode);
	if (url == NULL) {
		TRACE_ERROR("[ERROR][%d] URL is NULL", request->id);
		return DP_ERROR_INVALID_URL;
	}
	char *destination =
		dp_request_get_destination(request->id, request, &errorcode);
	if (destination != NULL)
		ext_data.install_path = destination;

	char *filename =
		dp_request_get_filename(request->id, request, &errorcode);
	if (filename != NULL)
		ext_data.file_name = filename;

	// call start_download() of download-agent

	char *tmp_saved_path =
		dp_request_get_tmpsavedpath(request->id, request, &errorcode);
	if (tmp_saved_path) {
		etag = dp_request_get_etag(request->id, request, &errorcode);
		if (etag) {
			TRACE_INFO("[RESUME][%d]", request->id);
			ext_data.etag = etag;
			ext_data.temp_file_path = tmp_saved_path;
		} else {
			/* FIXME later : It is better to handle the unlink function in download agaent module
			 * or in upload the request data to memory after the download provider process is restarted */
			TRACE_SECURE_INFO("[RESTART][%d] try to remove tmp file [%s]",
				request->id, tmp_saved_path);
			if (dp_is_file_exist(tmp_saved_path) == 0)
				if (unlink(tmp_saved_path) != 0)
					TRACE_STRERROR
						("[ERROR][%d] remove file", request->id);
		}
	}
	char *pkg_name = dp_request_get_pkg_name(request->id, request, &errorcode);
	if (pkg_name != NULL)
		ext_data.pkg_name = pkg_name;
	// get headers list from httpheaders table(DB)
	int headers_count = dp_db_get_cond_rows_count
			(request->id, DP_DB_TABLE_HTTP_HEADERS, NULL, 0, NULL);
	if (headers_count > 0) {
		ext_data.request_header = calloc(headers_count, sizeof(char*));
		if (ext_data.request_header != NULL) {
			ext_data.request_header_count = dp_db_get_http_headers_list
				(request->id, (char**)ext_data.request_header);
		}
	}

	ext_data.user_data = (void *)request_slot;

	// call start API of agent lib
	if (download_agent_start != NULL)
		da_ret = (*download_agent_start)(url, &ext_data, &req_dl_id);
	if (ext_data.request_header_count > 0) {
		int len = 0;
		int i = 0;
		len = ext_data.request_header_count;
		for (i = 0; i < len; i++) {
			if (ext_data.request_header[i])
				free((void *)(ext_data.request_header[i]));
		}
		free(ext_data.request_header);
	}
	free(url);
	if (destination)
		free(destination);
	if (filename)
		free(filename);
	if (tmp_saved_path)
		free(tmp_saved_path);
	if (etag)
		free(etag);
	if (pkg_name)
		free(pkg_name);

	// if start_download() return error cause of maximun download limitation,
	// set state to DOWNLOAD_STATE_PENDED.
	if (da_ret == DA_ERR_ALREADY_MAX_DOWNLOAD) {
		TRACE_INFO("[PENDING][%d] DA_ERR_ALREADY_MAX_DOWNLOAD [%d]",
			request->id, da_ret);
		return DP_ERROR_TOO_MANY_DOWNLOADS;
	} else if (da_ret != DA_RESULT_OK) {
		TRACE_ERROR("[ERROR][%d] DP_ERROR_CONNECTION_FAILED [%d]",
			request->id, da_ret);
		return __change_error(da_ret);
	}
	TRACE_INFO("[SUCCESS][%d] agent_id [%d]", request->id, req_dl_id);
	request->agent_id = req_dl_id;
	return DP_ERROR_NONE;
}

dp_error_type dp_resume_agent_download(int req_id)
{
	int da_ret = -1;
	if (req_id < 0) {
		TRACE_ERROR("[NULL-CHECK] req_id");
		return DP_ERROR_INVALID_PARAMETER;
	}
	if (download_agent_resume != NULL)
		da_ret = (*download_agent_resume)(req_id);
	if (da_ret == DA_RESULT_OK)
		return DP_ERROR_NONE;
	else if (da_ret == DA_ERR_INVALID_STATE)
		return DP_ERROR_INVALID_STATE;
	return __change_error(da_ret);
}

