%bcond_with x
%bcond_with wayland

Name:       capi-system-media-key
Summary:    A System Information library in SLP C API
Version:    0.1.0
Release:    6
Group:      System/API
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	capi-system-media-key.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-input)
BuildRequires:  pkgconfig(evas)

%if %{with x}
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(utilX)
%endif
%if %{with wayland}
BuildRequires:  pkgconfig(ecore-wayland)
%endif

%description
%{summary}.


%package devel
Summary:  A Media Key library in SLP C API (Development)
Group:    System/API
Requires: %{name} = %{version}-%{release}

%description devel
%devel_desc

%prep
%setup -q
cp %{SOURCE1001} .

%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DFULLVER=%{version} -DMAJORVER=${MAJORVER} \
%if %{with wayland}
-DWITH_WAYLAND=TRUE
%endif
%if %{with x}
-DWITH_X11=TRUE
%endif

make %{?jobs:-j%jobs}

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%license  LICENSE
%{_libdir}/libcapi-system-media-key.so.*
%manifest capi-system-media-key.manifest

%files devel
%manifest %{name}.manifest
%{_includedir}/system/media_key.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libcapi-system-media-key.so


