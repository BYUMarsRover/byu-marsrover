from setuptools import find_packages, setup
import os
from glob import glob

package_name = 'nav2_autonomy'

setup(
    name=package_name,
    version='0.0.1',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.launch.py')),
        (os.path.join('share', package_name, 'config'), glob('config/*')),
        (os.path.join('share', package_name, 'urdf'), glob('urdf/*')),
        (os.path.join('share', package_name, 'worlds'), glob('worlds/*')),
        (os.path.join('share', package_name, 'models/turtlebot_waffle_gps'),
            glob('models/turtlebot_waffle_gps/*')),
        # Add new models like this so they show up in Gazebo
        (os.path.join('share', package_name, 'models/urc_aruco_0'),
            glob('models/urc_aruco_0/model*')),
        (os.path.join('share', package_name, 'models/urc_aruco_0/meshes'),
            glob('models/urc_aruco_0/meshes/*')),
        (os.path.join('share', package_name, 'models/urc_aruco_1'),
            glob('models/urc_aruco_1/model*')),
        (os.path.join('share', package_name, 'models/urc_aruco_1/meshes'),
            glob('models/urc_aruco_1/meshes/*')),
        (os.path.join('share', package_name, 'models/urc_aruco_2'),
            glob('models/urc_aruco_2/model*')),
        (os.path.join('share', package_name, 'models/urc_aruco_2/meshes'),
            glob('models/urc_aruco_2/meshes/*')),
        (os.path.join('share', package_name, 'models/urc_aruco_3'),
            glob('models/urc_aruco_3/model*')),
        (os.path.join('share', package_name, 'models/urc_aruco_3/meshes'),
            glob('models/urc_aruco_3/meshes/*')),
        (os.path.join('share', package_name, 'models/urc_aruco_4'),
            glob('models/urc_aruco_4/model*')),
        (os.path.join('share', package_name, 'models/urc_aruco_4/meshes'),
            glob('models/urc_aruco_4/meshes/*')),
        (os.path.join('share', package_name, 'models/urc_aruco_5'),
            glob('models/urc_aruco_5/model*')),
        (os.path.join('share', package_name, 'models/urc_aruco_5/meshes'),
            glob('models/urc_aruco_5/meshes/*')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Nelson Durrant',
    maintainer_email='snelsondurrant@gmail.com',
    description='Autonomy capability for the BYU Mars Rover using Nav2',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'behavior_tree = nav2_autonomy.behavior_tree:main',
        ],
    },
)
