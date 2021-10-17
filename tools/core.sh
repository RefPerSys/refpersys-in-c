# shows an OK message
rps_ok() {
	printf '[\033[0;32m  OK\033[0m] %s...\n' "$1"
}


# shows a warning message
rps_warn() {
	printf '[\033[0;33mWARN\033[0m] %s...\n' "$1"
}


# shows a failure message and aborts
rps_fail() {
	printf '[\033[0;31mFAIL\033[0m] %s.\n' "$1"
	exit
}


# checks if an operation succeeded
mgx_check() {
	if [ "$1" -eq 0 ] ; then
		mgx_ok "$2"
	else
		mgx_fail "Operation failed: $2"
	fi
}

