#!/bin/bash

set -ex

PWD=`pwd`
target_dir="/home/pi/security/hls"
SBR="sbr"
MBR="mbr"

target_dir_sbr="$target_dir/$SBR"
target_dir_mbr="$target_dir/$MBR"

SBR_OR_MBR=$SBR

[[ "${1}" ]] && SBR_OR_MBR=$1

CAPTURE_WIDTH="1280"
CAPTURE_HEIGHT="720"
CAPTURE_FRAMERATE="24"
CAPTURE_BITRATE="2000000"
CAPTURE_COMMAND="raspivid --nopreview --verbose \
        -w $CAPTURE_WIDTH \
        -h $CAPTURE_HEIGHT \
        --framerate $CAPTURE_FRAMERATE \
        --bitrate $CAPTURE_BITRATE \
        --timeout 0 \
        --awb shade \
        --exposure night \
        --ISO 800 \
        --ev 6 \
        -ae 24,0x00,0x8080ff \
        -a 1032 -a 'Garage 1 | %A %Y-%m-%d %X %Z' \
        -ih \
        --output -"

HLS_BASE_SEGMENTS_URL="segments/"

FFMPEG_VIDEO_CODEC="h264_omx"

function generate_sbr_stream {
    echo "Generating SBR Stream."
    mkdir -p $target_dir_sbr
    mkdir -p $target_dir_sbr/segments
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
        -segment_list "$target_dir_sbr/live.m3u8" \
        -segment_list_size 8 \
        -segment_format mpegts \
        -segment_list_flags +live \
        -segment_list_type m3u8 \
        -segment_list_entry_prefix segments/ \
        "$target_dir_sbr/segments/%08d.ts"  

    trap "rm $target_dir_sbr/live.m3u8 $target_dir_sbr/segments/*.ts" EXIT SIGTERM SIGINT
}

