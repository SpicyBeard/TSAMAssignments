#!/bin/bash

# Compile both C++ programs
#g++ -o scanner scanner.cpp
#g++ -o puzzlesolver puzzlesolver.cpp
echo "Compiling necessery files" 
echo ""
make
#print compile status

# Ip address and port range
ip_addr="130.208.246.249"
low_port=4000
high_port=4100

echo "Scanning $ip_addr for open ports in range $low_port-$high_port"

# Run the scanner and capture the output
open_ports=$(./scanner "$ip_addr" "$low_port" "$high_port")

# Check if we found any open ports
if [[ -z "$open_ports" ]]; then
    echo "No open ports found. Exiting."
    exit 1
fi

# Extract the open ports from the output (assuming format: "Open ports: 1234 5678 91011 1112")
open_ports=($open_ports)  # Convert the string into an array
port1=${open_ports[2]}
port2=${open_ports[3]}
port3=${open_ports[4]}
port4=${open_ports[5]}

echo "Open ports found: $port1, $port2, $port3, $port4"
# Make sure the ports are not empty
if [[ -z "$port1" || -z "$port2" || -z "$port3" || -z "$port4" ]]; then
    echo "Error: Not enough ports detected."
    exit 1
fi

# Run the puzzlesolver with the open ports
echo "Running puzzlesolver with ports: $port1, $port2, $port3, $port4 \n"
echo ""
./puzzlesolver "$ip_addr" "$port1" "$port2" "$port3" "$port4"

echo "Cleaning up the files"
make clean