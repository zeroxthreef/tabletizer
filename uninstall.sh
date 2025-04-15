#!/bin/sh
echo uninstall tabletizer
set -ex
rm /usr/local/bin/tabletizer
rm /usr/local/share/tabletizer/config.json
systemctl -M "$(logname)@" --user stop tabletizer.service
systemctl -M "$(logname)@" --user disable tabletizer.service
rm /etc/systemd/user/tabletizer.service
systemctl daemon-reload
systemctl -M "$(logname)@" --user daemon-reload