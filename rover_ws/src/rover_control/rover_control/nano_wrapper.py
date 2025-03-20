# Copied over by Nelson Durrant, Feb 2025
# Modified to listen to the nav_state Int8 message instead
# I think this could be simplified, we don't use the battery topic at all

# Also, for reference:
# int8 AUTONOMOUS_STATE = 0
# int8 TELEOPERATION_STATE = 1
# int8 ARRIVAL_STATE = 2

"""
This wrapper allows us to merge 2 arduinos into one to allow for more USB ports
"""

import rclpy
from rclpy.node import Node
import serial
from std_msgs.msg import Int8

# from rover_msgs.msg import NavState, Battery, Gripper, RawBattery, Laser, Clicker
import time
import threading
import queue

##### Global Variables #####
# Default no-operation values
gripper = 0
navigation_state = -1
q = queue.Queue()


class NanoWrapper(Node):
    def __init__(self):
        super().__init__("nano_wrapper")

        # Publishers
        # self.battery_pub = self.create_publisher(RawBattery, '/raw_battery_info', 10)

        # Subscribers
        self.create_subscription(Int8, "nav_state", self.led_callback, 10)
        # self.create_subscription(Gripper, '/gripper', self.gripper_callback, 10)
        # self.create_subscription(Laser, '/laser_state', self.laser_callback, 10)
        # self.create_subscription(Clicker, '/click', self.click_callback, 10)

        # Arduino serial setup
        try:
            self.serial_port = serial.Serial("/dev/rover/peripheralsBoard", 9600)
            self.get_logger().info("Serial port initialized")
        except Exception as e:
            self.get_logger().error("Error: peripheralsBoard not yet ready")
            self.get_logger().error(str(e))
            rclpy.shutdown()
            exit(0)

        # Start threads
        self.queue_handler_thread = threading.Thread(target=self.queue_handler)
        self.queue_handler_thread.start()
        self.arduino_listener_thread = threading.Thread(target=self.arduino_listener)
        self.arduino_listener_thread.start()

    def led_callback(self, data):
        # Update LED based on the rover state
        data_array = f"L{data.data};"
        q.put(data_array)

    # def gripper_callback(self, data):
    #     data_array = f"G{data.gripper}:0;"
    #     self.get_logger().info(f"Gripper command: {data_array}")
    #     q.put(data_array)

    # def laser_callback(self, data):
    #     data_array = "S+;" if data.laser_state else "S-;"
    #     self.get_logger().info("Laser call to Arduino")
    #     q.put(data_array)

    # def click_callback(self, data):
    #     data_array = "S!;"
    #     q.put(data_array)

    def arduino_listener(self):
        self.serial_port.flush()

        while rclpy.ok():
            if self.serial_port.in_waiting:
                data = self.serial_port.readline().strip()
                voltage = int(data)

                # bat_voltage_msg = RawBattery()
                # bat_voltage_msg.voltage = voltage
                # self.battery_pub.publish(bat_voltage_msg)
                self.serial_port.flush()

    def queue_handler(self):
        while rclpy.ok():
            if not q.empty():
                try:
                    data = q.get(timeout=1).encode("utf-8")
                    self.serial_port.write(data)
                except Exception as e:
                    self.get_logger().warn(f"Failed to write to serial: {e}")


def main(args=None):
    rclpy.init(args=args)
    nano_wrapper = NanoWrapper()

    try:
        rclpy.spin(nano_wrapper)
    except KeyboardInterrupt:
        pass
    finally:
        nano_wrapper.queue_handler_thread.join()
        nano_wrapper.arduino_listener_thread.join()
        nano_wrapper.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
