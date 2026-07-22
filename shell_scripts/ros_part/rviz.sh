#!/bin/bash
ROS_DISTRO=$(ls /opt/ros)
source /opt/ros/${ROS_DISTRO}/setup.bash

CONFIG_FILE=@@REPO_PATH@@/configs/steamdeck_config.yaml
ros2 launch leo_teleop joy_teleop.launch.xml joy_config_file:=${CONFIG_FILE} &
LAUNCH_BACK_PID=$!
LAUNCH_BACK_PGID=$(ps -o pgid= -p $LAUNCH_BACK_PID | tr -d ' ')

rviz2 -d $1 &
RVIZ_BACK_PID=$!

wait $RVIZ_BACK_PID
echo "Rviz closed"

echo "Killing launch nodes"
kill -kill -${LAUNCH_BACK_PGID}