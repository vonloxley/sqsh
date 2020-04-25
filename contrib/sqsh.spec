%define ver 3.0
%define rel 1
Summary: An SQL shell
Name: sqsh
Version: %{ver}
Release: %{rel}
Group: Applications/database
Source: https://github.com/vonloxley/sqsh/archive/master.zip
Url: https://github.com/vonloxley/sqsh.git
BuildRoot: %{_tmppath}/%{name}-buildroot 
Vendor: www.sqsh.org
License: GPL
AutoReq: no

%description 
A command-line environment for SQL on Microsoft SQL Server/Sybase databases

%prep
%setup -n sqsh-master

%build
./configure --with-readline --prefix=/usr
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
* Fri Jul 10 2015 mwesdorp
- (eb5dafd) sqsh-3.0 new features and bugfixes (HEAD, origin/master, origin/HEAD, master)

* Wed Aug 6 2014 mwesdorp
- (0684cc7) sqsh-3.0 new features and bugfixes

* Tue Aug 5 2014 mwesdorp
- (850a663) sqsh-3.0 new features and bugfixes

* Mon Apr 14 2014 mwesdorp
- (2ee5d9b) sqsh-2.5.16.1 bugfixes and improvements

* Fri Apr 4 2014 mwesdorp
- (406fc8b) Make sqsh-2.5 OCS-16_0 aware.

* Mon Mar 17 2014 mwesdorp
- (20ac165) sqsh-2.5 new features and bugfixes

* Mon Mar 17 2014 mwesdorp
- (5d5d473) sqsh-2.5 new features and bugfixes

* Mon Mar 17 2014 mwesdorp
- (ae35571) sqsh-2.5 new features and bugfixes

* Sun Mar 16 2014 mwesdorp
- (b3660cc) sqsh-2.5 new features and bugfixes

* Fri Mar 14 2014 mwesdorp
- (bdec3b8) sqsh-2.5 new features and bugfixes

* Wed Mar 12 2014 mwesdorp
- (13fa983) sqsh-2.5 new features, bugfixes and documentation

* Wed Mar 12 2014 mwesdorp
- (58079eb) sqsh-2.5 Documentation

* Tue Mar 11 2014 mwesdorp
- (a342e8e) sqsh-2.5 new features and bugfixes

* Sun Mar 9 2014 mwesdorp
- (f6fc6f3) sqsh-2.5 new features and bugfixes

* Sun Mar 9 2014 mwesdorp
- (5813e46) sqsh-2.5 news features and bugfixes

* Mon Mar 3 2014 mwesdorp
- (1fa8cc5) sqsh-2.5 new features and bugfixes

* Mon Mar 3 2014 mwesdorp
- (2338953) sqsh-2.5 new features and bugfixes

* Sun Mar 2 2014 mwesdorp
- (476927a) sqsh-2.5 new features and bugfixes

* Sat Mar 1 2014 mwesdorp
- (48dc402) sqsh-2.5 new features and bugfixes

* Wed Feb 12 2014 mwesdorp
- (84f948f) sqsh-2.5 Cleanup unnecessary files in CVS tree

* Wed Feb 12 2014 mwesdorp
- (9f11531) sqsh-2.5 Generated files can be savely removed from CVS tree

* Wed Feb 12 2014 mwesdorp
- (4c97c37) sqsh-2.5 Generated files not needed in CVS tree

* Wed Feb 12 2014 mwesdorp
- (bf182b8) sqsh-2.5 New features and bugfixes

* Wed Feb 12 2014 mwesdorp
- (b52efb3) sqsh-2.5 New features and bugfixes

* Wed Feb 12 2014 mwesdorp
- (e2ecb96) sqsh-2.5 New features and bugfixes

* Wed Feb 12 2014 mwesdorp
- (3484f8b) sqsh-2.5 Added Cygwin64 support

* Mon Feb 10 2014 mwesdorp
- (64778a0) sqsh-2.5 new features and bugfixes

* Mon Feb 10 2014 mwesdorp
- (6182857) sqsh-2.5 new features and bugfixes

* Mon Feb 3 2014 mwesdorp
- (726582d) sqsh-2.5 New features (\run, \lcd, \pwd, \ls commands)

* Mon Feb 3 2014 mwesdorp
- (6ba0661) sqsh-2.5 New feature \run command

* Tue Jan 21 2014 mwesdorp
- (649400b) sqsh-2.5 new features and bugfixes

