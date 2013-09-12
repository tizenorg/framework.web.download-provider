Name:       download-provider
Summary:    download the contents in background.
Version:    1.0.9
Release:    0
Group:      Development/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): sys-assert
Requires(post): libdevice-node
Requires(post): org.tizen.indicator
Requires(post): org.tizen.quickpanel
Requires(post): sqlite
BuildRequires:  cmake
BuildRequires:  libprivilege-control-conf
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(libsoup-2.4)
BuildRequires:  pkgconfig(xdgmime)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(bundle)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(capi-appfw-app-manager)
BuildRequires:  pkgconfig(capi-network-connection)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(wifi-direct)
BuildRequires:  pkgconfig(libsmack)
BuildRequires:  gettext-devel
BuildRequires:  pkgconfig(libsystemd-daemon)

%description
Description: download the contents in background

%package devel
Summary:    download-provider
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Description: download the contents in background (developement files)

%prep
%setup -q

%define _data_install_path /usr/share/%{name}
%define _imagedir %{_data_install_path}/images 
%define _localedir %{_data_install_path}/locales
%define _sqlschemadir %{_data_install_path}/sql
%define _databasedir /opt/usr/dbspace
%define _databasefile %{_databasedir}/.download-provider.db
%define _sqlschemafile %{_sqlschemadir}/download-provider-schema.sql
%define _licensedir /usr/share/license
%define _smackruledir /opt/etc/smack/accesses.d

%define cmake \
	CFLAGS="${CFLAGS:-%optflags} -fPIC -D_REENTRANT -fvisibility=hidden"; export CFLAGS \
	FFLAGS="${FFLAGS:-%optflags} -fPIC -fvisibility=hidden"; export FFLAGS \
	LDFLAGS+=" -Wl,--as-needed -Wl,--hash-style=both"; export LDFLAGS \
	%__cmake \\\
		-DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \\\
		-DBIN_INSTALL_DIR:PATH=%{_bindir} \\\
		-DLIB_INSTALL_DIR:PATH=%{_libdir} \\\
		-DINCLUDE_INSTALL_DIR:PATH=%{_includedir} \\\
		-DPKG_NAME=%{name} \\\
		-DPKG_VERSION=%{version} \\\
		-DPKG_RELEASE=%{release} \\\
		-DIMAGE_DIR:PATH=%{_imagedir} \\\
		-DLOCALE_DIR:PATH=%{_localedir} \\\
		-DDATABASE_SCHEMA_DIR=%{_sqlschemadir} \\\
		-DDATABASE_FILE:PATH=%{_databasefile} \\\
		-DDATABASE_SCHEMA_FILE=%{_sqlschemafile} \\\
		-DLICENSE_DIR:PATH=%{_licensedir} \\\
		-DSMACK_RULE_DIR:PATH=%{_smackruledir} \\\
		-DSUPPORT_WIFI_DIRECT:BOOL=OFF \\\
		-DSUPPORT_LOG_MESSAGE:BOOL=ON \\\
		-DSUPPORT_CHECK_IPC:BOOL=ON \\\
		%if "%{?_lib}" == "lib64" \
		%{?_cmake_lib_suffix64} \\\
		%endif \
		%{?_cmake_skip_rpath} \\\
		-DBUILD_SHARED_LIBS:BOOL=ON

%build
%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif
%cmake .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}%{_licensedir}
mkdir -p %{buildroot}/%{_data_install_path}
mkdir -p %{buildroot}%{_libdir}/systemd/system/graphical.target.wants
mkdir -p %{buildroot}%{_libdir}/systemd/system/sockets.target.wants
ln -s ../download-provider.service %{buildroot}%{_libdir}/systemd/system/graphical.target.wants/
ln -s ../download-provider.socket %{buildroot}%{_libdir}/systemd/system/sockets.target.wants/

%post
mkdir -p %{_databasedir}

if [ ! -f %{_databasefile} ];
then
sqlite3 %{_databasefile} '.read %{_sqlschemafile}'
chmod 660 %{_databasefile}
chmod 660 %{_databasefile}-journal
fi

