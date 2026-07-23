from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

config_file = os.path.join(
    get_package_share_directory('gesture_recogniser'),
    'configs',
    'steamdeck_config.yaml'
)


def generate_launch_description():

    joy_node = Node(
        package='joy',
        executable='joy_node',
        name='joy_node',
        output='screen'
    )

    joy_arg = DeclareLaunchArgument(
        'joy',
        default_value='true',
        description='Enable joystick teleop'
    )

    teleop_node = Node(
        package='teleop_twist_joy',
        executable='teleop_node',
        name='teleop_twist_joy',
        parameters=[config_file],
        condition=IfCondition(LaunchConfiguration('joy'))
    )

    gesture_node = Node(
        package='gesture_recogniser',
        executable='gesture_node',
        name='gesture_node',
        output='screen'
    )

    return LaunchDescription([joy_node,joy_arg, teleop_node, gesture_node])