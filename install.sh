# Global return value
export rv

# Display success message
msg_ok() {
	printf '[\033[0;32m  OK\033[0m] %s...\n' "$1"
}


# Display warning message
msg_warn() {
	printf '[\033[0;33mWARN\033[0m] %s...\n' "$1"
}


# Display error message and exit
msg_fail() {
	printf '[\033[0;31mFAIL\033[0m] %s.\n' "$1"
	exit
}

# Determines if Linux kernel is running
os_kern() {
	_kern=$(uname -s)

	if [ "$_kern" = "Linux" ] ; then
		if [ -f /etc/os-release ] ; then
			rv=$_kern
		else
			msg_fail "Outdated Linux kernel: $_kern"
		fi

	else
		msg_fail "Unsupported kernel: $_kern"
	fi
}


# Determines Linux distro
os_distro() {
	os_kern

	if [ "$rv" = "Linux" ] ; then
		rv=$(grep 'NAME' /etc/os-release				\
				| head -n1 					\
				| cut -d '=' -f 2 				\
	        		| tr -d '"' 					\
				| cut -d ' ' -f 1)
	fi
}

# Determines host OS version
os_ver() {
	os_distro
	_distro=$rv

	if [ "$_distro" = "Debian" ] ; then
		rv=$(grep 'VERSION=' /etc/os-release				\
				| tr -d '"' 					\
		    		| cut -d '=' -f 2)
		
		_ver=$(echo "$rv" | tr -d 'A-Z a-z ( ) .')

		if [ "$_ver" -lt 10 ] ; then
			msg_warn "Debian version $rv detected"
			msg_fail "Debian version >= 10 required"
		fi

	elif
		rv=$(grep "VERSION_ID" /etc/os-release				\
				| tr -d '"'					\
		    		| cut -d '=' -f 2)
		
		_ver=$(echo "$rv" | tr -d 'A-Z a-z ( ) .')

		if [ "$_ver" -lt 2004 ] ; then
			msg_warn "Ubuntu version $rv detected"
			msg_fail "Ubuntu version >= 20.04 required"
		fi
	else
		msg_fail "Unsupported Linux distro: $_distro"
	fi
}
