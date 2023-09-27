#!/bin/sh

sudo apt remove needrestart -y
sudo apt update -y
sudo apt install make -y
sudo apt install g++ -y
sudo apt install vim -y
sudo apt install iperf3 -y
sudo apt install net-tools -y

make clean
make