function generate_mbr_stream {
    echo "Generating MBR Stream."

    # comment/add lines here to control which renditions would be created
    renditions=(
    # resolution  bitrate  audio-rate
    # "640x360    800k     96k"
    "848x480    1200k    128k"
    "1280x720   2000k    128k"
    )

    segment_target_duration=4       # try to create a new segment every X seconds
    max_bitrate_ratio=1.07          # maximum accepted bitrate fluctuations
    rate_monitor_buffer_ratio=1.5   # maximum buffer size between bitrate conformance checks
    playlist_type="event"           # Live stream
    dvr_window=5                    # DVR window of the live stream.

    target=${target_dir_mbr}

    custom_ffmpeg_path=""
    FFMPEG="ffmpeg"
    [[ "${custom_ffmpeg_path}" ]] && FFMPEG="${custom_ffmpeg_path}/ffmpeg"

    #########################################################################

    rm -rf ${target}

    mkdir -p ${target}


    key_frames_interval="$(echo "$CAPTURE_FRAMERATE*2" | bc || echo '')" # TODO: Parameterize
    key_frames_interval=${key_frames_interval:-50}
    key_frames_interval=$(echo `printf "%.1f\n" $(bc -l <<<"$key_frames_interval/10")`*10 | bc) # round
    key_frames_interval=${key_frames_interval%.*} # truncate to integer

    # static parameters that are similar for all renditions
    # static_params="-c:a aac -ar 48000 -c:v h264 -profile:v main -crf 20 -sc_threshold 0" # No audio required
    static_params+=" -g ${key_frames_interval} -keyint_min ${key_frames_interval} -hls_time ${segment_target_duration}"
    static_params+=" -hls_playlist_type ${playlist_type}"
    # HLS rendition DVR window - sliding window.
    static_params+=" -hls_list_size ${dvr_window}"
    # static_params+=" -hls_delete_threshold ${dvr_window}"
    static_params+=" -hls_flags delete_segments"

    video_codec="-c:v $FFMPEG_VIDEO_CODEC"

    # misc params
    misc_params="-r $CAPTURE_FRAMERATE -y -i -"

    master_playlist="#EXTM3U\n#EXT-X-VERSION:3\n"
    cmd=""
    for rendition in "${renditions[@]}"; do
        # drop extraneous spaces
        rendition="${rendition/[[:space:]]+/ }"

        # rendition fields
        resolution="$(echo ${rendition} | cut -d ' ' -f 1)"
        bitrate="$(echo ${rendition} | cut -d ' ' -f 2)"
        audiorate="$(echo ${rendition} | cut -d ' ' -f 3)"

        # calculated fields
        width="$(echo ${resolution} | grep -oE '^[[:digit:]]+')"
        height="$(echo ${resolution} | grep -oE '[[:digit:]]+$')"
        maxrate="$(echo "`echo ${bitrate} | grep -oE '[[:digit:]]+'`*${max_bitrate_ratio}" | bc)"
        bufsize="$(echo "`echo ${bitrate} | grep -oE '[[:digit:]]+'`*${rate_monitor_buffer_ratio}" | bc)"
        bandwidth="$(echo ${bitrate} | grep -oE '[[:digit:]]+')000"
        name="${height}p"

        # Rendition resolution - scaling
        cmd+=" ${static_params} -vf scale=w=${width}:h=${height}:force_original_aspect_ratio=decrease"

        # Video codec for rendition
        cmd+=" ${video_codec}"

        # Rendition bitrate - video and audio
        cmd+=" -b:v ${bitrate} -maxrate ${maxrate%.*}k -bufsize ${bufsize%.*}k" # No audio required.

        # HLS rendition filename format.
        cmd+=" -hls_segment_filename ${target}/${name}/${HLS_BASE_SEGMENTS_URL}${name}_%08d.ts -hls_base_url ${HLS_BASE_SEGMENTS_URL} ${target}/${name}/live.m3u8"

        # add rendition entry in the master playlist
        master_playlist+="#EXT-X-STREAM-INF:BANDWIDTH=${bandwidth},RESOLUTION=${resolution}\n${name}/live.m3u8\n"

        mkdir -p ${target}/${name}/${HLS_BASE_SEGMENTS_URL}
    done

    # create master playlist file
    echo -e "${master_playlist}" > ${target}/playlist.m3u8

    echo -e "Executing command:\n $CAPTURE_COMMAND | ${FFMPEG} ${misc_params} ${cmd}"

    # -re          | Realtime encoding.
    # -f           | Concation function for looping.
    # -i list.txt  | Input files for looping.
    # ${FFMPEG} -re -f concat ${misc_params} -i list.txt ${cmd}

    $CAPTURE_COMMAND | ${FFMPEG} ${misc_params} ${cmd}



    echo "Done - encoded HLS is at ${target}/"


    # raspivid --nopreview --verbose         -w 1280         -h 720         --framerate 24         --bitrate 2000000         --timeout 0         --awb shade         --exposure night         --ISO 800         --ev 6         -ae 24,0x00,0x8080ff         -a 1032 -a 'Garage 1 | %A %Y-%m-%d %X %Z'         -ih         --output - | \
    # ./ffmpeg -r 24 -y -i -  \
    # -g 48 -keyint_min 48 -hls_time 4 -hls_playlist_type event -hls_list_size 5 -hls_flags delete_segments -vf scale=w=1280:h=720:force_original_aspect_ratio=decrease -c:v h264_omx -b:v 2000k -maxrate 2140k -bufsize 3000k -hls_segment_filename /home/pi/security/hls/mbr/720p/720p_%03d.ts /home/pi/security/hls/mbr/720p/720p.m3u8 
    # -g 48 -keyint_min 48 -hls_time 4 -hls_playlist_type event -hls_list_size 5 -hls_flags delete_segments -vf scale=w=848:h=480:force_original_aspect_ratio=decrease -c:v h264_omx -b:v 1200k -maxrate 1284k -bufsize 1800k -hls_segment_filename /home/pi/security/hls/mbr/480p/480p_%03d.ts /home/pi/security/hls/mbr/480p/480p.m3u8

    # raspivid --nopreview --verbose         -w 1280         -h 720         --framerate 24         --bitrate 2000000         --timeout 0         --awb shade         --exposure night         --ISO 800         --ev 6         -ae 24,0x00,0x8080ff         -a 1032 -a 'Garage 1 | %A %Y-%m-%d %X %Z'         -ih         --output - | \
    # ffmpeg -r 24 -y -i -   \
    # -g 48 -keyint_min 48 -hls_time 4 -hls_playlist_type event -hls_list_size 5 -hls_flags delete_segments -vf scale=w=848:h=480:force_original_aspect_ratio=decrease -c:v h264_omx -b:v 1200k -maxrate 1284k -bufsize 1800k -hls_segment_filename /home/pi/security/hls/mbr/480p/segments/480p_%08d.ts -hls_base_url segments/ /home/pi/security/hls/mbr/480p/live.m3u8  \
    # -g 48 -keyint_min 48 -hls_time 4 -hls_playlist_type event -hls_list_size 5 -hls_flags delete_segments -vf scale=w=1280:h=720:force_original_aspect_ratio=decrease -c:v h264_omx -b:v 2000k -maxrate 2140k -bufsize 3000k -hls_segment_filename /home/pi/security/hls/mbr/720p/segments/720p_%08d.ts -hls_base_url segments/ /home/pi/security/hls/mbr/720p/live.m3u8
}

if [ $SBR_OR_MBR == $SBR ]; then
    generate_sbr_stream
else
    generate_mbr_stream
fi


