#!/usr/bin/env python3

import cv2
import mediapipe as mp
import rclpy

from rclpy.node import Node
# from sensor_msgs.msg import Image
from sensor_msgs.msg import CompressedImage
import numpy as np

from mediapipe.tasks.python import vision

import os
from ament_index_python.packages import get_package_share_directory
from rclpy import qos

BaseOptions = mp.tasks.BaseOptions
GestureRecognizer = vision.GestureRecognizer
GestureRecognizerOptions = vision.GestureRecognizerOptions
VisionRunningMode = vision.RunningMode

package_share = get_package_share_directory("gesture_recogniser")

MODEL_PATH = os.path.join(package_share, "models", "gesture_recognizer.task")

HAND_CONNECTIONS = [
    (0, 1),
    (1, 2),
    (2, 3),
    (3, 4),
    (0, 5),
    (5, 6),
    (6, 7),
    (7, 8),
    (5, 9),
    (9, 10),
    (10, 11),
    (11, 12),
    (9, 13),
    (13, 14),
    (14, 15),
    (15, 16),
    (13, 17),
    (17, 18),
    (18, 19),
    (19, 20),
    (0, 17),
]


def draw_result(frame, result):
    h, w, _ = frame.shape

    if not result.hand_landmarks:
        return frame

    for gestures, landmarks in zip(result.gestures, result.hand_landmarks):

        for start, end in HAND_CONNECTIONS:
            p1 = landmarks[start]
            p2 = landmarks[end]

            cv2.line(
                frame,
                (int(p1.x * w), int(p1.y * h)),
                (int(p2.x * w), int(p2.y * h)),
                (255, 0, 0),
                2,
            )

        for lm in landmarks:
            cv2.circle(frame, (int(lm.x * w), int(lm.y * h)), 4, (0, 255, 0), -1)

        if gestures:
            gesture = gestures[0]

            x = int(landmarks[0].x * w)
            y = int(landmarks[0].y * h)

            cv2.putText(
                frame,
                f"{gesture.category_name} ({gesture.score:.2f})",
                (x, y - 15),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.8,
                (0, 255, 255),
                2,
            )

    return frame


class GestureRecognitionNode(Node):

    def __init__(self):
        super().__init__("gesture_recognition")

        # self.bridge = CvBridge()

        options = GestureRecognizerOptions(
            base_options=BaseOptions(model_asset_path=MODEL_PATH),
            running_mode=VisionRunningMode.VIDEO,
            num_hands=2,
        )

        self.recognizer = GestureRecognizer.create_from_options(options)

        self.subscription = self.create_subscription(
            # Image, "/camera/image_raw", self.image_callback, qos.qos_profile_sensor_data
            CompressedImage, "/camera/image_color/compressed", self.image_callback, qos.qos_profile_sensor_data
        )

        self.get_logger().info("Gesture recognition node started")

    def image_callback(self, msg):

        np_arr = np.frombuffer(msg.data, dtype=np.uint8)
        frame = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

        # frame = self.ros_image_to_cv2(msg)

        if frame is None:
            self.get_logger().warn("Failed to decode compressed image.")
            return

        frame = cv2.flip(frame, 1)

        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)

        timestamp_ms = msg.header.stamp.sec * 1000 + msg.header.stamp.nanosec // 1000000

        result = self.recognizer.recognize_for_video(mp_image, timestamp_ms)

        frame = draw_result(frame, result)

        cv2.imshow("Gesture Recognition", frame)

        cv2.waitKey(1)

    # def ros_image_to_cv2(self, msg):
    #     if msg.encoding == "bgr8":
    #         return np.frombuffer(
    #             msg.data,
    #             dtype=np.uint8
    #         ).reshape(msg.height, msg.width, 3)

    #     elif msg.encoding == "rgb8":
    #         frame = np.frombuffer(
    #             msg.data,
    #             dtype=np.uint8
    #         ).reshape(msg.height, msg.width, 3)

    #         return cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

    #     else:
    #         raise ValueError(f"Unsupported encoding: {msg.encoding}")

    def destroy_node(self):
        self.recognizer.close()
        cv2.destroyAllWindows()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)

    node = GestureRecognitionNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
