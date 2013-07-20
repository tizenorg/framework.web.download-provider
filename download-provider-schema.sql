/* download-provider
*
* Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* PROPRIETARY/CONFIDENTIAL
*
* This software is the confidential and proprietary information of
* SAMSUNG ELECTRONICS ("Confidential Information"). You agree and acknowledge
* that this software is owned by Samsung and you shall not disclose
* such Confidential Information and shall use it only in accordance with the
* terms of the license agreement you entered into with SAMSUNG ELECTRONICS.
* SAMSUNG make no representations or warranties about the suitability
* of the software, either express or implied, including but not limited
* to the implied warranties of merchantability, fitness for a particular purpose,
* or non-infringement. SAMSUNG shall not be liable for any damages suffered
* by licensee arising out of or releated to this software.
*
*/

CREATE TABLE IF NOT EXISTS groups
(
id INTEGER UNIQUE PRIMARY KEY,
uid INTEGER DEFAULT 0,
gid INTEGER DEFAULT 0,
extra_int INTEGER DEFAULT 0,
packagename TEXT DEFAULT NULL,
smack_label TEXT DEFAULT NULL,
extra TEXT DEFAULT NULL,
date_first_connected DATE,
date_last_connected DATE
);

CREATE TABLE IF NOT EXISTS logging
(
id INTEGER UNIQUE PRIMARY KEY,
state INTEGER DEFAULT 0,
errorcode INTEGER DEFAULT 0,
startcount INTEGER DEFAULT 0,
packagename TEXT DEFAULT NULL,
createtime DATE,
accesstime DATE
);

CREATE TABLE IF NOT EXISTS requestinfo
(
id INTEGER UNIQUE PRIMARY KEY,
auto_download BOOLEAN DEFAULT 0,
state_event BOOLEAN DEFAULT 0,
progress_event BOOLEAN DEFAULT 0,
noti_enable BOOLEAN DEFAULT 0,
network_type TINYINT DEFAULT 0,
filename TEXT DEFAULT NULL,
destination TEXT DEFAULT NULL,
url TEXT DEFAULT NULL,
FOREIGN KEY(id) REFERENCES logging(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS downloadinfo
(
id INTEGER UNIQUE PRIMARY KEY,
http_status INTEGER DEFAULT 0,
content_size UNSIGNED BIG INT DEFAULT 0,
mimetype VARCHAR(64) DEFAULT NULL,
content_name TEXT DEFAULT NULL,
saved_path TEXT DEFAULT NULL,
tmp_saved_path TEXT DEFAULT NULL,
etag TEXT DEFAULT NULL,
FOREIGN KEY(id) REFERENCES logging(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS httpheaders
(
id INTEGER NOT NULL,
header_field TEXT DEFAULT NULL,
header_data TEXT DEFAULT NULL,
FOREIGN KEY(id) REFERENCES logging(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS notification
(
id INTEGER NOT NULL,
extra_key TEXT DEFAULT NULL,
extra_data TEXT DEFAULT NULL,
FOREIGN KEY(id) REFERENCES logging(id) ON DELETE CASCADE
);

CREATE UNIQUE INDEX IF NOT EXISTS requests_index ON logging (id, state, errorcode, packagename, createtime, accesstime);