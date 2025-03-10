#!/bin/bash
# Created by Nelson Durrant, Feb 2025
#
# Pulls the base station git branch and changes onto the rover

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

ROVER_IP_ADDRESS=192.168.1.120

# Get the current branch name
current_branch=$(git branch --show-current)

# Check for an SSH connection to the rover
if ! ssh marsrover@$ROVER_IP_ADDRESS "echo" &> /dev/null
then
    printError "No available SSH connection to the rover's computer"
    echo "Here's some debugging suggestions:"
    echo "  - Ensure the rover is powered on"
    echo "  - Ensure the rover is connected with a static IP address"

    exit
fi

# Send tmux commands to the rover over SSH
printInfo "Setting up the sync_git tmux session..."
ssh marsrover@$ROVER_IP_ADDRESS "tmux new-session -d -s sync_git; \
    tmux set-option -g default-terminal "screen-256color"; \
    tmux set -g mouse on; \
    tmux send-keys -t sync_git.0 'clear' Enter; \
    tmux send-keys -t sync_git.0 'cd ~/marsrover/rover_ws' Enter; \
    tmux send-keys -t sync_git.0 'git checkout $current_branch' Enter; \
    tmux send-keys -t sync_git.0 'git pull base $current_branch' Enter; \
    tmux send-keys -t sync_git.0 'colcon build'" # NO ENTER

# Attach to the 'sync_git' tmux session to view the output
ssh -t -X marsrover@$ROVER_IP_ADDRESS "tmux attach -t sync_git"

# Kill the tmux session on exit
ssh marsrover@$ROVER_IP_ADDRESS "tmux kill-session -t sync_git"
