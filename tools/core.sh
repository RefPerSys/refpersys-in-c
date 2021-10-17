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

