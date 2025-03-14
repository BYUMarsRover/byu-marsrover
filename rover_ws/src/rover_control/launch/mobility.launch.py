# Created by Nelson Durrant, Feb 2025
import launch
import launch_ros.actions
import launch_ros.descriptions


def generate_launch_description():
    
    return launch.LaunchDescription([
        launch_ros.actions.Node(
            # https://docs.ros.org/en/iron/p/joy/
            package="joy",
            executable="joy_node",
            name="joy_node_rover",
            output="screen",
        ),
        launch_ros.actions.Node(
            # https://github.com/ros2/teleop_twist_joy
            package="teleop_twist_joy",
            executable="teleop_node",
            output="screen",
            remappings=[
                ("cmd_vel", "cmd_vel_teleop"),
            ],
        ),
        launch_ros.actions.Node(
            package="rover_control",
            executable="drive_mux",
            output="screen",
        ),
        launch_ros.actions.Node(
            package="rover_control",
            executable="mega_wrapper",
            output="screen",
        ),
    ])