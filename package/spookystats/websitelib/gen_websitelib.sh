#!/usr/bin/env bash
cd $(dirname $0)

xxd -i ./homepage.html > websitelib.c
xxd -i ./systemtelemetry.html >> websitelib.c

cat ./funcs.c >> websitelib.c

echo "Dont forget" to fix the null bytes!
echo "Dont forget" to fix the null bytes! | figlet
