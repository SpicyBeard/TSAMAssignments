Assignment 3

Group 36
Daði Rúnarsson (dadir21@ru.is)
Lovísa Baldvinsdóttir (lovisa21@ru.is)

Project setup:
    In the main directory we have both the scanner and puzzle solver, 
    along with the Makefile. 
    We also have the common.cpp and common.h files that contain functions
    that are reused throughout the project. 
    Within the PortSolver folder we then have dedicated .cpp and .h files 
    for each stage of the puzzle solver.
    All work was done on Ubuntu through WSL on windows.

How to compile:
    To compile all neccecery file you can use the Makefile supplied with 
    the following command:
        make
    
How to run:
    The scanner and puzzle solver can be run on their own following the 
    instructions in the assignment:
    - To run the scanner:
        ./scanner <IP address> <lowest port> <highest port>
        where the lowest port is where you want to start scanning and the 
        highest port is how far you want to scann.
    - To runn the puzzle solver:
        sudo ./puzzlesolver <IP address> <port1> <port2> <port3> <port4>
        with the ports from the scanner.
    
    If you want to compile and run both at the same time you can run the 
    following script:
        sudo ./run_puzzle.sh
    After running the script, all executable files are removed.

    Using sudo is required for the puzzle solver because of the raw socket 
    used there.

Know bugs:
    I was running into an issue where sometimes the checksum looked like 
    it wasn't in the first 2 bytes of the last 6 bytes of the message we 
    recieved from the server. Im not sure if this was an issue on our end, 
    or on the server itself. For example in the message text it said that 
    the checksum was 0x2b21, while the last 6 bytes were 0x72292b2125bf 
    (in network byte order).
    It seems like this issue is resolved for now, at least I haven't seen 
    it for a while, despite running it frequently.
