#!/bin/bash

PROG="radish"




echo "[  INFO  ] Compiling radish.c..."

gcc radish.c -o radish

echo "[  OK  ] Compiled radish.c"

echo "[  INFO  ] Copying the elf file to the binaries folder..."

chmod +x radish
sudo cp radish /usr/bin
rm radish

echo "[  OK  ] Copy completed successfully!"
