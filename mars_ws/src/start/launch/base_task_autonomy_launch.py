
# ==================================
# ==== Base Autonomy Task Launch====
# ==================================

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
from launch.substitutions import LaunchConfiguration, EnvironmentVariable
from launch.actions import SetEnvironmentVariable

from launch_ros.actions import Node
import os

def generate_launch_description():
    # import environment variables
    mapviz_location=os.environ.get('MAPVIZ_LOCATION', 'hanksville')
    mapviz_location_arg = DeclareLaunchArgument('MAPVIZ_LOCATION', default_value='hanksville')

    # Start all common launch files on the base station
    include_base_common = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(
                get_package_share_directory('start'),
                'launch',
                'base_common_launch.py'
            )
        ])
    )

    include_autonomy_gui = Node(
        package='autonomy',
        executable='autonomy_gui',
        name='autonomy_gui',
        namespace='autonomy',
        output='screen',
        parameters=[
            {'location': LaunchConfiguration('MAPVIZ_LOCATION')}
        ],
        additional_env={'MAPVIZ_LOCATION': EnvironmentVariable('MAPVIZ_LOCATION', default_value='hanksville')}
    )

    # Start launch files specific to the Autonomy Task on the base station
    # (The only thing this does is launch the rqt gui)
    # include_base_autonomous = IncludeLaunchDescription(
    #     PythonLaunchDescriptionSource([
    #         os.path.join(
    #             get_package_share_directory('navigation'),
    #             'launch',
    #             'base_autonomous.launch.py'
    #         )
    #     ])
    # )

    #TODO: in the future, when we have built out path planning, include the launch file here
    
    return LaunchDescription([
        mapviz_location_arg,
        include_base_common,
        SetEnvironmentVariable('MAPVIZ_LOCATION', LaunchConfiguration('MAPVIZ_LOCATION')),
        include_autonomy_gui,
        # include_base_autonomous,
        # include_path_planning #TODO: uncomment out this when we build out and include path planning
    ])
