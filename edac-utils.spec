Name:      edac-utils
Version:   See META
Release:   See META

Summary:   Userspace helper for kernel EDAC drivers (ECC)
Group:     Applications/System
License:   GPL
Source:    %{name}-%{version}.tgz
BuildRoot: %{_tmppath}/%{name}-%{version}

%{?el5:%define _with_libsysfs 1}

%if 0%{?_with_libsysfs}
BuildRequires: libsysfs-devel
Requires: libsysfs
%else
BuildRequires: sysfsutils-devel
Requires: sysfsutils
%endif

%define debug_package %{nil}

%description
EDAC is the current set of drivers in the Linux kernel that handle
detection of ECC errors from memory controllers for most chipsets
on i386 and x86_64 architectures. This userspace component consists
of an init script which makes sure EDAC drivers and DIMM labels
are loaded at system startup, as well as a library and utility
for reporting current error counts from the EDAC sysfs files.


%prep 
%setup

%build
%configure
make %{_smp_mflags} CFLAGS="$RPM_OPT_FLAGS"

%install
rm -rf "$RPM_BUILD_ROOT"
mkdir -p "$RPM_BUILD_ROOT"
DESTDIR="$RPM_BUILD_ROOT" make install

%clean
rm -rf "$RPM_BUILD_ROOT"

%post
if [ $1 = 1 ]; then
  /sbin/chkconfig --add edac
fi

%preun
if [ $1 = 0 ]; then
  /sbin/chkconfig --del edac
fi

%files
%defattr(-,root,root,0755)
%doc README NEWS ChangeLog DISCLAIMER
%{_sbindir}/edac-ctl
%{_bindir}/edac-util
%{_libdir}/*
%{_mandir}/*/*
%{_includedir}/edac.h
%dir %attr(0755,root,root) %{_sysconfdir}/edac
%config(noreplace) %{_sysconfdir}/edac/*
%{_sysconfdir}/init.d/edac
%{_sysconfdir}/udev/rules.d/*


