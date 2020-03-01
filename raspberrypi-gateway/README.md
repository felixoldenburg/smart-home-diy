# raspberrypi-gateway

As a the gateway for the smart home project I'll use a RaspberryPi 2 B+ model

## Web Server
The gateway holds the logic for determining the temperature for the heater thermostat clients.
E.g. Reducing target temperature during night to save energy.

Before using a full flexed message based solution the webserver will act as the brain for the clients.
The Huzzah heater thermostat client wakes up every n minutes and will ask the gateway for the desired target temperature.
This way the Huzzah doesn't need to hold any stateful information.

## RaspberryPi Install Log
### Change default password
passwd
### Setup SSH
Enable SSH
sudo raspi-config
ssh-keygen
Generate ssh keys for later authentication with GitHub

### Setup Git
git config --global user.email "oldenburg.felix@gmail.com"
git config --global user.name "Felix Oldenburg"

