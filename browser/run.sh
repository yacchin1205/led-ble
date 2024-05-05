#!/bin/bash

docker build -t yacchin1205/led-ble .
docker run --rm -p 8080:80 -v $(pwd)/www:/usr/share/nginx/www:ro yacchin1205/led-ble
