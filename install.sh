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
