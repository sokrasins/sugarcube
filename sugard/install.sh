#!/bin/sh

echo "Building sugard.service"
cwd=$(pwd)

if [ -f sugard.service ]; then
  rm sugard.service
fi
cp sugard.service.template sugard.service

sed -i "" -e "s|%%USER%%|$USER|g" -e "s|%%PATH%%|$cwd|g" sugard.service

echo "Installing sugard.service"
sudo mv sugard.service /etc/systemd/system/

echo "Updating systemctl"
sudo systemctl daemon-reload

echo "Starting sugard"
sudo systemctl enable sugard
sudo systemctl start sugard
sudo systemctl status sugard
