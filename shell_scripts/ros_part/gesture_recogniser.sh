#!/bin/bash

ROS_DISTRO=$(ls /opt/ros)
source /opt/ros/${ROS_DISTRO}/setup.bash

source ~/leo_ws/install/setup.bash

ros2 launch gesture_recogniser demo.launch.py
