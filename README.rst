F-PLUG stat
===========


compile
-------

::

    make


prepare
-------

::

    sudo hcitool scan 
    sudo bluetooth-agent 1234 <address>
    sudo l2ping <address>
    sudo vi /etc/bluetooth/rfcomm.conf
    rfcomm0 {
            # Automatically bind the device at startup
            bind yes;

            # Bluetooth address of the device
            device <address>; 

            # RFCOMM channel for the connection
            channel 1;

            # Description of the connection
            comment "F-PLUG No.1";
    }
    sudo service bluetooth restart


usage
-----

::

    Usage:
	fplugstatd [-c <config_file_path>] [-F]
	fplugstatd -D <stat_file_path>


start daemon
------------

    sudo fplugstatd -c /etc/fplugstatd.conf


api
---

::

    * device list 
    curl  http://127.0.0.1/api/devicies
    
    * realtime data
    curl -X POST -d "address=<address>" [ -d "start=YYYmmDDHHMMSS" ] [ -d "end=YYYmmDDHHMMSS" ]  http://127.0.0.1/api/device/realtime
    
    * hourly power start
    curl -X POST -d "address=<address>" [ -d "init=1" ] http://127.0.0.1/api/device/hourly/power/total
    
    * hourly power 
    curl -X POST -d "address=<address>" [ -d "end=YYYmmDDHHMMSS" ] http://127.0.0.1/api/device/hourly/power/total
    
    * hourly other 
    curl -X POST -d "address=<address>" [ -d "end=YYYmmDDHHMMSS" ] http://127.0.0.1/api/device/hourly/other
    
    * reset
    curl -X POST -d "address=<address>" http://127.0.0.1/api/device/reset
    
    * set datetime
    curl -X POST -d "address=<address>" http://127.0.0.1/api/device/datetime


dump stat
---------

::

    ls /var/tmp/data/fplugstatd
    fplustatd -D <file path of save stat>


config sample
-------------

::

    [global]
    # daemon or local0 - local8 or etc
    syslogFacility = daemon
    # emerge, alert, crit, error, warn, notice, info, debug
    syslogSeverity = info
    
    [fplug]
    # max fplug device to 7
    maxDevice = 3
    # polling interval (sec)
    pollingInterval = 5
    
    [stat]
    # store point (realtime)
    storePoint = 6600000
    # save stat
    save_enable = true
    # save path
    save_path = /var/tmp/data/fplugstatd
    
    [controller]
    # bind address
    httpAddress = 0.0.0.0
    # bind port
    httpPort = 80
    # static resource path
    httpResourcePath = /usr/share/fplugstat/www
    
    [device1]
    # device name
    name = myfplug1
    # device address
    address = <address>
    
    [device2]
    # device name
    name = myfplug2
    # device address
    address = <address>
    

troubleshooting
---------------

::

    sudo bluez-test-device remove <address>
