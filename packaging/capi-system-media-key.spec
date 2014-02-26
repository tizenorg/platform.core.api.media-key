%bcond_with wayland
%bcond_with x

Name:       capi-system-media-key
Summary:    A System Information library in SLP C API
Version:    0.1.0
Release:    0
Group:      System/API
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: capi-system-media-key.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(aul)
%if %{with x}
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(utilX)
%endif

%description
%{summary}.


%package devel
Summary:  A Media Key library in SLP C API (Development)
Group:    System/Development
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
        -DWAYLAND_SUPPORT=On \
%else
        -DWAYLAND_SUPPORT=Off \
%endif
%if %{with x}
        -DX11_SUPPORT=On
%else
        -DX11_SUPPORT=Off
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