* Tue Jan 21 2014 mwesdorp
- (f97f5e2) sqsh-2.5 new features and bugfixes

* Sun Jan 19 2014 mwesdorp
- (1b7ff09) sqsh-2.5 new features and bugfixes

* Sat Jan 18 2014 mwesdorp
- (0778893) sqsh-2.5 new features and bugfixes

* Tue Dec 24 2013 mwesdorp
- (69b5daa) sqsh-2.5 new features and bugfixes

* Thu Dec 19 2013 mwesdorp
- (8fef50a) sqsh-2.5 new features and bugfixes

* Thu Dec 12 2013 mwesdorp
- (a2cc34f) sqsh-2.5 new features and bugfixes

* Sun Dec 8 2013 mwesdorp
- (6a68207) sqsh-2.5 new features and bugfixes

* Tue Dec 3 2013 mwesdorp
- (6ff4be2) Do not need generated files in source directory

* Tue Dec 3 2013 mwesdorp
- (de44515) Do not need generated files in source directory

* Tue Dec 3 2013 mwesdorp
- (7a23857) Do not need sqsh executable in sources

* Tue Dec 3 2013 mwesdorp
- (c777e9d) sqsh-2.5 new features and bugfixes

* Tue Aug 27 2013 mwesdorp
- (11ca311) sqsh-2.4 Installation scripts changes

* Tue Aug 27 2013 mwesdorp
- (3eca8eb) sqsh-2.4 New .sqshrc file example

* Tue Aug 27 2013 mwesdorp
- (d35d617) sqsh-2.4 Last minute documentation changes

* Tue Aug 27 2013 mwesdorp
- (76b2bfd) sqsh-2.4 Cleanup doc directory

* Tue Aug 27 2013 mwesdorp
- (13a5e7a) sqsh-2.4 Move some really old files to sqsh/doc/old/

* Tue Aug 27 2013 mwesdorp
- (926afd9) sqsh-2.4 Cleanup doc directory

* Mon Aug 26 2013 mwesdorp
- (83e5870) sqsh-2.4 Configuration and script file changes

* Mon Aug 26 2013 mwesdorp
- (854327e) sqsh-2.4 Cygwin documentation changes

* Mon Aug 26 2013 mwesdorp
- (d8eeea5) sqsh-2.4 Adapted makefile for cygwin

* Mon Aug 26 2013 mwesdorp
- (8946262) Remove obsolete file

* Mon Aug 26 2013 mwesdorp
- (ca6bc87) sqsh-2.4 Documentation

* Mon Aug 26 2013 mwesdorp
- (28a2201) sqsh-2.4 New features and bugfixes

* Thu Aug 22 2013 mwesdorp
- (110a759) sqsh-2.4 New features and bugfixes

* Wed Aug 21 2013 mwesdorp
- (834518f) sqsh-2.4 new features and bugfixes

* Wed Aug 21 2013 mwesdorp
- (4eb46cc) sqsh-2.4 new features and bugfixes

* Wed Aug 21 2013 mwesdorp
- (902ca8a) sqsh-2.4 New features and bugfixes

* Wed Aug 21 2013 mwesdorp
- (ad81eed) sqsh-2.4 new features and bugfixes

* Wed Aug 21 2013 mwesdorp
- (2551a42) sqsh-2.4 New features and bugfixes

* Wed Jul 24 2013 mwesdorp
- (7118d1b) sqsh-2.3 Documentation changes

* Tue Jul 23 2013 mwesdorp
- (6277fd8) sqsh-2.3 Additional checks for existence of locale.h before activating localeconv code.

* Tue Jul 23 2013 mwesdorp
- (87451c6) sqsh-2.3 Documentation

* Mon Jul 22 2013 mwesdorp
- (0c0cc2d) sqsh-2.3 Bugfixes and new features

* Sun Jul 21 2013 mwesdorp
- (1c372c7) sqsh-2.3 new features and bugfixes

* Sat Jul 20 2013 mwesdorp
- (3d352c2) sqsh-2.3 new features and bugfixes

* Tue May 7 2013 mwesdorp
- (3819ffa) sqsh-2.2.0 quality control last minute fixes

* Sun May 5 2013 mwesdorp
- (e46535b) sqsh-2.2.0 last minute modifications.

* Fri May 3 2013 mwesdorp
- (b76442b) sqsh-2.2.0 - sqsh_readline.c renamed function re_match to regex_match

