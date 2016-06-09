Name:       mmsvc-player
Summary:    A Media Player module for muse server
Version:    0.2.15
Release:    0
Group:      Multimedia/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	mmsvc-player.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(mused)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-player)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(capi-media-sound-manager)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(ecore-wayland)
BuildRequires:  pkgconfig(capi-media-tool)
BuildRequires:  pkgconfig(json-c)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(ttrace)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(eom)

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
A Media Player module for muse server and Tizen Native API.
%package devel
Summary:  A Media Player module for muse server.(Development)
Group:    Development/Multimedia
Requires: %{name} = %{version}-%{release}

%description devel
%devel_desc

%prep
%setup -q
cp %{SOURCE1001} .


%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
#export CFLAGS+=" -D_USE_X_DIRECT_"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif
export CFLAGS+=" -DPLAYER_ASM_COMPATIBILITY"
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DFULLVER=%{version} -DMAJORVER=${MAJORVER} \
%if "%{?profile}" == "tv" || "%{?profile}" == "wearable"
    -DEVAS_RENDERER_SUPPORT=Off
%else
    -DEVAS_RENDERER_SUPPORT=On
%endif

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest mmsvc-player.manifest
%license LICENSE.APLv2
%{_libdir}/liblegacy-player.so*
%{_libdir}/libmuse-player.so*
%{_bindir}/*

%files devel
%{_includedir}/media/*.h
%{_includedir}/media/*.def
%{_libdir}/pkgconfig/*.pc
%{_libdir}/liblegacy-player.so
%{_libdir}/libmuse-player.so
