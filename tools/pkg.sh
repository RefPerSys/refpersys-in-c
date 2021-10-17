export rv


rps_pkg_install() {
	rps_warn "Installing packages, this may take a while"
	[ "$(id -u)" -ne 0 ] && __su="sudo"

	rps_os_distro

	if [ "$rv" == "Arch" ] ; then
		rps_run_quiet "$__su pacman -S --noconfirm --needed --quiet $1"

	else
		rps_run "$__su apt install -y --no-upgrade $1"
	fi

	rps_check $? "Package(s) $1 installed"
}


rps_pkg_update() {
	rps_warn "Updating packages, this may take a while"
	[ "$(id -u)" -ne 0 ] && __su="sudo"
	
	rps_os_distro

	if [ "$rv" == "Arch" ] ; then
		rps_run "$__su pacman -Sy"
		rps_check $? "Package lists updated"
	
	else
		rps_run "$__su apt update"
		rps_check $? "Package lists updated"
	fi
}

