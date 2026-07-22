from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():

    joy_arg = DeclareLaunchArgument(
        'joy',
        default_value='false',
        description='Enable joystick teleop'
    )

    teleop_node = Node(
        package='teleop_twist_joy',
        executable='teleop_node',
        name='teleop_twist_joy',
        parameters=[{'joy_config': 'xbox'}],
        condition=IfCondition(LaunchConfiguration('joy'))
    )

    gesture_node = Node(
        package='gesture_recogniser',
        executable='gesture_node',
        name='gesture_node',
        output='screen'
    )

    return LaunchDescription([joy_arg, teleop_node, gesture_node])