* Fri May 3 2013 mwesdorp
- (49163a3) sqsh-2.2.0 new features and bugfixes. Final package.

* Fri May 3 2013 mwesdorp
- (e826ed1) sqsh-2.2.0 new features and bugfixes. New documentation files.

* Mon Apr 29 2013 mwesdorp
- (c9060b8) sqsh-2.2.0 new features and bugfixes

* Fri Apr 26 2013 mwesdorp
- (ef6560b) sqsh-2.2.0 new features and bugfixes

* Thu Apr 25 2013 mwesdorp
- (9b42fb7) sqsh-2.2.0 new features and bugfixes

* Thu Apr 18 2013 mwesdorp
- (6d5178d) sqsh-2.2.0 new features and bugfixes

* Thu Apr 4 2013 mwesdorp
- (901c39d) sqsh-2.2.0 new features, bugfixes and code cleanup

* Mon Feb 25 2013 mwesdorp
- (3c5fa7c) sqsh-2.1.9 doc changes

* Mon Feb 25 2013 mwesdorp
- (65322e8) sqsh-2.1.9 Cygwin changes

* Mon Feb 25 2013 mwesdorp
- (50f8921) sqsh-2.1.9 doc changes

* Mon Feb 25 2013 mwesdorp
- (f83789c) sqsh-2.1.9 bugfixes and new features

* Sun Feb 24 2013 mwesdorp
- (316d503) sqsh-2.1.9 bugfixes and new features

* Sun Feb 24 2013 mwesdorp
- (500333e) sqsh-2.1.9 bugfixes and new features

* Wed Feb 20 2013 mwesdorp
- (67bf45e) sqsh-2.1.9 bugfixes and new features

* Wed Feb 20 2013 mwesdorp
- (6c2cb89) sqsh-2.1.9 bugfixes and new features

* Tue Feb 19 2013 mwesdorp
- (abd0ee2) sqsh-2.1.9 bugfixes and new features

* Tue Jan 8 2013 mwesdorp
- (40c4e02) Fix unstifle_history declaration from extern void to extern int

* Wed May 2 2012 mwesdorp
- (4c257f4) Prepare for new release sqsh-2.1.8 These are the libraries to build against Sybase OpenClient 15.7 for Cygwin

* Wed May 2 2012 mwesdorp
- (99f7511) Prepare for new release sqsh-2.1.8

* Wed May 2 2012 mwesdorp
- (ea6b106) Prepare for new release sqsh-2.1.8

* Wed May 2 2012 mwesdorp
- (7cde2db) Prepare for new release sqsh-2.1.8

* Tue May 1 2012 mwesdorp
- (7facdd6) Improve configure and Makefile generation scripts

* Sun Apr 29 2012 mwesdorp
- (dea20be) New features and bugfixes for sqsh-2.1.8

* Thu Mar 29 2012 mwesdorp
- (9fcabee) Allow wordlist and dynamic completion simultaneously (Patch 3510445) and some additional changes to sqsh-2.1.8 dynamic keyword loading

* Thu Mar 15 2012 mwesdorp
- (19ff8a9) Document changes for sqsh-2.1.8

* Wed Mar 14 2012 mwesdorp
- (f0f2caa) New doc files

* Wed Mar 14 2012 mwesdorp
- (27ce195) New features and bugfixes for sqsh-2.1.8

* Fri Aug 12 2011 mwesdorp
- (e6b8a6a) Fix for bug report 3388213: Option to read password from stdin stuck in 2.1.7 ? Function hide_password now correctly handles option -P -

* Sat Oct 23 2010 mwesdorp
- (b2835a3) Use Sybase and Freetds CT-LIB functionality to display binary datatypes instead of handling these datatypes natively. This fixes bug report 3079678. Also display binary datatypes left aligned like in isql/osql.

* Mon May 3 2010 mpeppler
- (0be32eb) Bump version number

* Mon May 3 2010 mpeppler
- (b3accdf) ASE 15.5 logic/datatypes.

* Sun Mar 28 2010 mpeppler
- (9c43bc1) Set the correct CS_VERSION flag for OCS 15.5, to handle bigdatetime data.

* Thu Feb 25 2010 mwesdorp
- (f9d3c10) sqsh-2.1.7 new features and bugfixes

* Wed Feb 17 2010 mwesdorp
- (ff912a1) sqsh-2.1.7 new features and bugfixes

* Fri Feb 12 2010 mwesdorp
- (8b5700f) sqsh-2.1.7 new features and bugfixes

