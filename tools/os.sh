# return value from shell functions
export rv


# checks if Linux kernel is available
rps_os_kern() {
	_kern=$(uname -s)

	if [ "$_kern" = "Linux" ] ; then
		if [ -f /etc/os-release ] ; then
			rv=$_kern
		else
			rps_msg_fail "Outdated Linux kernel: $_kern"
		fi

	else
		rps_mgs_fail "Unsupported kernel: $_kern"
	fi
}


# checks if supported Linux distribution is available
# we are focusing on Ubuntu for now, but keeping the option of
# other distros such as Arch and Debian.
rps_os_distro() {
	rps_os_kern

	if [ "$rv" = "Linux" ] ; then
		rv=$(grep 'NAME' /etc/os-release	\
	            | head -n1 				\
		    | cut -d '=' -f 2 			\
	            | tr -d '"' 			\
		    | cut -d ' ' -f 1)
	fi

	if [ "$rv" != "Arch" ]	\
	    && [ "$rv" != "Debian" ]	\
	    && [ "$rv" != "Ubuntu" ] 	\
		rps_msg_fail "Unsupported distribution: $rv"
	fi
}


# determines distro version number
# we want to use at least Ubuntu 20.04 or Debian 10
# arch is rolling so is always current
rps_os_ver() {
	rps_os_distro
	_distro=$rv

	if [ "$_distro" = "Arch" ] ; then
		rv="rolling"


	elif [ "$_distro" = "Debian" ] ; then
		rv=$(grep 'VERSION=' /etc/os-release	\
		    | tr -d '"' 			\
		    | cut -d '=' -f 2)
		
		_ver=$(echo "$rv" | tr -d 'A-Z a-z ( ) .')

		if [ "$_ver" -lt 10 ] ; then
			rps_msg_warn "Debian version $rv detected"
			rps_msg_fail "Debian version >= 10 required"
		fi

	else
		rv=$(grep "VERSION_ID" /etc/os-release	\
		    | tr -d '"'				\
		    | cut -d '=' -f 2)
		
		_ver=$(echo "$rv" | tr -d 'A-Z a-z ( ) .')

		if [ "$_ver" -lt 2004 ] ; then
			rps_msg_warn "Ubuntu version $rv detected"
			rps_msg_fail "Ubuntu version >= 20.04 required"
		fi
	fi
}

