#!/bin/bash
# Created by Nelson Durrant, Feb 2025
#
# Launches tasks on the rover over SSH using the 'rover_launch' tmux session

function printInfo {
  # print blue
  echo -e "\033[0m\033[36m[INFO] $1\033[0m"
}

function printWarning {
  # print yellow
  echo -e "\033[0m\033[33m[WARNING] $1\033[0m"
}

function printError {
  # print red
  echo -e "\033[0m\033[31m[ERROR] $1\033[0m"
}

LOOPBACK_IP_ADDRESS=127.0.0.1
ROVER_IP_ADDRESS=192.168.1.120
FAST_DDS_PORT=11811
DOCKER_SSH_PORT=2233

# Check for an SSH connection to the rover's Docker container
if ! ssh marsrover-docker@$ROVER_IP_ADDRESS -p $DOCKER_SSH_PORT "echo" &> /dev/null
then
    printError "No available SSH connection to the rover's Docker container"
    echo "Here's some debugging suggestions:"
    echo "  - Ensure the rover is powered on"
    echo "  - Ensure the rover is connected with a static IP address"
    echo "  - Ensure the rover's Docker container is running"

    exit
fi

# Launch the specified task configuration over SSH
case "$1" in
    "autonomy")
        printInfo "Setting up the autonomy task..."
        # Send tmux commands to the rover's Docker container over SSH
        ssh marsrover-docker@$ROVER_IP_ADDRESS -p $DOCKER_SSH_PORT "\
            tmux new-session -d -s rover_launch; \
            tmux send-keys -t rover_launch.0 'clear' Enter; \
            tmux send-keys -t rover_launch.0 "export ROS_DISCOVERY_SERVER=$LOOPBACK_IP_ADDRESS:$FAST_DDS_PORT" Enter; \
            tmux send-keys -t rover_launch.0 'source ~/rover_ws/install/setup.bash' Enter; \
            tmux send-keys -t rover_launch.0 'ros2 launch rover_bringup rover_autonomy.launch.py'" # NO ENTER 
        ;;
    "servicing")
        printWarning "Not implemented yet"
        exit
        ;;
    "retrieval")
        printWarning "Not implemented yet"
        exit
        ;;
    "science")
        printWarning "Not implemented yet"
        exit
        ;;
    *)
        printWarning "No task specified, simply entering the current tmux session..."
        echo "Specify a task using 'bash launch.sh <task>' (ex. 'bash launch.sh autonomy')"
        ;;
esac

# Attach to the 'rover_launch' tmux session
ssh -t -X marsrover-docker@$ROVER_IP_ADDRESS -p $DOCKER_SSH_PORT 'tmux attach -t rover_launch'

# Kill the tmux session on exit
ssh marsrover-docker@$ROVER_IP_ADDRESS -p $DOCKER_SSH_PORT 'tmux kill-session -t rover_launch'
