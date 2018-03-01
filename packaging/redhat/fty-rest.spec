#
#    fty-rest - Common core REST API for 42ity project
#
#    Copyright (C) 2014 - 2018 Eaton
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif
Name:           fty-rest
Version:        1.0.0
Release:        1
Summary:        common core rest api for 42ity project
License:        GPL-2.0+
URL:            https://42ity.org
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  xmlto
BuildRequires:  gcc-c++
BuildRequires:  libsodium-devel
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRequires:  malamute-devel
BuildRequires:  libcidr-devel
BuildRequires:  cxxtools-devel
BuildRequires:  tntnet-devel
BuildRequires:  tntdb-devel
BuildRequires:  file-devel
BuildRequires:  fty-proto-devel
BuildRequires:  libsasl2-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
fty-rest common core rest api for 42ity project.

%package -n libfty_rest1
Group:          System/Libraries
Summary:        common core rest api for 42ity project shared library

%description -n libfty_rest1
This package contains shared library for fty-rest: common core rest api for 42ity project

%post -n libfty_rest1 -p /sbin/ldconfig
%postun -n libfty_rest1 -p /sbin/ldconfig

%files -n libfty_rest1
%defattr(-,root,root)
%doc COPYING
# Note: this file was amended to include .so here too
# since tntnet shared object is not a typical library
%{_libdir}/libfty_rest.so*

%package devel
Summary:        common core rest api for 42ity project
Group:          System/Libraries
Requires:       libfty_rest1 = %{version}
Requires:       libsodium-devel
Requires:       zeromq-devel
Requires:       czmq-devel
Requires:       malamute-devel
Requires:       libcidr-devel
Requires:       cxxtools-devel
Requires:       tntnet-devel
Requires:       tntdb-devel
Requires:       file-devel
Requires:       fty-proto-devel
Requires:       libsasl2-devel

%description devel
common core rest api for 42ity project development tools
This package contains development files for fty-rest: common core rest api for 42ity project

%files devel
%defattr(-,root,root)
# Note: this file was amended to NOT include some files
# since tntnet shared object is not a typical library
# Note that the .so symlink is delivered by main "library" package
%{_includedir}/*
###%{_libdir}/libfty_rest.so
%{_libdir}/pkgconfig/libfty_rest.pc
%{_mandir}/man3/*
%{_mandir}/man7/*

# Note: Customization over zproject-generated code follows:
%package -n fty-rest-scripts
Group:          System/Libraries
Summary:        helper scripts used by fty-rest

%description -n fty-rest-scripts
This package contains helper scripts and data files used by the overall solution with fty-rest.

%files -n fty-rest-scripts
%defattr(-,root,root)
%{_libexecdir}/bios-passwd
%{_libexecdir}/testpass.sh
%{_datadir}/examples/tntnet.xml.example
#%{_datadir}/.git_details

%package -n fty-rest-clients
Group:          System/Libraries
Requires:       libfty_rest1 = %{version}
Summary:        binary programs using fty-rest elements

%description -n fty-rest-clients
This package contains binary programs that go along with fty-rest.

%files -n fty-rest-clients
%defattr(-,root,root)
### TODO : Makefile, install and uncomment
%{_bindir}/warranty/warranty-metric
%{_mandir}/man1/warranty/warranty-metric*
#%{_libexecdir}/bios-csv

%package -n fty-rest
Group:          System/Libraries
Requires:       libfty_rest1 = %{version}
Requires:       fty-rest-clients = %{version}
Requires:       fty-rest-scripts = %{version}
Requires:       ipc-data
Requires:       augeas-tools
Requires:       tntdb-mysql
Requires:       libcidr0
Requires:       tntnet-runtime
Requires:       malamute
Requires:       libsnmp30
Requires:       cracklib-runtime
Requires:       msmtp
Summary:        grouping of end-user solution with fty-rest

%description -n fty-rest
This metapackage depends on actual packages needed to implement the core 42ity REST API with fty-rest for end-users of a product.


%prep

%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS} --with-tntnet=yes --with-libmagic=yes
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f


%changelog
