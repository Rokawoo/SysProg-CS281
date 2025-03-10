#!/bin/bash

# Function to display information
show_info() {
    echo "=== Process Information ==="
    echo "PID: $$"
    echo "UID/GID: $(id -u)/$(id -g)"
    echo "Username: $(id -un)"
    echo "Groups: $(id -G)"
    
    echo -e "\n=== Namespace Information ==="
    for ns in user mnt pid net ipc uts; do
        echo "$ns namespace: $(readlink /proc/self/ns/$ns)"
    done
    
    echo -e "\n=== Capability Information ==="
    if command -v capsh > /dev/null; then
        capsh --print | grep "Bounding set"
    else
        echo "capsh not available"
    fi
    
    echo -e "\n=== Process List (as seen by this process) ==="
    ps -ef | head -5
}

# Show information before creating a namespace
echo "*** BEFORE ENTERING USER NAMESPACE ***"
show_info

# Create a new user namespace and show information again
echo -e "\n*** AFTER ENTERING USER NAMESPACE ***"
# Export the function so it's available in the child process
export -f show_info
# Don't try to mount proc, just use the existing one
unshare --user bash -c "show_info"