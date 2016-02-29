#!/bin/bash

curl http://127.0.0.1/api/devicies
echo ""
curl -X POST -d "address=B0:99:28:A4:5E:D5" http://127.0.0.1/api/device/realtime
echo ""
curl -X POST -d "address=B0:99:28:A4:5E:D5" http://127.0.0.1/api/device/hourly/power/total
echo ""
curl -X POST -d "address=B0:99:28:A4:5E:D5" http://127.0.0.1/api/device/hourly/other
echo ""
#curl -X POST -d "address=B0:99:28:A4:5E:D5" -d "start=init" http://127.0.0.1/api/device/hourly/power/total
#echo ""
#curl -X POST -d "address=B0:99:28:A4:5E:D5" http://127.0.0.1/api/device/reset
#echo ""
#curl -X POST -d "address=B0:99:28:A4:5E:D5" http://127.0.0.1/api/device/datetime
#echo ""
