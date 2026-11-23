Name:           harbour-bureau
Summary:        Bureau - Android-style home screen
Version:        0.1.0
Release:        2
Group:          Qt/Qt
License:        GPLv3
URL:            https://openrepos.net/
Source0:        %{name}-%{version}.tar.bz2
Requires:       sailfishsilica-qt5 >= 0.10.9
Requires:       /usr/bin/lca-tool
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  desktop-file-utils

%description
Bureau is an Android-style home screen for Sailfish OS: place your
Sailfish and Android apps wherever you want on a grid, group them in
folders, and show it as a page of the Sailfish home screen.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 VERSION=%{version}
%make_build

%install
%qmake5_install

desktop-file-install --delete-original \
  --dir %{buildroot}%{_datadir}/applications \
  %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%license LICENSE
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
