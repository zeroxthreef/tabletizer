#!/bin/sh
echo installing tabletizer to /usr/local/bin and config to /usr/local/share
set -ex
cp tabletizer /usr/local/bin
mkdir -p /usr/local/share/tabletizer
cp example_config.json /usr/local/share/tabletizer/config.json
cp tabletizer.service /etc/systemd/user/
systemctl -M "$(logname)@" --user enable tabletizer.service
systemctl daemon-reload
systemctl -M "$(logname)@" --user daemon-reload
systemctl -M "$(logname)@" --user stop tabletizer.service
echo install complete