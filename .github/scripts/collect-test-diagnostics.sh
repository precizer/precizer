#!/usr/bin/env bash

# Saves system logs and precizer/testitall crash reports since tests started.
# Usage: bash .github/scripts/collect-test-diagnostics.sh START_FILE OUTPUT_DIRECTORY
# START_FILE contains a UTC timestamp and its modification time marks the cutoff.
# Missing diagnostic sources are recorded without changing the test result.
# The CI workflow limits collection time and uploads any available files

set -u

if [[ $# -ne 2 ]]; then
	printf 'Usage: %s START_FILE OUTPUT_DIRECTORY\n' "$0" >&2
	exit 2
fi

start_file="$1"
output_directory="$2"
mkdir -p "$output_directory" || exit 1
exec 2>"$output_directory/collector-errors.txt"

if [[ ! -s "$start_file" ]]; then
	printf 'Test start time is unavailable; system logs were not collected\n' >&2
	exit 1
fi
started_at=$(<"$start_file")

case "$(uname -s)" in
	Darwin)
		# Allow 30 seconds for Crash Reporter to write reports before collecting diagnostics
		sleep 30

		# Include kernel, signing, and Crash Reporter diagnostics, plus messages naming the test processes
		predicate='process == "kernel" OR process == "amfid" OR process == "ReportCrash" OR eventMessage CONTAINS[c] "precizer" OR eventMessage CONTAINS[c] "testitall"'
		sudo -n /usr/bin/log show --start "$started_at+0000" --timezone UTC \
			--style compact --predicate "$predicate" > "$output_directory/system.log"

		# Copy only reports created or updated after the test-start marker
		mkdir -p "$output_directory/crash-reports"
		for reports_directory in "$HOME/Library/Logs/DiagnosticReports" /Library/Logs/DiagnosticReports; do
			if [[ -d "$reports_directory" ]]; then
				find "$reports_directory" -type f -newer "$start_file" \
					\( -name 'precizer*.ips' -o -name 'precizer*.crash' \
					-o -name 'testitall*.ips' -o -name 'testitall*.crash' \) \
					-exec cp {} "$output_directory/crash-reports/" \;
			fi
		done
		;;
	Linux)
		# Kernel OOM events and userspace OOM kills are separate journal sources
		sudo -n journalctl -k --since "$started_at UTC" --utc --no-pager \
			> "$output_directory/kernel.log"
		sudo -n journalctl -u systemd-oomd.service --since "$started_at UTC" --utc --no-pager \
			> "$output_directory/systemd-oomd.log"
		;;
	*)
		printf 'System log collection is unsupported on this platform\n' >&2
		;;
esac

exit 0
