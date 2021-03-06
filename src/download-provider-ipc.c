#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "download-provider-ipc.h"
#include "download-provider-log.h"
#include "bundle.h"

int ipc_receive_header(int fd)
{
	if (fd <= 0)
		return -1;

	download_controls msgheader = 0;
	if (read(fd, &msgheader, sizeof(download_controls)) < 0) {
		TRACE_DEBUG_MSG("failed to read message header (%s)",
				strerror(errno));
		return -1;
	}
	return msgheader;
}

int ipc_send_stateinfo(download_clientinfo *clientinfo)
{
	if (!clientinfo || clientinfo->clientfd <= 0)
		return -1;

	download_state_info stateinfo;
	download_controls type = DOWNLOAD_CONTROL_GET_STATE_INFO;
	memset(&stateinfo, 0x00, sizeof(download_state_info));
	stateinfo.state = clientinfo->state;
	stateinfo.err = clientinfo->err;

	// send control
	if (send(clientinfo->clientfd, &type, sizeof(download_controls), 0) < 0) {
		TRACE_DEBUG_MSG("failed to send message header (%s)",
				strerror(errno));
		return -1;
	}
	if (send(clientinfo->clientfd, &stateinfo, sizeof(download_state_info), 0) < 0) {
		TRACE_DEBUG_MSG("failed to send message header (%s)",
				strerror(errno));
		return -1;
	}
	return type;
}

int ipc_send_request_stateinfo(download_clientinfo *clientinfo)
{
	if (!clientinfo || clientinfo->clientfd <= 0)
		return -1;

	download_request_state_info requeststateinfo;
	download_controls type = DOWNLOAD_CONTROL_GET_REQUEST_STATE_INFO;
	memset(&requeststateinfo, 0x00, sizeof(download_request_state_info));
	requeststateinfo.stateinfo.state = clientinfo->state;
	requeststateinfo.stateinfo.err = clientinfo->err;
	if (clientinfo->requestinfo)
		requeststateinfo.requestid = clientinfo->requestinfo->requestid;

	// send control
	if (send(clientinfo->clientfd, &type, sizeof(download_controls), 0) < 0) {
		TRACE_DEBUG_MSG("failed to send message header (%s)",
				strerror(errno));
		return -1;
	}
	if (send
		(clientinfo->clientfd, &requeststateinfo, sizeof(download_request_state_info),
		0) < 0) {
		TRACE_DEBUG_MSG("failed to send message header (%s)",
				strerror(errno));
		return -1;
	}
	return type;
}

int ipc_send_downloadinfo(download_clientinfo *clientinfo)
{
	if (!clientinfo || clientinfo->clientfd <= 0
		|| !clientinfo->downloadinfo)
		return -1;

	download_controls type = DOWNLOAD_CONTROL_GET_DOWNLOAD_INFO;
	// send control
	if (send(clientinfo->clientfd, &type, sizeof(download_controls), 0) < 0) {
		TRACE_DEBUG_MSG("failed to send message header (%s)",
				strerror(errno));
		return -1;
	}
	if (send
		(clientinfo->clientfd, clientinfo->downloadinfo,
		sizeof(download_content_info), 0) < 0) {
		TRACE_DEBUG_MSG("failed to send message header (%s)",
				strerror(errno));
		return -1;
	}
	return type;
}

int ipc_send_downloadinginfo(download_clientinfo *clientinfo)
{
	if (!clientinfo || clientinfo->clientfd <= 0
		|| !clientinfo->downloadinginfo)
		return -1;

	download_controls type = DOWNLOAD_CONTROL_GET_DOWNLOADING_INFO;
	// send control
	if (send(clientinfo->clientfd, &type, sizeof(download_controls), 0) < 0) {
		TRACE_DEBUG_MSG("failed to send message header (%s)",
				strerror(errno));
		return -1;
	}
	if (send
		(clientinfo->clientfd, clientinfo->downloadinginfo,
		sizeof(downloading_state_info), 0) < 0) {
		TRACE_DEBUG_MSG("failed to send message header (%s)",
				strerror(errno));
		return -1;
	}
	return type;
}

