#!/bin/sh
cat /root/banner.txt > /dev/console

ifup eth0

echo "VlVDVEZ7UzNyMWFsX3cxdGhfbTFsa30K" | base64 -d > /dev/console

chmod 666 /etc/graveyard/config
chmod +s /usr/bin/portalctl
echo "portalctl: attracting ghosts..."

daemon --user websrv --name spookystats --respawn --command spookystats
daemon --name dropbear --respawn --command "dropbear -F -R"
