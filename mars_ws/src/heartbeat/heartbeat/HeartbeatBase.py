#!/usr/bin/python3
import rclpy
from rclpy.node import Node
from rover_msgs.msg import Heartbeat, HeartbeatStatusBase
from . import parameters as p


class BaseHeartbeat(Node):
    def __init__(self):
        super().__init__('base_heartbeat')
        self.last_received = None
        self.startup_time = self.get_clock().now()
        self.sub_heartbeat = self.create_subscription(
            Heartbeat, "/heartbeat_rover", self.update_elapsed_time, 10
        )
        self.pub_heartbeat = self.create_publisher(
            Heartbeat, "/heartbeat_base", 10
        )
        self.pub_heartbeat_status = self.create_publisher(
            HeartbeatStatusBase, "/heartbeat_status_base", 10
        )
        self.timer = self.create_timer(1.0 / p.RATE, self.ping_and_publish)

    def update_elapsed_time(self, msg):
        t = self.get_clock().now()
        self.get_logger().info("Received heartbeat from rover: t=" + str(msg.current_time), throttle_duration_sec=10)
        self.get_logger().debug("Base received heartbeat: t=" + str(msg.current_time),throttle_duration_sec=1)
        self.last_received = t

    def ping_and_publish(self):
        heartbeat_msg = Heartbeat()
        heartbeat_msg.current_time = self.get_clock().now().to_msg()
        self.pub_heartbeat.publish(heartbeat_msg)
        if self.last_received is not None:
            heartbeat_status_msg = HeartbeatStatusBase()
            heartbeat_status_msg.elapsed_time = (
                (self.get_clock().now() - self.last_received).nanoseconds / 1e9
            )
            self.pub_heartbeat_status.publish(heartbeat_status_msg)
            self.check_status()

    def check_status(self):
        t = self.get_clock().now()
        if (t - self.startup_time).nanoseconds / 1e9 < p.BASE_HEARTBEAT_WARMUP:
            return
        if (t - self.last_received).nanoseconds / 1e9 > p.BASE_WARNING_TOLERANCE:
            self.get_logger().warn(p.BASE_WARNING_STRING, throttle_duration_sec=5)


def main(args=None):
    rclpy.init(args=args)
    heartbeat = BaseHeartbeat()
    rclpy.spin(heartbeat)
    heartbeat.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
