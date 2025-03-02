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
            output="screen",
        ),
        launch_ros.actions.Node(
            # https://github.com/ros2/teleop_twist_joy
            package="teleop_twist_joy",
            executable="teleop_node",
            output="screen",
        ),
        launch_ros.actions.Node(
            package="mobility",
            executable="state_switcher",
            output="screen",
        ),
        launch_ros.actions.Node(
            package="mobility",
            executable="mega_middleman",
            output="screen",
        ),
    ])