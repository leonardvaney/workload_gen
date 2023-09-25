#!/bin/sh

sudo apt update
sudo apt install make -y
sudo apt install g++ -y
sudo apt install vim -y

make clean
make
