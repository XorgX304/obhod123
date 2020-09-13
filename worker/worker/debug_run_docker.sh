#!/bin/bash
cd /media/sf_projects_2018/aimore/obhod123/src/worker/worker
image="obhod123/obhod123"


echo "Start"
ds=$(echo -e "qazwsx\n" | sudo -S docker ps -a -q)
echo $ds
echo -e "qazwsx\n" | sudo -S docker build -t "$image" .
echo -e "qazwsx\n" | sudo -S docker run --mount type=bind,source=/obhod123,destination=/obhod123 --rm -p 1194:1194/udp -p 80:80/tcp -e ip='192.168.0.109' --cap-add=NET_ADMIN obhod123/obhod123

