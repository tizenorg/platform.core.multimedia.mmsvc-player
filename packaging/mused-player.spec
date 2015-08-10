%bcond_with wayland
%bcond_with x

Name:       mused-player
Summary:    A Media Daemon player library in Tizen Native API
Version:    0.3.1
Release:    0
Group:      Multimedia/API
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(mused)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-player)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(capi-media-sound-manager)
BuildRequires:  pkgconfig(legacy-capi-media-player)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(evas)
%if %{with x}
BuildRequires:  pkgconfig(ecore-x)
%endif
%if %{with wayland}
BuildRequires:  pkgconfig(ecore-wayland)
%endif
BuildRequires:  pkgconfig(capi-media-tool)
BuildRequires:  pkgconfig(json)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(eom)

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
A Media Player Daemon library in Tizen Native API.

%package devel
Summary:  A Media Player Daemon library in Tizen Native API.(Development)
Group:    Development/Multimedia
Requires: %{name} = %{version}-%{release}

%description devel

%prep
%setup -q


%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
#export CFLAGS+=" -D_USE_X_DIRECT_"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DFULLVER=%{version} -DMAJORVER=${MAJORVER} \
%if "%{?profile}" == "wearable"
	-DTIZEN_WEARABLE=YES \
    %else
    -DTIZEN_MOBILE=YES \
    %endif
%if %{with wayland}
    -DWAYLAND_SUPPORT=On \
%else
    -DWAYLAND_SUPPORT=Off \
%endif
%if %{with x}
    -DX11_SUPPORT=On \
%else
    -DX11_SUPPORT=Off \
%endif
    -DCMAKE_INSTALL_PREFIX=/usr -DFULLVER=%{version} -DMAJORVER=${MAJORVER} \
    -DCMAKE_INSTALL_PREFIX=/usr -DFULLVER=%{version} -DMAJORVER=${MAJORVER}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
mkdir -p %{buildroot}/usr/bin
cp LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}
cp client/test/player_test %{buildroot}/usr/bin
cp client/test/player_media_packet_test %{buildroot}/usr/bin
cp client/test/player_es_push_test %{buildroot}/usr/bin

%make_install

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest mused-player.manifest
%manifest client/capi-media-player.manifest
%{_libdir}/libmused-player.so.*
%{_libdir}/libcapi-media-player.so.*
%{_datadir}/license/%{name}
/usr/bin/player_test
/usr/bin/player_media_packet_test
/usr/bin/player_es_push_test

%files devel
%{_includedir}/media/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libmused-player.so
%{_libdir}/libcapi-media-player.so


