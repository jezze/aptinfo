#!/bin/bash

if ! test -f ./aptinfo
then
    echo "aptinfo needs to be built first"
    exit 1
fi

test -f Packages || curl -s http://archive.ubuntu.com/ubuntu/dists/jammy/main/binary-amd64/Packages.gz | gunzip > Packages

echo "==========="
echo "Should work"
echo "==========="
./aptinfo compare '2' '>=' '1'
./aptinfo compare '1.0' '=' '1.0'
./aptinfo compare '1.0' '<<' '1.1'
./aptinfo compare '1.0' '<=' '1.1'
./aptinfo compare '1.1' '>>' '1.0'
./aptinfo compare '1.1' '>=' '1.0'
./aptinfo compare '1:1.0' '=' '1:1.0'
./aptinfo compare '1:1.0' '<<' '1:1.1'
./aptinfo compare '1.1-2ubuntu4' '=' '1.1-2ubuntu4'
./aptinfo compare '1:4.4.27-1' '>=' '1:4.4.10-10ubuntu4'
./aptinfo compare '2:1.02.175-2.1ubuntu4' '>=' '2:1.02.175-2.1ubuntu4~'
echo "================"
echo "Should not work:"
echo "================"
./aptinfo compare '2' '<=>' '1'
./aptinfo compare '2' '<=' '1'
./aptinfo compare '1.0' '>>' '1.0'
./aptinfo compare '1.0' '>>' '1.1'
./aptinfo compare '1.0' '>=' '1.1'
./aptinfo compare '1.0' '<<' '1.0'
./aptinfo compare '1.1' '<<' '1.0'
./aptinfo compare '1.1' '<=' '1.0'
./aptinfo compare '1:1.0' '>>' '1:1.1'
./aptinfo compare '1:4.4.27-1' '<=' '1:4.4.10-10ubuntu4'
echo "========="
echo "SHOW wget"
echo "========="
./aptinfo show wget Packages
echo "============"
echo "DEPENDS wget"
echo "============"
./aptinfo depends wget Packages
echo "============="
echo "RDEPENDS wget"
echo "============="
./aptinfo rdepends wget Packages
echo "=============="
echo "RESOLVE wget 1"
echo "=============="
./aptinfo resolve "debconf,wget" Packages
echo "=============="
echo "RESOLVE wget 2"
echo "=============="
./aptinfo resolve "cdebconf,wget" Packages
echo "====================="
echo "RESOLVE ubuntu-server"
echo "====================="
./aptinfo resolve "media-types,pinentry-curses,dpkg,python3-debconf,debconf,dbus,e2fsprogs,libpam-systemd,fdisk,xxd,ubuntu-server" Packages
