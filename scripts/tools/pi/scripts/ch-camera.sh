#!/bin/bash

base="/home/pi/security/hls"

mkdir -p $base
mkdir -p $base/segments

cd $base


# raspivid -a 4 -a "Garage 1 | %A %Y-%m-%d %X %Z" -w 848 -h 480 -fps 24 --profile main -vf -t 0 -b 400000 --rotation 180 -o /security/fifo.h264 &


#ffmpeg \
#	-y -i '/security/fifo.h264' \
#	-f s16le -i /dev/zero -r:a 48000 \
#	-ac 2 -c:v copy -c:a aac -b:a 128k \
#	-map 0:0 -map 1:0 -f segment \
#	-segment_time 8 \
#	-segment_format mpegts \
#	-segment_list '/security/hls/live.m3u8' \
#	-segment_list_size 8 \
#	-segment_list_flags live \
#	-segment_list_type m3u8 \
#	-segment_list_entry_prefix live/ '/security/hls/live/%08d.ts' &


#    --exposure night \


raspivid --nopreview --verbose \
    -w 848 \
    -h 480 \
    --framerate 24 \
    --bitrate 800000 \
    --timeout 0 \
    --awb shade \
    --exposure night \
    --ISO 800 \
    --ev 6 \
    -ae 24,0x00,0x8080ff \
    -a 1032 -a "Garage 1 | %A %Y-%m-%d %X %Z" \
    -ih \
    --output - \
 | ffmpeg -y \
    -r 25 \
    -i - \
    -c:v copy \
    -map 0:0 \
    -framerate 24 \
    -f segment \
    -segment_time 8 \
    -segment_list "$base/live.m3u8" \
    -segment_list_size 8 \
    -segment_format mpegts \
    -segment_list_flags +live \
    -segment_list_type m3u8 \
    -segment_list_entry_prefix segments/ \
    "$base/segments/%08d.ts"  

trap "rm $base/live.m3u8 $base/segments/*.ts" EXIT SIGTERM SIGINT
