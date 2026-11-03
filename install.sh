#!/bin/sh

set -eu

if [ "$(id -u)" -ne 0 ]; then
    echo "Please execute the script as root (or sudo)" >&2
    exit 1
fi

echo "[  INFO  ] Compiling radish.c..."

gcc radish.c -Wall -Wextra -o radish

echo "[  OK  ] Finished compiling radish.c"

echo "[  INFO  ] Copying the elf file to the binaries folder..."

chmod +x radish
cp radish /usr/bin
rm radish

echo "[  OK  ] Copy completed successfully!"

echo "[  INFO  ] Compressing the man page..."

gzip -k radish.1

echo "[  OK  ] Man page compressed"

echo "[  INFO  ] Moving the man page to the man1 folder..."

mv radish.1.gz /usr/share/man/man1/

echo "[  OK  ] Done"

echo "Instalation finished!"
