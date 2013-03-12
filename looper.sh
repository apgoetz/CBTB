#!/bin/sh
while true   
cd ~/CBTB
do  
    nice -n 20 ~/CBTB/client.pl $1 
    sleep 1 
done
