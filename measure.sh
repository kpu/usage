#!/bin/bash
path="$(dirname "$(readlink -f "$0")")"/libusage.so
if [ ! -x "$path" ]; then
  echo Did not find $path or it was not executable 1>&2
  exit 1
fi
export LD_PRELOAD="$path":/usr/lib64/librt.so
exec "$@"
