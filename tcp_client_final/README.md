# README

### files: `TCP_server.py`
    
    run this python file first to start the tcp server. Check the local IP address and change it before running it.

## To Run:
install esp-idf

### In command prompt:
        
```
cd ~/esp/esp-idf
./install.sh
. ./export.sh
cd [folder location]
idf.py menu config
```

go to example config and set the wifi ssid and wifi password

### In `main/tcp_client_v4.c`:
 change the IP to server IP address(the same one as in the python file).

### In command prompt:
 `idf.py -p [port] flash monitor`
