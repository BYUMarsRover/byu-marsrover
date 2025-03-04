### Get Started:

- Install WSL2 on your Windows machine by following the instructions [here](https://docs.microsoft.com/en-us/windows/wsl/install).

- Install Docker Desktop on your Windows machine by following the instructions [here](https://docs.docker.com/desktop/), and enable the WSL 2 backend by following the instructions [here](https://docs.docker.com/desktop/windows/wsl/).

- Open a WSL terminal and clone the marsrover repo into your Linux environment using `git clone https://github.com/BYUMarsRover/marsrover.git`.

- Run `bash compose.sh` to pull and launch the latest Docker image from DockerHub.

  > **NOTE:** If you ever need to restart the container, simply run `bash compose.sh down` to stop the container, and then `bash compose.sh` to start it again.

--

### Essential Tutorials:

> **NOTE:** We would ***strongly encourage*** each year's team to take a couple of months at the beginning of the first semester and simply work together through these tutorials before diving into software development. It may not seem romantic, but I promise it'll be worth it.

Linux Command Line Tutorial - [https://linuxjourney.com/lesson/the-shell](https://linuxjourney.com/lesson/the-shell)

GitHub Basics Tutorial - [https://docs.github.com/en/get-started/start-your-journey/hello-world](https://docs.github.com/en/get-started/start-your-journey/hello-world)

Docker Concepts and Tutorials - [https://docs.docker.com/get-started/introduction/whats-next/](https://docs.docker.com/get-started/introduction/whats-next/)

ROS 2 Concepts - [https://docs.ros.org/en/iron/Concepts/Basic.html](https://docs.ros.org/en/iron/Concepts/Basic.html)

ROS 2 CLI Tutorials* - [https://docs.ros.org/en/iron/Tutorials/Beginner-CLI-Tools.html](https://docs.ros.org/en/iron/Tutorials/Beginner-CLI-Tools.html)

ROS 2 Code Tutorials* - [https://docs.ros.org/en/iron/Tutorials/Beginner-Client-Libraries.html](https://docs.ros.org/en/iron/Tutorials/Beginner-Client-Libraries.html)

Robotics in ROS 2 Tutorials - [https://github.com/henki-robotics/robotics_essentials_ros2/tree/main](https://github.com/henki-robotics/robotics_essentials_ros2/tree/main)

ROS 2 Tf2 Tutorials* - [https://docs.ros.org/en/iron/Tutorials/Intermediate/Tf2/Tf2-Main.html](https://docs.ros.org/en/iron/Tutorials/Intermediate/Tf2/Tf2-Main.html)

GPS Navigation w Nav2 Tutorial* - [https://docs.nav2.org/tutorials/docs/navigation2_with_gps.html](https://docs.nav2.org/tutorials/docs/navigation2_with_gps.html)

**All the dependencies for these tutorials are pre-included in the Docker container, and we've mounted 'tutorial_ws' as a dedicated ROS 2 tutorial workspace.*

--

### Contributing:

- **Create a new branch.** Our main code branches are protected, so you'll need to create a new branch. Name your branch with a combination of your name and the feature you are working on (i.e. nelson/repo-docs).

- **Make your changes.** Develop and debug your new feature or bug fix.

  > **NOTE:** If you need to add dependencies, add them to the Dockerfile in your branch and test building the image locally. Once your pull request is merged into main, GitHub CI will automatically build and push the new Docker image to DockerHub.

- **Rebase your branch often.** Keep your branch up-to-date with main by rebasing. This will help prevent merge conflicts down the road.

- **Submit a pull request.** Once you have made and tested your changes, create a new pull request. Get another member of the team to review and approve it, and you can merge your new code into the main branch.

--

Created by Nelson Durrant, Feb 2025.
