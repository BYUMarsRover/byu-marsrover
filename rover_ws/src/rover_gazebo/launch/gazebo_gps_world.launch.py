# Copyright (c) 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import ExecuteProcess, SetEnvironmentVariable
from launch_ros.actions import Node


def generate_launch_description():
    # Get the launch directory
    gazebo_dir = get_package_share_directory(
        "rover_gazebo")
    launch_dir = os.path.join(gazebo_dir, 'launch')
    world = os.path.join(gazebo_dir, "worlds", "sonoma_raceway.world")

    models_dir = os.path.join(gazebo_dir, "models")
    models_dir += os.pathsep + \
        f"/opt/ros/{os.getenv('ROS_DISTRO')}/share/turtlebot3_gazebo/models"
    set_gazebo_model_path_cmd = None

    if 'GAZEBO_MODEL_PATH' in os.environ:
        gazebo_model_path = os.environ['GAZEBO_MODEL_PATH'] + \
            os.pathsep + models_dir
        set_gazebo_model_path_cmd = SetEnvironmentVariable(
            "GAZEBO_MODEL_PATH", gazebo_model_path)
    else:
        set_gazebo_model_path_cmd = SetEnvironmentVariable(
            "GAZEBO_MODEL_PATH", models_dir)

    set_tb3_model_cmd = SetEnvironmentVariable("TURTLEBOT3_MODEL", "waffle")

    # Specify the actions
    start_gazebo_server_cmd = ExecuteProcess(
        cmd=['gzserver', '-s', 'libgazebo_ros_init.so',
             '-s', 'libgazebo_ros_factory.so', world],
        cwd=[launch_dir], output='both')

    start_gazebo_client_cmd = ExecuteProcess(
        cmd=['gzclient'],
        cwd=[launch_dir], output='both')
    
    sim_obj_detect_cmd = Node(
        package="rover_gazebo",
        executable="sim_obj_detect",
        output="screen",
        parameters=[{"use_sim_time": "true"}],
    ),

    # Create the launch description and populate
    ld = LaunchDescription()

    # Set gazebo up to find models properly
    ld.add_action(set_gazebo_model_path_cmd)
    ld.add_action(set_tb3_model_cmd)

    # simulator launch
    ld.add_action(sim_obj_detect_cmd)
    ld.add_action(start_gazebo_server_cmd)
    ld.add_action(start_gazebo_client_cmd)

    return ld
