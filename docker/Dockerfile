# Created by Nelson Durrant, Oct 2024
FROM ros:iron-ros-base
ARG DEBIAN_FRONTEND=noninteractive
ARG TARGETARCH

# Define a username and password for the new user
ARG NAME=marsrover-docker
ARG PASS=marsrover

# Update and upgrade
RUN apt update && apt upgrade -y

# Set up a new user
RUN useradd -ms /bin/bash $NAME
RUN usermod -aG sudo $NAME
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
RUN usermod -aG video $NAME
RUN usermod -aG dialout $NAME
RUN groupadd docker
RUN usermod -aG docker $NAME
RUN echo "$NAME:$PASS" | chpasswd
USER $NAME
WORKDIR /home/$NAME

# Set up the direct Docker ssh connection
USER root
RUN apt update && apt install -y openssh-server
RUN mkdir -p /var/run/sshd
RUN sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config
RUN sed -i 's/PasswordAuthentication no/PasswordAuthentication yes/' /etc/ssh/sshd_config
RUN sed -i 's/\(^Port\)/#\1/' /etc/ssh/sshd_config && echo Port 2233 >> /etc/ssh/sshd_config
EXPOSE 2233
USER $NAME

# Install general dependencies
USER root
RUN apt update && apt install -y \
    curl \
    vim \
    tmux \
    docker.io \
    python3-pip
USER $NAME

# Install graphical interface dependencies
USER root
RUN apt update && apt install -y \
    x11-apps \
    x11-utils \
    x11-xserver-utils \
    xauth \
    ros-iron-rqt*
USER $NAME

# Install the navigation2 and robot_localization packages
USER root
RUN apt update && apt install -y \
    ros-iron-navigation2 \
    ros-iron-robot-localization
USER $NAME

# Install tutorial-specific packages
USER root
RUN apt update && apt install -y \
    ros-iron-teleop-twist-keyboard \
    ros-iron-rviz2 \
    python3-tk
USER $NAME

# Install gazebo and turtlebot3 packages
# For some reason, these packages are not available for arm64
USER root
RUN if [ "$TARGETARCH" = "amd64" ]; then \
    apt update && apt install -y \
    ros-iron-nav2-bringup \
    ros-iron-turtlebot3-gazebo; \
    # if we ever switch to the new gazebo, change this to ros-<ros2-distro>-nav2-minimal-tb*
fi
USER $NAME

# Install the aruco_opencv package
USER root
RUN apt update && apt install -y ros-iron-aruco-opencv
USER $NAME

# Install mapviz packages
# For some reason, these packages are not available for arm64
USER root
RUN if [ "$TARGETARCH" = "amd64" ]; then \
    apt update && apt install -y \
    ros-iron-mapviz \
    ros-iron-mapviz-plugins \
    ros-iron-tile-map; \
fi
USER $NAME

# Set up automatic ROS 2 sourcing and colorized output
RUN echo "source /opt/ros/iron/setup.bash" >> ~/.bashrc
RUN echo "export RCUTILS_COLORIZED_OUTPUT=1" >> ~/.bashrc
RUN touch ~/.hushlogin

###############################################################
# IMPORTANT! When adding dependencies or Dockerfile commands
# below, please document your changes following this format:
#     
#     # What do these commands do? - Full Name, Month Year
#     # Why do we need them? Which ROS 2 nodes require them?
#     < Add Dockerfile commands here >
#
###############################################################

# Install matplotlib - Nelson Durrant, Oct 2024
# Used for plotting the path planning script results
USER root
RUN apt update && apt install -y python3-matplotlib
USER $NAME
