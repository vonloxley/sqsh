%define ver 2.1
%define rel 1
Summary: An SQL shell
Name: sqsh
Version: %{ver}
Release: %{rel}
Copyright: distributable
Group: Applications/database
Source: http://www.sqsh.org/sqsh-2.1-src.tar.gz
Url: http://www.sqsh.org/
Requires: freetds
BuildRoot: %{_tmppath}/%{name}-buildroot 
Vendor: www.sqsh.org

%description 
A command-line environment for SQL on Microsoft SQL Server/Sybase databases

%prep
%setup -n sqsh-%{version}

%build
SYBASE=/usr ./configure --with-readline --prefix=/usr
make

%clean 
rm -rf $RPM_BUILD_ROOT

%install
rm -rf $RPM_BUILD_ROOT
INSTALL=./autoconf/install-sh
INSTALL_MAN=./autoconf/install-man
$INSTALL_MAN -s ./doc -d $RPM_BUILD_ROOT/usr/man sqsh.1
$INSTALL -d $RPM_BUILD_ROOT/usr/bin
$INSTALL -c ./src/sqsh $RPM_BUILD_ROOT/usr/bin/sqsh
$INSTALL -c -m 644 doc/global.sqshrc $RPM_BUILD_ROOT/usr/etc/sqshrc

%files 
%defattr(-,root,root)
%config /usr/etc/sqshrc
%doc /usr/man/man1/sqsh.1.gz
/usr/bin/sqsh

%changelog 
* Fri Sep  5 2003 Ian Grant <Ian.Grant@cl.cam.ac.uk>
- Initial

