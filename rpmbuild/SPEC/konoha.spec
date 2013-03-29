Summary: konoha scripting language
Group: Development/Languages
Name: konoha
Prefix: /usr
Provides: konoha
Release: 1.el6
Source0: %{name}-%{version}.tar.gz
URL: http://www.konohascript.org/
Version: 3.0
License: BSD-2-Clause license
BuildRequires: cmake >= 2.6, libcurl-devel, gmp-devel, mpich2-devel, openssl-devel, libxml2-devel, httpd-devel, libevent-devel, mecab-devel, llvm-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
Konoha is a scripting language, characterized by:
* simple
* static
* syntax extensible
The language is based on C-like grammar, not new for everybody.

%prep
%setup -q

%build
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DUSE_APACHE_PLATFORM=ON -DAPXS_BIN=ON
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
cd build
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,0755)
%doc
%{_bindir}/konoha
%{_includedir}/konoha3/*
/usr/lib/libkonoha.so
/usr/lib/konoha/*
/usr/lib64/httpd/modules/mod_konoha.so

%changelog
* Wed Mar 11 2013 Tadaki SAKAI <stadaki.dev@gmail.com>
- Initial build.
