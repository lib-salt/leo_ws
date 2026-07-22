#!/bin/bash
export PATH=$HOME/.local/bin:$PATH
export PATH=$HOME/.local/podman/bin:$PATH
xhost +si:localuser:$USER

SCRIPT_PATH=@@REPO_PATH@@/shell_scripts/ros_part/gesture_recogniser.sh
distrobox enter @@CONTAINER_NAME@@ -- ${SCRIPT_PATH}