\
%global debug_package %{nil}

Name:       equalff
Version:    2.0.0
Release:    1%{?dist}
Summary:    High-speed duplicate file finder

License:    GPLv3+
URL:        https://github.com/jhkst/equalff
Source0:    https://github.com/jhkst/equalff/archive/refs/tags/v%{version}.tar.gz#/equalff-%{version}.tar.gz

BuildRequires: gcc
BuildRequires: make
BuildRequires: sed # For potential SONAME fixup if needed

%description
Equalff is a high-speed duplicate file finder. It employs a unique
algorithm, making it faster than other duplicate file finders for a
single run. The project is structured as a core dynamic library
(libequalff.so) providing the duplicate detection logic, and a
command-line utility (equalff) that utilizes this library.

%package -n libequalff0
Summary:    Core library for equalff duplicate file finder
# The '0' suffix is a convention if your library has a SONAME like libequalff.so.0
# If it's just libequalff.so without a SONAME, you might name this subpackage 'libequalff'
# and adjust file paths below.

%description -n libequalff0
This package contains the shared library providing the core duplicate
detection logic for the equalff utility.

%package -n libequalff-devel
Summary:    Development files for libequalff
Requires:   libequalff0%{?_isa} = %{version}-%{release}

%description -n libequalff-devel
This package contains the headers and development files needed to build
software against libequalff.

%prep
%autosetup -p1
# The -p1 assumes your tarball extracts to a directory like equalff-2.0.0/
# If it extracts to just 'equalff', you might need %autosetup -n equalff-%{version} -p1
# or just %setup -q if the tarball directly contains the files without a versioned top-level dir.

%build
%make_build PREFIX=%{_prefix} LIBDIR=%{_libdir} # This calls 'make' in your Makefile

%install
rm -rf %{buildroot}
%make_install DESTDIR=%{buildroot} PREFIX=%{_prefix} LIBDIR=%{_libdir} # This calls 'make install'

# Ensure the license file is installed
install -Dpm 644 LICENSE %{buildroot}%{_licensedir}/%{name}/LICENSE

%files
%{_bindir}/equalff
%{_mandir}/man1/equalff.1*

%files -n libequalff0
%{_libdir}/libequalff.so
# If your library has a SONAME, e.g., libequalff.so.0, list that:
# %{_libdir}/libequalff.so.0
# And potentially a symlink if your Makefile creates one:
# %{_libdir}/libequalff.so

%files -n libequalff-devel
%{_includedir}/equalff/
%{_libdir}/libequalff.so
# For devel package, we point to the .so link if there's a SONAME,
# or just the .so if there isn't. The linker uses this.

%changelog
* Mon Jan 01 2024 jhkst <jhkst@centrum.cz> - 2.0.0-1
- Initial RPM packaging 