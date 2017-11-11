#!/bin/bash

#base="/var/www/html/hls"
base="/media/workspace/git/ch-storage-client/scripts"

cd $base

set -x

rm -rf live live.h264 "$base/live"
mkdir -p live
#ln -s "$PWD/live" "$base/live"
mkfifo live.h264
#raspivid -w 1280 -h 720 -fps 25 -vf -t 86400000 -b 1800000 -o live.h264 &

#ffmpeg -f video4linux2 -s 320x240 -i /dev/video0 out.mpg

sleep 2
ffmpeg -y \
  -i live.h264 \
  -f s16le -i /dev/zero -r:a 48000 -ac 2 \
  -c:v copy \
  -c:a aac -b:a 128k \
  -map 0:0 -map 1:0 \
  -f segment \
  -segment_time 8 \
  -segment_format mpegts \
  -segment_list "$base/live.m3u8" \
  -segment_list_size 720 \
  -segment_list_flags live \
  -segment_list_type m3u8 \
  -segment_list_entry_prefix "live/" \
  "live/%08d.ts" < /dev/null

# vim:ts=2:sw=2:sts=2:et:ft=sh
