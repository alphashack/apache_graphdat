#!/bin/sh
set -o errexit

SYSTEM="linux"
CLOCK_INCLUDE="-l rt"

# check to make sure we have apxs or apxs2
if [ -f "/etc/debian_version" ];
then
  type apxs2 >/dev/null 2>&1 || { echo >&2 "The apache dev tools are not installed.  Please run 'sudo apt-get install apache2-dev' and then try again."; exit 1; }
  BUILDWITH="apxs2"
elif [ -f "/etc/redhat-release" ];
then
  type apxs >/dev/null 2>&1 || { echo >&2 "The apache dev tools are not installed.  Please run 'sudo yum install httpd-dev' and then try again."; exit 1; }
  BUILDWITH="apxs"
# and this for osx
else
    if [[ `uname` == "Darwin" ]]; then
        SYSTEM="osx"
        BUILDWITH="apxs"
        CLOCK_INCLUDE=""
    else
        SYSTEM="unknown"
        echo "unknown operating system: sorry we only support linux and osx right now" >&2
        exit
    fi
fi

sudo $BUILDWITH -c -i -a -n graphdat \
    -I lib/module_graphdat/os \
    $CLOCK_INCLUDE \
    -I  lib/module_graphdat/os/$SYSTEM \
        lib/module_graphdat/list.c \
        lib/module_graphdat/os/$SYSTEM/timehelper.c \
        lib/module_graphdat/os/$SYSTEM/mutex.c \
        lib/module_graphdat/os/$SYSTEM/thread.c \
        lib/module_graphdat/os/$SYSTEM/socket.c \
        lib/module_graphdat/msgpack/src/objectc.c \
        lib/module_graphdat/msgpack/src/unpack.c \
        lib/module_graphdat/msgpack/src/version.c \
        lib/module_graphdat/msgpack/src/vrefbuffer.c \
        lib/module_graphdat/msgpack/src/zone.c \
        lib/module_graphdat/graphdat.c \
        mod_graphdat.c

