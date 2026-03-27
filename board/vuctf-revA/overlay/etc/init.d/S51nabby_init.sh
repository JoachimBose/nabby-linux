#!/bin/sh
cat /root/banner.txt > /dev/console

echo "we have code execution!" > /dev/console
ifup eth0