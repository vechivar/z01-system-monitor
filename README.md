# System-monitor

This project uses C++ Dear ImGui library to build a program displaying information about the computer. It is the only project using C++ in the entire cursus.

WARNING : some of this project's features may not work on your computer. See Project State section for deatils.

## How to use

`make` to compile program

`./monitor` to run program

## Features

3 different windows, displaying different informations.

- System window
    - Basic informations (operating system, computer name, user logged in, number of processes, CPU name)
    - Three different charts : CPU usage, processor temperature and fan speed
    - Chart option : slider to adjust FPS (i.e number of displayed values per second)
    - Chart option : change max value (default 100%, 200°C, 5000rpm)
    - Chart option : activate/disactivate animation

- Memory and processes window
    - Physical memory (ram) usage display
    - Virtual memory (swap) usage display
    - Disk usage display
    - Process tables, displaying informations about all processes running on the computer :
        - PID
        - name
        - state
        - CPU usage
        - memory usage
    - Search bar to filter displayed processes

- Network window
    - IPv4 of network interfaces
    - Table of informations on data received and transmitted on every interface
    - Bar display of total data passed through different interfaces (awkward display required by subject)

## Project state

This project has been validated by Zone01 Rouen community, but several points need to be raised.

- This project was built from my own computer running on Ubuntu, but the way all the datas displayed in the program should be collected heavily depends on operating system and computer brand. Therefore, this program will probably not work at all when runnign on non-UNIX operating systems. Some features should work on different UNIX distributions, but other (like temperature or fan speed) will most likely not work when running on different computers. Making a program able to display all the required informations on most computers is way above these project goals.

- CPU usage is a complicated value, and choices need to be made to calculated it. I made those choices and tried to make sure the values I obtained were consistent, but it's extremely complicated to confirm no mistakes were done while calculating them. Even ubuntu `top` command sometimes displays values over 100% usage...

- As an isolated C++ project, requiring heavy documentation research to find data and proper algorithm to update and calculate some values, I'm aware that this project has a lot of room for improvement. However, I feel like I learned a lot from it and I'm satisfied with its current state.