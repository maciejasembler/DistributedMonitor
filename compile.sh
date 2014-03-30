#!/bin/sh
rm /home/maciej/pvm3/bin/LINUX/master
rm /home/maciej/pvm3/bin/LINUX/slave
g++  master.cpp  -o /home/maciej/pvm3/bin/LINUX/master -lpvm3 -lgpvm3 -lpthread
g++  slave.cpp monitor.cpp semafor.cpp cond.cpp buffer.cpp -o /home/maciej/pvm3/bin/LINUX/slave  -lpvm3 -lgpvm3 -lpthread
#cp master slave /home/maciej/pvm3/bin/LINUX