* Fri Feb 12 2010 mwesdorp
- (0c64631) sqsh-2.1.7 new features and bugfixes

* Mon Feb 8 2010 mwesdorp
- (63c621f) sqsh-2.1.7 new features and bugfixes

* Mon Feb 8 2010 mwesdorp
- (ecdd1f5) sqsh-2.1.7 new features and bugfixes

* Thu Feb 4 2010 mwesdorp
- (b2816e8) Add sqsh-2.1.7 new features and bugfix documentation

* Thu Feb 4 2010 mwesdorp
- (9d1e62f) Add a more advanced sqshrc file as an example of what sqsh can do

* Thu Feb 4 2010 mwesdorp
- (4bf4a50) sqsh-2.1.7 features and bugfixes

* Mon Feb 1 2010 mwesdorp
- (e6c9236) sqsh-2.1.7 features and bugfixes

* Thu Jan 28 2010 mwesdorp
- (661f744) sqsh-2.1.7 features and bugfixes

* Tue Jan 26 2010 mwesdorp
- (8f85b94) sqsh-2.1.7 features and bugfixes

* Tue Jan 12 2010 mwesdorp
- (04954c1) Modifications for sqsh-2.1.7 features and bugfixes

* Thu Jan 7 2010 mwesdorp
- (400d256) Applied patch 1290313 provided by John Griffin to improve X window resizing behavior.

* Wed Jan 6 2010 mwesdorp
- (737c3bd) SQSH_HIDEPWD functionality improved. (Original patch code from Yichen Xie and David Wood, ID 2607434)

* Wed Dec 23 2009 mwesdorp
- (2c8d2b8) Properly expand the $session variable.

* Wed Dec 23 2009 mwesdorp
- (483e220) Add optional ignore SIGQUIT (Ctrl-\) signal handler between #if 0 / #endif compiler directives.

* Wed Dec 23 2009 mwesdorp
- (c4fa346) Redirect stderr to -o output file as well as stdout.

* Wed Dec 23 2009 mwesdorp
- (80725c9) Fix for bug report 2920048, using calloc instead of malloc.

* Wed Dec 23 2009 mwesdorp
- (2f4530f) Applied fix from patch 2061950 by Klaus-Martin Hansche.

* Wed Dec 23 2009 mwesdorp
- (8fbce68) Applied fix for bugreport 1959260 supplied by Stephen Doherty.

* Thu Oct 1 2009 mwesdorp
- (f199ff6) Fix ignoreeof and histunique initializers (set to "0" instead of "Off")

* Sun Aug 9 2009 mpeppler
- (8cca1c9) Updated version.

* Sun Aug 9 2009 mpeppler
- (1afdda0) Man/htlp build script.

* Sun Aug 9 2009 mpeppler
- (21e6ed9) Updated versions, generated from pod file.

* Sun Aug 9 2009 mpeppler
- (7ffd532) Add documentation for Martin's additions.

* Sun Aug 9 2009 mpeppler
- (f185155) Updated to include Martin Wesdorp.

* Sun Jul 5 2009 mpeppler
- (97b11ed) First version of converted man page to POD format.

* Sun Apr 19 2009 mwesdorp
- (47c8aec) Fixed problem building sqsh on RH 7.3 (reported by Jeff Woods)

* Tue Apr 14 2009 mwesdorp
- (1666f55) Changed version to 2.1.6 and increased password length from 12 to 30

* Tue Apr 14 2009 mwesdorp
- (6b5e4b1) Implemented login and query timeout parameters (T, Q), network authentication parameters (K,V,R,Z) and proper keyword_file variable expansion

* Tue Apr 14 2009 mwesdorp
- (b7eab77) Implemented login and query timeout, modified network authentication (kerberos)

* Tue Apr 14 2009 mwesdorp
- (ed122c2) Implemented feature hist_unique

* Tue Apr 14 2009 mwesdorp
- (70bbb6c) Proper tmp_dir variable expansion

* Tue Apr 14 2009 mwesdorp
- (e890ab5) Implemented color prompt support, ignoreeof setting and file variable expansion (readline_history)

* Tue Apr 14 2009 mwesdorp
- (f2f06cd) Defined new variables

* Tue Apr 14 2009 mwesdorp
- (2866d69) Expand interfaces variable

* Tue Apr 14 2009 mwesdorp
- (5e6bfeb) Implemented color prompt support

