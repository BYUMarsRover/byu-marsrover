#!/bin/bash
# Created by Nelson Durrant, Feb 2025
#
# Launches the full simulation stack for the autonomy task

if [ $(docker ps | grep danielsnider/mapproxy | wc -l) -eq 0 ]; then
    docker run -p 8080:8080 -d -t -v ~/mars_ws/src/nav2_autonomy/mapproxy:/mapproxy danielsnider/mapproxy
fi

source ~/mars_ws/install/setup.bash
ros2 launch nav2_autonomy rover_task_autonomy.launch.py sim_mode:=True use_rviz:=True use_mapviz:=True
