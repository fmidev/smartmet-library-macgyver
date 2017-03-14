%define DIRNAME macgyver
%define LIBNAME smartmet-%{DIRNAME}
%define SPECNAME smartmet-library-%{DIRNAME}
Summary: macgyver library
Name: %{SPECNAME}
Version: 17.3.14
Release: 1%{?dist}.fmi
License: MIT
Group: Development/Libraries
URL: https://github.com/fmidev/smartmet-library-macgyver
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: boost-devel
BuildRequires: imake
BuildRequires: smartmet-timezones >= 13.10.22
BuildRequires: ctpp2-devel
BuildRequires: libicu-devel
BuildRequires: fmt-devel
Requires: fmt
Requires: ctpp2
Requires: libicu >= 50.1
Provides: %{SPECNAME}
Obsoletes: libsmartmet_macgyver < 16.12.20
Obsoletes: libsmartmet_macgyver-debuginfo < 16.12.20
Obsoletes: libsmartmet-macgyver < 16.12.20
Obsoletes: libsmartmet-macgyver-debuginfo < 16.12.20

%description
FMI MacGyver library

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{SPECNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(0775,root,root,0775)
%{_libdir}/libsmartmet-%{DIRNAME}.so

%package -n %{SPECNAME}-devel
Summary: FMI MacGyver library development files
Provides: %{SPECNAME}-devel
Obsoletes: libsmartmet_macgyver-devel < 16.12.20

%description -n %{SPECNAME}-devel
FMI MacGyver library development files

%files -n %{SPECNAME}-devel
%defattr(0664,root,root,0775)
%{_includedir}/smartmet/%{DIRNAME}

%changelog
* Tue Mar 14 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.3.14-1.fmi
- Fixed to compile on Windows

* Wed Feb 22 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.22-1.fmi
- Added NearTreeLatLon::SurfaceLength

* Sat Feb 18 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.18-1.fmi
- NearTreeLatLon is now templated based on the type of the ID

* Fri Feb 17 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.17-1.fmi
- Added a coordinate object for efficient nearest neighbour searches with latlon coordinates

* Wed Feb 15 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.15-1.fmi
- Fixed obsoletes-fields

* Wed Jan 18 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.18-1.fmi
- Upgraded from cppformat to fmt

* Tue Dec 20 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.12.20-1.fmi
- Switched to open source naming conventions

* Fri Sep 30 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.9.30-1.fmi
- ThreadPool now catches all exceptions to prevent program termination

* Fri May  6 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.5.6-1.fmi
- Avoid using string streams when formatting times for HTTP response headers

* Wed May  4 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.5.4-1.fmi
- Added TimeZones class
- Marked TimeZoneFactory singleton to be deprecated

* Mon Apr 18 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.4.18-1.fmi
- Updated to cppformat 2.0

* Fri Apr  8 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.4.8-1.fmi
- Fixed a memory leak in NearTree::clear()

* Sat Jan 23 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.23-1.fmi
- API breaking changes:
- Use std::unique_ptr in TimeZoneFactory and DirectoryMonitor to avoid shared_ptr locks
- Removed Singleton as obsolete with C++11 thread safety guarantees

* Tue Jan 19 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.19-1.fmi
- Disallow NaN and infinities in string to number conversions

* Tue Nov 10 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.11.10-1.fmi
- Replaced remaining lexical_cast calls with to_string

* Mon Nov  9 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.11.9-1.fmi
- RHEL6 - compatible implementation of ascii case conversions

* Thu Nov  5 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.11.5-1.fmi
- Added copying versions of tolower and toupper

* Wed Nov  4 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.11.4-1.fmi
- Added optimized tolower and toupper - functions for ASCII input

* Mon Sep 07 2015 Anssi Reponen <anssi.reponen@fmi.fi> - 15.9.7-1.fmi
- Validity check of input data in Stat::trend-function corrected.

* Mon Aug 24 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.24-1.fmi
- Fixed to_string for unsigned longs to use format %lu instead of %ul

* Fri Aug 21 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.8.21-1.fmi
- ISO-time parser now parses (but ignores) fractional seconds

* Wed Aug 19 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.19-1.fmi
- Optimized to_iso_string, to_iso_extended_string and TimeStampFormatter for speed

* Tue Aug 18 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.18-1.fmi
- Added formatters for boost time classes to avoid global locks from sstreams

* Tue Aug 11 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.11-2.fmi
- Fixed double parser to use a double variable instead of a float

* Tue Aug 11 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.11-1.fmi
- Added string <--> number conversion tools

* Mon Jul  6 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.7.6-1.fmi
- Timezone parsing now follows ISO spec.

* Tue May 19 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.5.19-1.fmi
- Removed TernarySearchTrie as obsolete
- TernarySearchTree now forces storage to use shared pointers
- TernarySearchTree allows non-shared pointers to be inserted

* Fri May 15 2015 Santeri Oksman <santeri.oksman@fmi.fi> - 15.5.15-1.fmi
- Check that filenames do not start with dot in DirectoryMonitor

* Fri Apr 10 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.10-1.fmi
- Timeparser now allows 0h, 0m etc as offsets

* Thu Feb 12 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.2.12-1.fmi
- Fixed bug in file cache reload size tracking

* Wed Feb 11 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.2.11-1.fmi
- Fixed bug in FileCache insertion order

* Fri Feb  6 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.2.6-1.fmi
- Cache now supports reporting of evicted cache items
- Added FileCache class for hard drive backed caching of serializable objects

* Tue Sep 23 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.9.23-1.fmi
- Epoch time can now be 11 digits long
- FMI-timestamp can now include seconds

* Fri Sep  5 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.9.5-1.fmi
- Fixed NearTree not to return any points if the distance limit is not satisfied

* Tue Aug  5 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.8.5-1.fmi
- Fixed memory leak in Cache when using but not expiring any tags

* Mon Jun  2 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.6.2-1.fmi
- Corrected dependencies

* Wed May 14 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.14-1.fmi
- Switched to a shared library

* Sun May 11 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.5.11-1.fmi
- Fixed possible bug in offset parser error reporting

* Thu May  8 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.5.8-1.fmi
- Disambiguated time parsers

* Tue May  6 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.7-1.fmi
- Hotfix to time parser: must not mistake epoch time 1400011697 for a datetime

* Thu Apr 10 2014 Anssi Reponen <anssi.reponen@fmi.fi> - 14.4.10-1.fmi
- Handling of missing value corrected. Name of struct changed. Parameter order of functions changed.

* Thu Feb 27 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.2.27-1.fmi
- Generic parse-function now uses the new parsers

* Wed Feb 26 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.2.26-1.fmi
- Time parsing using Boost Spirit

* Tue Jan 14 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.1.14-1.fmi
- sum_rr function renamed to integ. Implementation of integ-function changed so that actual weights (duration of value) are always used in calculation. Sum function implementation changed so that weight is always 1.0 for all values.

* Mon Jan 13 2014 Santeri Oksman <santeri.oksman@fmi.fi> - 14.1.13-1.fmi
- Supported time range widened to the year when the Gregorian calendar is introduced.

* Tue Dec 17 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.12.17-2.fmi
- Anssi Reponen: Optimized speed of min/max calculations in Stat

* Tue Dec 17 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.12.17-1.fmi
- Anssi Reponen: Statistical functions added (Stat.cpp,Stat.h,StatTest.cpp).

* Mon Dec  2 2013 Tuomo Lauri <tuomo.lauri@fmi.fi> - 13.12.2-1.fmi
- Added shutdown-method to ThreadPool

* Fri Oct 25 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.10.25-1.fmi
- Added TemplateFormatter and TypeName from brainstorm-spine
- Code in Astronomy namespace refactored. Moon rise/set time calculation added.

* Tue Oct  8 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.10.8-1.fmi
- Fixed sunrise and sunset calculations to work near the international date line

* Fri Oct  4 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.10.4-1.fmi
- Added DirectoryMonitor::ready()

* Mon Sep 16 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.9.16-1.fmi
- Rewrote NearTree API to use boost::optional
- Added LatLonTree for searching nearest points in the FMI latlon sphere
- Added Geometry::Bearing

* Thu Sep  5 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.9.5-1.fmi
- Added TimeParser::parse_duration
- Added support for week units in time offset parsing
- Link to boost libraries with the -mt suffix
- Fixed TimeParser::looks_utc to return true for time offsets

* Tue Aug 13 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.8.13-1.fmi
- Use data files provided by smartmet-timezones by default

* Thu Aug  8 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.8.8-1.fmi
- Disabled cache counts for now since the RHEL6 compiler does not understand variadic macros

* Fri Aug  2 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.8.2-1.fmi
- Cache now counts hits and misses by default

* Mon Jul 22 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.7.22-1.fmi
- Fixed thread safety issues in using timegm

* Wed Jul  3 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.7.3-1.fmi
- TimeParser now understands offsets measured in days (as in "+2d")
- Updated to boost 1.54

* Wed Jun 26 2013 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 13.6.26-1.fmi
- Replaced long with std::time_t in time handling in Cache to silence any warnings

* Tue Jun 25 2013 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 13.6.25-1.fmi
- Added getMaxQueueSize-method to Threadpool

* Sun Jun 23 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.6.23-1.fmi
- Number zero is now considered to mean "now" by the time parser

* Sat Jun 22 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.6.22-1.fmi
- TimeParser can now handle time offsets

* Mon Jun 10 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.6.10-1.fmi
- Visual C++ support

* Mon Mar 25 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.3.25-1.fmi
- Added TimeParser::make_time which handles DST changes gracefully

* Wed Mar 20 2013 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 13.3.20-1.fmi
- ThreadPool modifications:
- Renamed method 'wait' to 'join'
- Default scheduler size is 'unlimited'
- Fixed comments

* Wed Feb 27 2013 Santeri Oksman <santeri.oksman@fmi.fi> - 13.2.27-1.fmi
- New package which contains ObjectPool header which is needed in compiling wfs plugin

* Wed Feb 6  2013 Tuomo Lauri    <tuomo.lauri@fmi.fi>   -  13.2.6-1.fmi
- Updated Helmert transformations

* Fri Jan 11 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.1.11-1.fmi
- Added code for Helmert transformations

* Thu Jan 10 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.1.10-1.fmi
- Improved error detection in TimeParser::looks_iso

* Fri Nov 30 2012 Tuomo Lauri <tuomo.lauri@fmi.fi>	 - 12.11.30-1.fmi
- Directory Monitor can now monitor single files

* Thu Nov 29 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.11.29-1.fmi
- Added dynamic size thread pool class

* Thu Nov 22 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.11.22-1.el6.fmi

* Improved cache size tracking implementation

* Wed Nov 21 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.11.21-1.el6.fmi

* Cache size is now a customisable concept

* Thu Nov 15 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.11.15-1.el6.fmi
- Improved TimeParser functions, parse_iso in particularly now follows the standard more closely

* Wed Nov  7 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.11.7-1.el6.fmi
- Upgrade to boost 1.52

* Thu Oct 11 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.10.11-1.el6.fmi
- Added cache content reporting

* Tue Oct 9 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.10.9-4.el6.fmi
- Added per-value cache hits reporting

* Tue Oct 9 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.10.9-3.el6.fmi
- Added (nonlocking) iteration through the cache for report purposes

* Tue Oct 9 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.10.9-2.el6.fmi
- Added resizing function to cache

* Tue Oct 9 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.10.9-1.el6.fmi
- More understandable names for cache policies

* Tue Oct 2 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.10.2-1.el6.fmi
- Added generic caching v1.0

* Thu Sep 13 2012 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 12.9.13-1.el6.fmi
- Tentative fix for sunrise and sunset time bugs in Astronomy

* Mon Aug  6 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.8.6-1.el6.fmi
- Added Base64 encoder and decoder

* Fri Jul  6 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.7.6-1.el6.fmi
- Fixed WorldTimeZones destructor to use delete[]

* Thu Jul  5 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.7.5-1.el6.fmi
- Migration to boost 1.50

* Tue Jul  3 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.7.3-2.el6.fmi
- Fixed TernarySearchTree not to accept duplicate keys

* Tue Jul  3 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.7.3-1.el6.fmi
- Fixed bug in returning NULL from TernaryTree::findprefix

* Wed Jun 13 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.6.13-1.el6.fmi
- Added TernarySearchTree class

* Mon Apr  2 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.4.2-1.el6.fmi
- Upgraded to boost 1.49
- Single threaded version no longer supported

* Fri Dec 16 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.12.16-1.el5.fmi
- Fixed reading of timezone shapes

* Thu Dec 15 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.12.15-2.el5.fmi
- Added support for comments for lines beginning with "#"

* Thu Dec 15 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.12.15-1.el5.fmi
- Added delimiter (abomination) support to CsvReader

* Fri Nov 11 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.11.11-1.el5.fmi
- Added error checking to WorldTimeZones

* Fri Nov  4 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.11.4-1.el5.fmi
- Added possibility to get list of timezones

* Wed Jul 20 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.7.20-1.el5.fmi
- Upgrade to boost 1.47

* Tue Apr 19 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.4.19-1.el5.fmi
- Made TimeParser::looks visible in the header

* Fri Mar 25 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.25.1-1.el5.fmi
- Fixed DirectoryMonitor to use boost::this_thread::sleep

* Thu Mar 24 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.3.24-2.el5.fmi
- Fixed a race condition in DirectoryMonitor

* Thu Mar 24 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.3.24-1.el5.fmi
- Upgrade to boost 1.46

* Fri Dec 10 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.12.10-1.el5.fmi
- Added UTF16 conversion functions

* Thu Oct 28 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.10.28-1.el5.fmi
- Made API const correct
- Removed shared_ptr from TimeFormatterFactory API as unnecessary

* Tue Sep 14 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.9.14-1.el5.fmi
- Upgrade to boost 1.44

* Thu Aug  5 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.8.5-1.el5.fmi
- Added Fmi::number_cast in new header Cast.h

* Fri Jul 16 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.7.16-1.el5.fmi
- Added Fmi::Geometry namespace based on smartmet/newbase/NFmiGeoTools.h

* Tue Jun  1 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.6.1-1.el5.fmi
- Improved TimeParser::parse_xml to handle fractional seconds

* Tue Apr 13 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.3.13-1.el5.fmi
- Added TimeTools::parse_http for parsing all HTTP-date formats
- Added http as a choice for TimeFormatter::create

* Fri Jan 15 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.1.15-1.el5.fmi
- Upgrade to boost 1.41

* Tue Jul 14 2009 Mika Heiskanen <mika.heiskanen@fmi.fi> - 9.7.14-1.el5.fmi
- Upgrade to boost 1.39 

* Fri Jun 26 2009 Mika Heiskanen <mika.heiskanen@fmi.fi> - 9.6.26-2.el5.fmi
- Added more utf8/latin1 tools

* Fri Jun 26 2009 Mika Heiskanen <mika.heiskanen@fmi.fi> - 9.6.26-1.el5.fmi
- Added CharsetTools, other minor improvements

* Thu Jun 25 2009 Mika Heiskanen <mika.heiskanen@fmi.fi> - 9.6.25-1.el5.fmi
- Added TernarySearchTrie

* Tue Apr 21 2009 Mika Heiskanen <mika.heiskanen@fmi.fi> - 9.4.21-1.el5.fmi
- Build single- and multithreaded versions

* Fri Dec 19 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.12.19-1.el5.fmi
- Added TimeFormatter and TimeParser from BrainStorm project

* Wed Dec 17 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.12.17-1.el5.fmi
- Added CsvReader

* Wed Oct 22 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.10.22-1.el5.fmi
- Timezone database moved to /smartmet/share

* Wed Oct 15 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.10.15-1.el5.fmi
- TimeZoneFactory now recognizes zone UTC

* Mon Sep 22 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.9.22-1.el5.fmi
- Only boost-static is now required

* Thu Sep 11 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.9.11-1.el5.fmi
- Boost 1.36 into use

* Wed Sep 10 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.9.10-1.el5.fmi
- Added singleton support
- Added timezone information
- Added directory monitoring class

* Fri Jul 18 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.7.18-1.el5.fmi
- Added astronomical calculations

* Mon Jun 16 2008 Mika Heiskanen <mika.heiskanen@fmi.fi> - 8.6.16-1.el5.fmi
- Initial build