extern int service_import_from_bundle(service_h service, bundle *data);
int ipc_receive_request_msg(download_clientinfo *clientinfo)
{
	if (!clientinfo || clientinfo->clientfd <= 0)
		return -1;

	if (!clientinfo->requestinfo)
		clientinfo->requestinfo =
			(download_request_info *) calloc(1, sizeof(download_request_info));

	if (!clientinfo->requestinfo)
		return -1;

	// read reqeust structure
	if (read
		(clientinfo->clientfd, clientinfo->requestinfo,
		sizeof(download_request_info)) < 0) {
		TRACE_DEBUG_MSG("failed to read message header");
		return -1;
	}
	if (clientinfo->requestinfo->client_packagename.length > 1
		&& clientinfo->requestinfo->client_packagename.length < DP_MAX_STR_LEN) {
		clientinfo->requestinfo->client_packagename.str =
			(char *)
			calloc((clientinfo->requestinfo->client_packagename.length +
				1), sizeof(char));
		if (read
			(clientinfo->clientfd,
			clientinfo->requestinfo->client_packagename.str,
			clientinfo->requestinfo->client_packagename.length *
			sizeof(char)) < 0) {
			TRACE_DEBUG_MSG
				("failed to read message header client_app_id(%s)",
				strerror(errno));
			return -1;
		}
		clientinfo->requestinfo->client_packagename.str[clientinfo->
								requestinfo->
								client_packagename.
								length] = '\0';
		TRACE_DEBUG_INFO_MSG("request client_packagename [%s]",
				clientinfo->requestinfo->client_packagename.
				str);
	}
	if (clientinfo->requestinfo->url.length > 1
		&& clientinfo->requestinfo->url.length < DP_MAX_URL_LEN) {
		clientinfo->requestinfo->url.str =
			(char *)calloc((clientinfo->requestinfo->url.length + 1),
					sizeof(char));
		if (read
			(clientinfo->clientfd, clientinfo->requestinfo->url.str,
			clientinfo->requestinfo->url.length * sizeof(char)) < 0) {
			TRACE_DEBUG_MSG("failed to read message header url(%s)",
					strerror(errno));
			return -1;
		}
		clientinfo->requestinfo->url.str[clientinfo->requestinfo->url.
							length] = '\0';
		TRACE_DEBUG_INFO_MSG("request url [%s]",
				clientinfo->requestinfo->url.str);
	}
	if (clientinfo->requestinfo->install_path.length > 1
		&& clientinfo->requestinfo->install_path.length < DP_MAX_PATH_LEN) {
		clientinfo->requestinfo->install_path.str =
			(char *)
			calloc((clientinfo->requestinfo->install_path.length + 1),
				sizeof(char));
		if (read
			(clientinfo->clientfd,
			clientinfo->requestinfo->install_path.str,
			clientinfo->requestinfo->install_path.length *
			sizeof(char)) < 0) {
			TRACE_DEBUG_MSG
				("failed to read message header install_path(%s)",
				strerror(errno));
			return -1;
		}
		clientinfo->requestinfo->install_path.str[clientinfo->
								requestinfo->
								install_path.length] =
			'\0';
		TRACE_DEBUG_INFO_MSG("request install_path [%s]",
				clientinfo->requestinfo->install_path.str);
	}
	if (clientinfo->requestinfo->filename.length > 1
		&& clientinfo->requestinfo->filename.length < DP_MAX_STR_LEN) {
		clientinfo->requestinfo->filename.str =
			(char *)
			calloc((clientinfo->requestinfo->filename.length + 1),
				sizeof(char));
		if (read
			(clientinfo->clientfd,
			clientinfo->requestinfo->filename.str,
			clientinfo->requestinfo->filename.length * sizeof(char)) <
			0) {
			TRACE_DEBUG_MSG
				("failed to read message header filename(%s)",
				strerror(errno));
			return -1;
		}
		clientinfo->requestinfo->filename.str[clientinfo->requestinfo->
								filename.length] = '\0';
		TRACE_DEBUG_INFO_MSG("request filename [%s]",
				clientinfo->requestinfo->filename.str);
	}
	if (clientinfo->requestinfo->service_data.length > 1) {
		bundle_raw *raw_data = NULL;
		int len = 0;
		bundle *b = NULL;
		service_h service_handle;
		clientinfo->requestinfo->service_data.str =
			(char *)
			calloc((clientinfo->requestinfo->service_data.length + 1),
				sizeof(char));
		if (read
			(clientinfo->clientfd,
			clientinfo->requestinfo->service_data.str,
			clientinfo->requestinfo->service_data.length * sizeof(char)) <
			0) {
			TRACE_DEBUG_MSG
				("failed to read message header filename(%s)",
				strerror(errno));
			return -1;
		}
		clientinfo->requestinfo->service_data.str[clientinfo->requestinfo->
								service_data.length] = '\0';
		raw_data = (bundle_raw *)clientinfo->requestinfo->service_data.str;
		len = clientinfo->requestinfo->service_data.length;
		if ((b = bundle_decode(raw_data, len)) == NULL) {
			TRACE_DEBUG_MSG("Failed to decode bundle raw data");
			return -1;
		}
		if (service_create(&service_handle) < 0) {
			TRACE_DEBUG_MSG("Failed to create service handle");
			bundle_free(b);
			return -1;
		}
		if (service_import_from_bundle(service_handle, b) < 0) {
			TRACE_DEBUG_MSG("Failed to import service handle");
			bundle_free(b);
			service_destroy(service_handle);
			return -1;
		}

		clientinfo->service_handle = service_handle;
		char *pkg = NULL;
		service_get_package(service_handle, &pkg);
		if (pkg) {
			TRACE_DEBUG_MSG("operation### [%s]",pkg);
			free(pkg);
		}

		bundle_free(b);
	}
	if (clientinfo->requestinfo->headers.rows) {
		clientinfo->requestinfo->headers.str =
			(download_flexible_string *) calloc(clientinfo->requestinfo->headers.
						rows, sizeof(download_flexible_string));
		int i = 0;
		for (i = 0; i < clientinfo->requestinfo->headers.rows; i++) {
			if (read
				(clientinfo->clientfd,
				&clientinfo->requestinfo->headers.str[i],
				sizeof(download_flexible_string)) < 0) {
				TRACE_DEBUG_MSG
					("failed to read message header headers(%s)",
					strerror(errno));
				return -1;
			}
			if (clientinfo->requestinfo->headers.str[i].length > 1) {
				TRACE_DEBUG_INFO_MSG("headers[%d] length[%d]", i,
						clientinfo->requestinfo->
						headers.str[i].length);
				clientinfo->requestinfo->headers.str[i].str =
					(char *)
					calloc((clientinfo->requestinfo->headers.
						str[i].length + 1), sizeof(char));
				if (read
					(clientinfo->clientfd,
					clientinfo->requestinfo->headers.str[i].
					str,
					clientinfo->requestinfo->headers.str[i].
					length * sizeof(char)) < 0) {
					TRACE_DEBUG_MSG
						("failed to read message header headers(%s)",
						strerror(errno));
					return -1;
				}
				clientinfo->requestinfo->headers.str[i].
					str[clientinfo->requestinfo->headers.str[i].
					length] = '\0';
				TRACE_DEBUG_INFO_MSG("headers[%d][%s]", i,
						clientinfo->requestinfo->
						headers.str[i].str);
			}
		}
	}
	return 0;
}
