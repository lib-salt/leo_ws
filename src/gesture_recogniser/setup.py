from setuptools import find_packages, setup
import glob

package_name = 'gesture_recogniser'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch',
         glob.glob('launch/*.launch.py')),
        ('share/' + package_name + '/models',
         glob.glob('gesture_recogniser/models/*.task')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='neils-legacy',
    maintainer_email='libby.salter@global.amentum.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    entry_points={
        'console_scripts': [
            'gesture_node = gesture_recogniser.gesture_recognition_node:main',
        ],
    },
)
