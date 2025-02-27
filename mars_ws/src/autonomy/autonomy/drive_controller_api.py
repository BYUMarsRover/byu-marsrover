
from rover_msgs.msg import MobilityGPSWaypoint2Follow, MobilityAutopilotCommand, MobilityVelocityCommands, MobilityArucoAutopilotCommand
from std_srvs.srv import SetBool

class DriveControllerAPI:

    def __init__(self, node):
        if node is None:
            raise ValueError("This is a helper class. You need a node to create this object")

        self.node = node # Store the node object for later use
        
        # Publishers - Use node to create publishers
        self.path_cmds_pub = self.node.create_publisher(MobilityGPSWaypoint2Follow, '/mobility/waypoint2follow', 10)
        self.autopilot_cmds_pub = self.node.create_publisher(MobilityAutopilotCommand, '/mobility/autopilot_cmds', 10)
        self.drive_cmds_pub = self.node.create_publisher(MobilityVelocityCommands, '/mobility/rover_vel_cmds', 10)
        self.aruco_autopilot_cmds_pub = self.node.create_publisher(MobilityArucoAutopilotCommand, '/mobility/aruco_autopilot_cmds', 10)

        # Messages
        self.path_cmd = MobilityGPSWaypoint2Follow()
        self.autopilot_cmd = MobilityAutopilotCommand()
        self.drive_cmd = MobilityVelocityCommands()
        self.aruco_autopilot_cmd = MobilityArucoAutopilotCommand()

        # State
        self.path_manager_enabled = False
        self.autopilot_manager_enabled = False
        self.drive_manager_enabled = False
        self.wheel_manager_enabled = False
        self.aruco_autopilot_manager_enabled = False

        # Flags to track if we are attempting to enable or disable the managers
        self.path_manager_attempted_new_status = True
        self.autopilot_manager_attempted_new_status = True
        self.drive_manager_attempted_new_status = True
        self.wheel_manager_attempted_new_status = True
        self.aruco_autopilot_manager_attempted_new_status = True

        self.enable_list_buff = [False] * 5

    def stop(self):
        self._activate_managers([False] * 5)

    def issue_path_cmd(self, lat: float, lon: float, yaw=0):
        # Path Command: Sends latitude and longitude waypoints to be followed.
        self._activate_managers([True, True, True, True, False])
        self.path_cmd.latitude = lat
        self.path_cmd.longitude = lon
        self.path_cmds_pub.publish(self.path_cmd)

    def issue_autopilot_cmd(self, distance: float, heading: float):
        # Autopilot Command: Sends target distance and heading.
        self._activate_managers([False, True, True, True, False])
        self.autopilot_cmd.distance_to_target = distance
        self.autopilot_cmd.course_angle = heading
        self.autopilot_cmds_pub.publish(self.autopilot_cmd)

    def issue_drive_cmd(self, lin_vel: float, ang_vel: float):
        # Drive Command: Sends linear and angular velocity commands.
        self._activate_managers([False, False, True, True, False])
        self.drive_cmd.u_cmd = float(lin_vel)
        self.drive_cmd.omega_cmd = float(ang_vel)
        self.drive_cmds_pub.publish(self.drive_cmd)

    def issue_aruco_autopilot_cmd(self, angle: float, distance: float):
        # ArUco Autopilot Command: Sends movement commands relative to an ArUco marker/object.
        self._activate_managers([False, False, True, True, True])
        self.aruco_autopilot_cmd.distance_to_target = distance
        self.aruco_autopilot_cmd.angle_to_target = angle
        self.aruco_autopilot_cmds_pub.publish(self.aruco_autopilot_cmd)

    def _activate_managers(self, enable_list: list):
        #This method is responsible for enabling/disabling different managers 
        #based on a list of booleans (enable_list)
        # buffer for the previous enabled list
        # if the buffer changed, print what is enabled and disabled and then update the buffer
        assert len(enable_list) == 5, "enable list is wrong size"

        # Update the managers if the enable list is different from current enabled status of the managers
        if enable_list != self.enable_list_buff:
            self.enable_list_buff = enable_list
            if enable_list[0] != self.path_manager_enabled:
                self._toggle_enable_path_manager(enable_list[0])
            if enable_list[1] != self.autopilot_manager_enabled:
                self._toggle_enable_autopilot_manager(enable_list[1])
            if enable_list[2] != self.drive_manager_enabled:
                self._toggle_enable_drive_manager(enable_list[2])
            if enable_list[3] != self.wheel_manager_enabled:
                self._toggle_enable_wheel_manager(enable_list[3])
            if enable_list[4] != self.aruco_autopilot_manager_enabled:
                self._toggle_enable_aruco_autopilot_manager(enable_list[4])
            
            # Log the current status of the managers
            self.node.get_logger().info(f"Path Manager {'ENABLED' if self.path_manager_enabled else 'DISABLED'}")
            self.node.get_logger().info(f"Autopilot Manager: {'ENABLED' if self.autopilot_manager_enabled else 'DISABLED'}")
            self.node.get_logger().info(f"Drive Manager: {'ENABLED' if self.drive_manager_enabled else 'DISABLED'}")
            self.node.get_logger().info(f"Wheel Manager: {'ENABLED' if self.wheel_manager_enabled else 'DISABLED'}")
            self.node.get_logger().info(f"ArUco Autopilot Manager: {'ENABLED' if self.aruco_autopilot_manager_enabled else 'DISABLED'}")

    #TOGGLE & UPDATE FUNCTIONS-------------------------------------------------------------
    #  _toggle_enable_*_manager methods use ROS2 services (SetBool) to enable/disable the respective managers.
        # Each method - 
        # 1. creates a client (create_client)
        # 2. sends a service request with enable as a parameter
        # 3. asynchronously calls the service using client.call_async(request)
        # 4. includes the while not client.wait_for_service(timeout_sec=1.0) loop to ensure that the service is available
    # After the service call, the result is handled in callbacks (_update_*_manager_status), 
    # and if the service call is successful, the respective manager’s enabled status is updated.
    
    def _toggle_enable_path_manager(self, enable: bool):
        client = self.node.path_manager_client
        while not client.wait_for_service(timeout_sec=1.0):
            self.node.get_logger().info('Waiting for service /mobility/path_manager/enabled...')
        request = SetBool.Request()
        request.data = enable
        self.path_manager_attempted_new_status = enable
        future = client.call_async(request)
        future.add_done_callback(self._update_path_manager_status)

    def _update_path_manager_status(self, future):
        if future.result().success:
            self.path_manager_enabled = self.path_manager_attempted_new_status

    def _toggle_enable_autopilot_manager(self, enable: bool):
        client = self.node.autopilot_manager_client
        while not client.wait_for_service(timeout_sec=1.0):
            self.node.get_logger().info('Waiting for service /mobility/autopilot_manager/enabled...')
        request = SetBool.Request()
        request.data = enable
        self.autopilot_manager_attempted_new_status = enable
        future = client.call_async(request)
        future.add_done_callback(self._update_autopilot_manager_status)

    def _update_autopilot_manager_status(self, future):
        if future.result().success:
            self.autopilot_manager_enabled = self.autopilot_manager_attempted_new_status

    def _toggle_enable_drive_manager(self, enable: bool):
        client = self.node.drive_manager_client
        while not client.wait_for_service(timeout_sec=1.0):
            self.node.get_logger().info('Waiting for service /mobility/drive_manager/enabled...')
        request = SetBool.Request()
        request.data = enable
        self.drive_manager_attempted_new_status = enable
        future = client.call_async(request)
        future.add_done_callback(self._update_drive_manager_status)

    def _update_drive_manager_status(self, future):
        if future.result().success:
            self.drive_manager_enabled = self.drive_manager_attempted_new_status

    def _toggle_enable_wheel_manager(self, enable: bool):
        client = self.node.wheel_manager_client
        while not client.wait_for_service(timeout_sec=1.0):
            self.node.get_logger().info('Waiting for service /mobility/wheel_manager/enabled...')
        request = SetBool.Request()
        request.data = enable
        self.wheel_manager_attempted_new_status = enable
        future = client.call_async(request)
        future.add_done_callback(self._update_wheel_manager_status)

    def _update_wheel_manager_status(self, future):
        if future.result().success:
            self.wheel_manager_enabled = self.wheel_manager_attempted_new_status

    def _toggle_enable_aruco_autopilot_manager(self, enable: bool):
        client = self.node.aruco_manager_client
        while not client.wait_for_service(timeout_sec=1.0):
            self.node.get_logger().info('Waiting for service /mobility/aruco_autopilot_manager/enabled...')
        request = SetBool.Request()
        request.data = enable
        self.aruco_autopilot_manager_attempted_new_status = enable
        future = client.call_async(request)
        future.add_done_callback(self._update_aruco_autopilot_manager_status)

    def _update_aruco_autopilot_manager_status(self, future):
        if future.result().success:
            self.aruco_autopilot_manager_enabled = self.aruco_autopilot_manager_attempted_new_status

#-------------------------------------------------------------------------------
#Debugging Tips
# Service Availability:
# The while not client.wait_for_service() blocks in the code are helpful for debugging 
# issues related to service availability. If your node seems stuck, check the services 
# being called—ensure they are actually running.

# Logging:
# Use self.node.get_logger().info() to track the execution flow. If something seems off, you can 
# add additional logs to get more details about which parts of the code are executing.

# Service Callbacks:
# If the service calls don’t behave as expected, verify the response using the callback functions 
# (e.g., _update_*_manager_status). You can log the actual response from the service to see if 
# something unexpected happens.

# Command Issues:
# If the rover isn’t responding to path or velocity commands, ensure that the publishers 
# are active, and the messages (MobilityGPSWaypoint2Follow, MobilityVelocityCommands, etc.) 
# are being populated with correct data before publishing.