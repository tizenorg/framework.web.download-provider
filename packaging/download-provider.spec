Name:       download-provider
Summary:    download the contents in background.
Version:    1.1.6
Release:    0
Group:      Development/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): sys-assert
Requires(post): libdevice-node
Requires(post): org.tizen.indicator
Requires(post): org.tizen.quickpanel
Requires(post): sqlite
Requires(post): connman
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
* Tue Oct 29 2013 Jungki Kwak <jungki.kwak@samsung.com> 
- Resolve a bug about invalid url

* Tue Sep 17 2013 Jungki Kwak <jungki.kwak@samsung.com> 
- Add depenency of connman for smack label

* Thu Sep 12 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Resolve a bug to check return value
- Resolve build warnnings and add error exception
- Register download notification when client process is exited

* Wed Sep 11 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Implement auto-resume in ip-changed case
- Remove dbus-activation feature
- Implementation of new APIs for notification

* Tue Sep 05 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Update downloading icon for ongoing notification
- Return disk full error when file system call return in case of ENOSPC

* Tue Jul 09 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Change to use vconf for downloading icon of indicator

* Mon Jul 08 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Add to check smack enable state

* Thu Jul 04 2013 Jungki Kwak <jungki.kwak@samsung.com>
- Check smack integrity about install directory and downloaded file

