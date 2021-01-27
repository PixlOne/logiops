#!/usr/bin/env bash

set -e

uninstall_logid() {
    if [ 1 -gt ${#1} ]; then
        printf '%s\n' "No manifest_path detected for uninstall. Skipping..."
        return
    fi
    local manifest_path="$1"
    echo "manifest_path is $manifest_path"
    while IFS= read -r line; do 
        printf '%s\n' "Removing $line.."
        rm "$line"
    done < <(cat "$manifest_path")

    printf '%s\n' "Uninstall complete"
}

get_manifest_path() {
    local manifest_path=$(find . -name 'install-manifest.txt' -type f -print)
    if [ 1 -lt ${#manifest_path} ]; then
        printf '%s\n' "This repo has not been built yet. Please build it to generate install-manifest.txt. Continuing..."
        return
    fi
    echo "$manifest_path"
}

disable_daemon() {
    systemctl status logid 2> /dev/null
    if [ 0 -ne $? ]; then
        printf '%s\n' "Unable to detect logid daemon in systemctl. Exiting..."
        exit 1
    fi
    sudo systemctl disable logid 2> /dev/null
}

# MAIN
project_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$project_dir"
manifest_path=$(get_manifest_path)
uninstall_logid "$manifest_path"
disable_daemon

printf '%s\n' "Completed uninstall"


