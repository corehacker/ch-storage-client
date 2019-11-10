#!/bin/bash


ffmpeg \
	-y -i '/security/fifo.h264' \
	-f s16le -i /dev/zero -r:a 48000 \
	-ac 2 -c:v copy -c:a aac -b:a 128k \
	-map 0:0 -map 1:0 -f segment \
	-segment_time 8 \
	-segment_format mpegts \
	-segment_list '/security/hls/live.m3u8' \
	-segment_list_size 8 \
	-segment_list_flags live \
	-segment_list_type m3u8 \
	-segment_list_entry_prefix live/ '/security/hls/live/%08d.ts' &

trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM EXIT

wait