%files
%defattr(-,root,root,-)
%manifest download-provider.manifest
%{_imagedir}/*.png
%{_imagedir}/*.gif
%{_localedir}/*
%{_libdir}/libdownloadagent2.so.0.0.1
%{_libdir}/libdownloadagent2.so
%{_libdir}/systemd/system/download-provider.service
%{_libdir}/systemd/system/graphical.target.wants/download-provider.service
%{_libdir}/systemd/system/download-provider.socket
%{_libdir}/systemd/system/sockets.target.wants/download-provider.socket
%{_libdir}/libdownload-provider-interface.so.%{version}
%{_libdir}/libdownload-provider-interface.so.0
%{_bindir}/%{name}
%{_licensedir}/%{name}
%{_smackruledir}/%{name}.rule
%{_sqlschemafile}

%files devel
%defattr(-,root,root,-)
%{_libdir}/libdownloadagent2.so.0.0.1
%{_libdir}/libdownloadagent2.so
%{_libdir}/libdownload-provider-interface.so
%{_includedir}/download-provider/download-provider-defs.h
%{_includedir}/download-provider/download-provider-interface.h
%{_bindir}/%{name}
%{_libdir}/pkgconfig/download-provider.pc
%{_libdir}/pkgconfig/download-provider-interface.pc

%changelog
* Wed Sep 04 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Update downloading icon for ongoing notification
- Changes for the registering downloading icon of Indicator

* Mon Aug 05 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Change to ignore uppper case when parsing http response header
- Change to permit the space at file name

* Mon Jul 15 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Resolve prevent issue
- Register download notification when client process is exited

* Wed Jul 03 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Check smack integrity about install directory and downloaded file
- Change the privilege of systemd configuration
- Change to use smack rule file

* Fri Jun 28 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Change to use the default storage as phone memory
- Change db file permission at post section
- Add smack rules

* Tue Jun 18 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Add smack rule about "_"
- Unlock mutex before calling libsoup cancel API
- Fix double lock when wakeup queue thread
- Disable DBUS-Activation by socket-activation of systemd
- Convert to systemd API
- Merge source codes from private master branch

* Thu May 15 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Create default download directory with app ownership

* Fri May 10 2013 Kwangmin Bang <justine.bang@samsung.com>
- raise the limitaion regarding the number of downloads which a process can make : 5 => 32

* Thu May 09 2013 Jungki Kwak <jungki.kwak@samsung.com>
- pthread_join for ending of queue-manager thread
- fix the segmentation fault if call download_get_url() without calling download_create()
- download_get_url() return wrong error with invalid id
- use dp_request_slots as user_data of da_agent
- remove warning message in build time
- Resolve a prevent issue
- PRAGMA synchronous=FULL
- Change to check mandatory value in started callback
- Resolve a bug about checking agent id
- Fix for 64 bit compatibility

* Wed Mar 27 2013 Kwangmin Bang <justine.bang@samsung.com>
- prevent defect : Explicit null dereferenced
- smack for dbus service

* Mon Mar 25 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Security coding : check all of parameters
- Security & privacy : remove private info from log message
- Missing errorcode

* Wed Mar 20 2013 Kwangmin Bang <justine.bang@samsung.com>
- increase the length limitation

* Tue Mar 19 2013 Kwangmin Bang <justine.bang@samsung.com>
- new API : dp_interface_get_http_header_field_list()

* Tue Mar 05 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Add function to handle credential URL
- Add functions for notification extra list.
- Close socket and group if failed to read the packet
- Return errorcode if failed to copy string
- Resolve wrong initialization
- Remove warning message in build time
- Functionize accepting the client
- Support N values per a extra-parem key
- Resolve a bug about converting error from agent
- Modify to check return value about DRM API
- Use enum value same with url-download
- Add to ignore SIGPIPE
- [prevent defect] Dereference before null check (REVERSE_INULL)
- [prevent defect] Logically dead code (DEADCODE)
- Check System Signal in all read call
- Apply "ignore EINTR" patch of url-download to provider-interface

* Fri Feb 01 2013 Kwangmin Bang <justine.bang@samsung.com>
- [smack] manifest for booting-script
- [prevent defect] Dereference after null check

* Thu Jan 31 2013 Kwangmin Bang <justine.bang@samsung.com>
- add the state which means is connecting to remote

* Wed Jan 30 2013 Kwangmin Bang <justine.bang@samsung.com>
- [smack] labeling for booting script
- [prevent defect] Dereference before/after null check 

* Tue Jan 29 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Add to check invalid state when setting the callback
- Resolve prevent defects
- Remove the dependancy with unnecessary package
- Remove the dependancy with dbus-glib
- Modify smack label of shared library

* Mon Jan 28 2013 Kwangmin Bang <justine.bang@samsung.com>
- recognize who launch me by parameter of main function
- ready the socket first before initializing dbus service

* Fri Jan 25 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Resolve compile error with GCC-4.7
- Convert error value in case the download start is failed
- Add to check the user's install direcoty
- Modify the name of license and add notice file
- Call sqlite3 in install section in spec file
- Terminate Only when support DBUS-Activation

* Fri Jan 11 2013 Kwangmin Bang <justine.bang@samsung.com>
- crash when terminated after clear requests all in timeout
- Change the flow of playready download
- audo_download need start_time

* Tue Jan 08 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Resolve prevent defects
- [Title] Fixed a bug that the icon of txt file can't be recognized and the txt file can't be opend
- Modify to check returned value of sqlite API
- Show error string in case of download CAPI failure

* Fri Dec 21 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Send file_size from memory first
- Add error code about libsoup error message
- Change the session time-out to 60 seconds
- Remove duplicated dlog message
- Modify a bug to get mime type from db
- Modify license information at spec file
- Resolve prevent defects
- Add missed existed db file when creaing db table
- The temporary file is not used any more
- Use SQL without MUTEX LOCK
- Support ECHO command
- Reduce SQL query to prevent SQL Error
- replace snprintf to sqlite3_mprintf for prepare sqlite3_stmt

* Fri Dec 14 2012 Kwangmin Bang <justine.bang@samsung.com>
- add credential in request structure
- change to ORDER BY createtime ASC from DESC in getting old list
- Resolve prevent defects
- Add boiler plates
- Remove old source codes
- apply the authentication by packagename for all commands
- fix memory leak in error case of DB query
- Add micro seconds value for temporary file name
- apply the limit count and sorting in SQL query
- do not load PAUSED request in booting time
- Add define statement for OMA DRM
- call IPC for sending event at the tail of function
- maintain DB info in DESTROY command
- Separate DB Table, reduce the amount of memory

* Fri Dec 07 2012 Kwangmin Bang <justine.bang@samsung.com>
- fix the crash when terminating by itself

* Thu Dec 06 2012 Kwangmin Bang <justine.bang@samsung.com>
- Fix the crash when accessing the history

* Fri Nov 30 2012 Kwangmin Bang <justine.bang@samsung.com>
- support DBUS-activation
- apply int64 for file size
- most API support ID_NOT_FOUND errorcode
- fix crash and memory leak in SET_EXTRA_PARAM
- fix memory leak in HTTP_HEADER case
- remove servicedata column
- Add to update the last received size

* Tue Nov 27 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Change to parse the content name
- Change the policy for temporary file name
- Change to get etag when download is failed
- DP_ERROR_ID_NOT_FOUND for set_url COMMAND
- new common sql function for getting/setting a column
- Add operation when registering notification
- disable flexible timeout, use 60 sec for timeout
- allow set_url even if unloaded from memory
- Add functions for notification extra param
- Block to can not re-use the request is completed
- send errorcode to client why failed to create request
- unload the request which not setted auto_download when client is terminated
- call da_agent_cancel after free slot
- Add debug message for event queue of client mgr
- Modify abort flow in case of deinit

* Fri Nov 16 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Resolve a bug about execute option API of ongoing notification
- load all request in queue in booting time
- stay on the memory although no client
- Dead default in switch
- Enable to register notification message
- Add to add, get and remove http header values
- Change functions to pause and resume which is based on download ID
- Unchecked return value
- clear history if registered before 48 hours or total of histories is over 1000.
- stay on memory if client is connected with provider
- advance auto-download feature
- remove __thread keyword for sqlite handle
- update regdate after copy history from queue
- concede network is connected although detail get API is failed in connection state changed callback.
- read pid from client when no support SO_PEERCRED
- wake up Queue thread only when exist ready request
- Add functions to save and submit http status code to client
- detect network changes using connection callback

* Thu Nov 08 2012 Jungki Kwak <jungki.kwak@samsung.com>
- In Offline state, do not search queue
- Apply changed enum value of connection CAPI

* Wed Oct 31 2012 Jungki Kwak <jungki.kwak@samsung.com>
- change the limitaiton count can download at once
- support Cancel after Pause
- Change the installation directory according to guide
- deal as error if write() return 0

* Mon Oct 28 2012 Kwangmin Bang <justine.bang@samsung.com>
- increase the timout of socket
- enhance socket update feature

* Thu Oct 25 2012 Kwangmin Bang <justine.bang@samsung.com>
- prevent defects
- change the limitation of length
- improve the performance logging to DB
- check packagename for resume call
- refresh FD_SET when only new socket is accepted
- Fix the socket error when a client is crashed

* Tue Oct 23 2012 Kwangmin Bang <justine.bang@samsung.com>
- terminate daemon in main socket error
- apply reading timeout for socket

* Mon Oct 22 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Resolve the lockup when client app is crashed
- Check the state before starting download
- Support paused callback

* Fri Oct 19 2012 Kwangmin Bang <justine.bang@samsung.com>
 - replace ID based download-provider to Major Process

* Tue Oct 16 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Install LICENSE file

* Fri Oct 12 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Install LICENSE file

* Thu Oct 11 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Use mutex when calling xdgmime API
- Do not use fsync function

* Mon Oct 08 2012 Kwangmin Bang <justine.bang@samsung.com>
- Fix the crash when come many stop with terminating App

* Fri Sep 28 2012 Kwangmin Bang <justine.bang@samsung.com>
- Add exception code about content-dispostion
- wait to free till callback is finished

* Mon Sep 24 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Add to define for db an library in manifest file

* Fri Sep 21 2012 Kwangmin Bang <justine.bang@samsung.com>
- prevent free slot till called notify_cb
- socket descriptor can be zero

* Fri Sep 21 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Apply manifest file in which the domain is defined
- Change way to pass extension data

* Fri Sep 14 2012 Kwangmin Bang <justine.bang@samsung.com>
- fix the crash by assert of pthread_mutex_lock
- NULL-check before free
- guarantee instant download for pended requests
- add descriptor
- fix the memory leak in error case of ipc_receive_request_msg
- guarantee one instant download per app
- call pthread_exit to terminate the currently running thread
- Remove pthread_cancel function

* Fri Sep 07 2012 Kwangmin Bang <justine.bang@samsung.com>
- add LICENSE
- Add to search download id from history db

* Thu Sep 06 2012 Kwangmin Bang <justine.bang@samsung.com>
- start to download again even if already finished
- change thread style
- arrange the request priority
- change data type
- wait till getting the response from client

* Mon Sep 03 2012 Kwangmin Bang <justine.bang@samsung.com>
- fix timeout error

* Mon Sep 03 2012 Kwangmin Bang <justine.bang@samsung.com>
- free slot after getting event from url-download
- fix INTEGER OVERFLOW

* Thu Aug 30 2012 Kwangmin Bang <justine.bang@samsung.com>
- initialize mutex for auto-redownloading
- support Pause/Resume with new connection
- fix the memory leak

* Mon Aug 27 2012 Kwangmin Bang <justine.bang@samsung.com>
- Change the ownership of downloaded file
- Add detached option when pthread is created
- fix the failure getting history info from database
- fix first timeout takes a long time
- fix wrong checking of network status
- fix the crash by double free
- divide log level
- Resolve prevent defects for agent module
- Resolve a bug to join domain in case of playready

* Tue Aug 23 2012 Kwangmin Bang <justine.bang@samsung.com>
- event thread does not deal in some state
- fix the lockup by mutex and the crash by invaild socket event

* Tue Aug 22 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Fix the crash when use notification
- One thread model for socket
- Fix the defects found by prevent tool
- Remove mutex lock/unlock in case of invalid id
- Support the status of download in case of getting new connection with requestid
- Clear db and register notification when stopped the download
- Update notification function
- Enable to set the defined file name by user

* Tue Aug 17 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Enable to use destination path
- Add to handle invalid id

* Tue Aug 16 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Change socket close timing

* Mon Aug 13 2012 Kwangmin Bang <justine.bang@samsung.com>
- Disable default dlog in launching script.

* Tue Aug 09 2012 Jungki Kwak <jungki.kwak@samsung.com>
- The function to init dbus glib is removed

* Tue Aug 08 2012 Jungki Kwak <jungki.kwak@samsung.com>
- The function to init dbus glib is added for connection network CAPI

* Tue Aug 07 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Change the name of temp direcoty.
- When add requestinfo to slot, save it to DB.

* Mon Aug 06 2012 Jungki Kwak <jungki.kwak@samsung.com>
- Initial version is updated.