* Tue Apr 14 2009 mwesdorp
- (d4a0f97) Implemented color prompt support

* Tue Apr 14 2009 mwesdorp
- (4f065b3) Implemented color prompt support

* Tue Apr 14 2009 mwesdorp
- (3034f0a) expand prompt buffer from 32 to 64 bytes

* Tue Apr 14 2009 mwesdorp
- (9d7df4b) Implemented \buf-del command

* Tue Apr 14 2009 mwesdorp
- (b0d83f1) Implemented \buf-del command

* Tue Apr 14 2009 mwesdorp
- (b7ce769) Implemented \buf-del command

* Sat Apr 11 2009 mwesdorp
- (3b18ed2) Duplicate reset removed.

* Sat Aug 30 2008 mpeppler
- (86fb77c) Intermediate check-in

* Wed May 21 2008 mpeppler
- (a6e110d) Add support for getting the server principal via an external program, add Niksa's patches.

* Tue Apr 8 2008 mpeppler
- (f345468) Fixes for kerberos handling

* Sun Apr 6 2008 mpeppler
- (1d49533) Version number updates

* Sun Apr 6 2008 mpeppler
- (5a08e81) Fix ReleaseNotes

* Sun Apr 6 2008 mpeppler
- (40f3405) Fix man page

* Sun Apr 6 2008 mpeppler
- (cafce38) Various minor patches, plus support for Kerberos auth

* Sun Apr 16 2006 mpeppler
- (c0dfacd) check in for 2.1.4

* Fri Dec 30 2005 mpeppler
- (0599c50) Add CS_SERVERADDR property handling

* Fri Dec 30 2005 mpeppler
- (f07313d) Hide -P command line option

* Sat Nov 19 2005 mpeppler
- (b0d293f) patch to handle CS_ROW_FAIL

* Sun Jul 24 2005 mpeppler
- (ad134ae) Added CSV output format - thanks to Thomas Depke

* Sat Apr 9 2005 mpeppler
- (3f187f7) Various minor fixes

* Tue Apr 5 2005 mpeppler
- (2ee65f3) Various minor fixes

* Thu Dec 2 2004 mpeppler
- (e23e289) 2.1.3 version number update

* Wed Nov 24 2004 mpeppler
- (fa1b789) Fix CS_VERSION problem

* Wed Nov 24 2004 mpeppler
- (1d26c7d) cygwin stuff

* Wed Nov 24 2004 mpeppler
- (5fa4a8d) Script to generate .a files for Cygwin

* Wed Nov 24 2004 mpeppler
- (8d618f1) Cygwin build files

* Tue Nov 23 2004 mpeppler
- (392ad5a) Fix incorrect #if def logic

* Mon Nov 22 2004 mpeppler
- (616eeb6) Various minor fixes

* Mon Nov 8 2004 mpeppler
- (280062a) Better library detection to support OCS 15.

* Mon Nov 8 2004 mpeppler
- (0c879b7) Added release notes

* Mon Nov 8 2004 mpeppler
- (fff414b) doc fixes

* Fri Nov 5 2004 mpeppler
- (d6ef5b9) Various fixes - see changelog

* Fri Nov 5 2004 mpeppler
- (30fb5d8) Fix large column support

* Thu Nov 4 2004 mpeppler
- (03d9532) Fix segv for go in comment

* Thu Nov 4 2004 mpeppler
- (0c7d0f1) Add -n command line option to set CHAINED mode on connect

* Thu Nov 4 2004 mpeppler
- (1f95864) open /dev/tty in cmd_read.c if necessary

* Thu Nov 4 2004 mpeppler
- (7c5dd74) Revert part of cmd_read to 1.7 version

* Mon Apr 12 2004 mpeppler
- (4c72d30) Update version number

* Mon Apr 12 2004 mpeppler
- (25f65d6) Fix configure issues, and fix cmd_bcp.c to work when the CS_VERSION_xxx is not set to CS_VERSION_100

* Sun Apr 11 2004 mpeppler
- (b096562) Fix version in sqsh.1

* Sun Apr 11 2004 mpeppler
- (897184d) More fixes from Scott's archive

* Sat Apr 10 2004 mpeppler
- (1c5cdfe) Add support for 12.5 wide varchar, and for 12.5.1 date/time datatypes

* Wed Apr 7 2004 chunkm0nkey
- (6b283d0) Initial import

* Wed Apr 7 2004 chunkm0nkey
- (d5b75c9) *** empty log message ***

