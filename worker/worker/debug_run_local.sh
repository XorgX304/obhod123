#!/bin/bash
echo "Start local"
echo -e "qazwsx\n" | sudo -S nxweb -H 0.0.0.0:80 -w /media/sf_projects_2018/obhod123/src/worker/worker/nxweb -c /media/sf_projects_2018/obhod123/src/worker/worker/nxweb/nxweb_config_local.json

