%define _optdir	/opt
%define _usrdir	/usr
%define _ugdir	%{_usrdir}/ug

Name:       ug-bluetooth-efl
Summary:    UI gadget about the bluetooth
Version:    0.3.2
Release:    1
Group:      TO_BE/FILLED_IN
License:    Flora-1.1
Source0:    %{name}-%{version}.tar.gz
Requires(post): vconf
Requires(post): coreutils
BuildRequires: cmake
BuildRequires: edje-tools
BuildRequires: pkgconfig(edje)
BuildRequires: gettext-tools
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(ui-gadget-1)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(edbus)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(capi-network-bluetooth)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(motion)
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)

%description
UI gadget about the bluetooth

%prep
%setup -q

%build
LDFLAGS="$LDFLAGS -Wl,-z -Wl,nodelete"

export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

%if "%{?profile}" == "mobile"
export CFLAGS="$CFLAGS -DTIZEN_HID"
%endif

export LDFLAGS
cmake . -DCMAKE_INSTALL_PREFIX=%{_ugdir}

make %{?jobs:-j%jobs}

%post
mkdir -p /usr/ug/bin/
ln -sf /usr/bin/ug-client /usr/ug/bin/setting-bluetooth-efl
ln -sf /usr/bin/ug-client /usr/ug/bin/setting-bluetooth-efl-single

%install
rm -rf %{buildroot}
%make_install
install -D -m 0644 LICENSE %{buildroot}%{_datadir}/license/ug-bluetooth-efl

%files
%manifest ug-bluetooth-efl.manifest
%defattr(-,root,root,-)
%{_ugdir}/res/help/*
%{_ugdir}/res/edje/ug-setting-bluetooth-efl/*.edj
%{_ugdir}/res/locale/*/LC_MESSAGES/*
%{_ugdir}/res/images/ug-setting-bluetooth-efl/*
%{_ugdir}/lib/libug-setting-bluetooth-efl.so.0.1.0
%{_ugdir}/lib/libug-setting-bluetooth-efl.so
%ifarch i586
%exclude %{_usrdir}/share/packages/ug-bluetooth-efl.xml
%else
%{_usrdir}/share/packages/ug-bluetooth-efl.xml
%endif
%{_usrdir}/share/icons/default/small/ug-bluetooth-efl.png
%{_datadir}/license/ug-bluetooth-efl
%{_ugdir}/res/tables/ug-setting-bluetooth-efl/ug-bluetooth-efl_ChangeableColorTable.xml
%{_ugdir}/res/tables/ug-setting-bluetooth-efl/ug-bluetooth-efl_FontInfoTable.xml

