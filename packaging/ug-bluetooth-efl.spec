%define _optdir	/opt
%define _usrdir	/usr
%define _ugdir	%{_optdir}/ug


Name:       ug-bluetooth-efl
Summary:    UI gadget about the bluetooth
Version: 0.2.114
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO BE FILLED IN
Source0:    %{name}-%{version}.tar.gz
Requires(post): /usr/bin/vconftool
BuildRequires: cmake
BuildRequires: edje-tools
BuildRequires: gettext-tools
BuildRequires: pkgconfig(elementary)
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
BuildRequires: pkgconfig(dbus-glib-1)

%description
UI gadget about the bluetooth

%prep
%setup -q

%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_ugdir}

make %{?jobs:-j%jobs}

%post
vconftool set -t int file/private/libug-setting-bluetooth-efl/visibility_time "0" -g 6520
mkdir -p /opt/ug/bin/
ln -sf /usr/bin/ug-client /opt/ug/bin/setting-bluetooth-efl

%install
rm -rf %{buildroot}
%make_install

%files
%defattr(-,root,root,-)
%{_ugdir}/res/locale/*/LC_MESSAGES/*
%{_ugdir}/res/images/ug-setting-bluetooth-efl/*
%{_ugdir}/lib/libug-setting-bluetooth-efl.so.0.1.0
%{_ugdir}/lib/libug-setting-bluetooth-efl.so
%{_usrdir}/share/packages/ug-bluetooth-efl.xml

