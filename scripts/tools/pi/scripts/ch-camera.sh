#!/bin/bash

set -ex

PWD=`pwd`
target_dir="/home/pi/security/hls"
SBR="sbr"
MBR="mbr"

target_dir_sbr="$target_dir/$SBR"
target_dir_mbr="$target_dir/$MBR"

SBR_OR_MBR=$MBR

[[ "${1}" ]] && SBR_OR_MBR=$1

CAPTURE_BANNER="Garage 1 - %A %Y-%m-%d %X %Z"
CAPTURE_WIDTH="1280"
CAPTURE_HEIGHT="720"
CAPTURE_FRAMERATE="18"
CAPTURE_BITRATE="2000000"

BITRATE_FACTOR=1

HLS_BASE_SEGMENTS_URL="segments/"

FFMPEG_VIDEO_CODEC="h264_omx"
#FFMPEG_VIDEO_CODEC="libx264"

FFMPEG_CUSTOM_BINARY_PATH="/usr/local/bin/ffmpeg-n4.2.1-dynamic"

remove_files=""

function generate_sbr_stream {
    # The generated SBR HLS stream is a passthrough stream. raspivid generates an H.264 encoded stream.
    # ffmpeg is used only to segment the stream.
    echo "Generating SBR Stream."
    mkdir -p $target_dir_sbr
    mkdir -p $target_dir_sbr/segments

    FFMPEG="ffmpeg"
    [[ "${FFMPEG_CUSTOM_BINARY_PATH}" ]] && FFMPEG="${FFMPEG_CUSTOM_BINARY_PATH}/ffmpeg"

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
    | ${FFMPEG} -y \
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

}

function generate_mbr_stream {
    # The generated MBR HLS stream is a transcoded stream. raspivid generates an H.264 encoded stream.
    # ffmpeg is used to transcode multiple renditions.
    echo "Generating MBR Stream."

    # comment/add lines here to control which renditions would be created
    # Renditions should be ordered by width and height and bitrate.
    # The highest resolution and bitrate will be used as capture parameter.
    renditions=(
    # resolution  bitrate  audio-rate
    # "640x360    800k     96k"
    "848x480    1200k    128k"
    #"1280x720   3000k    128k"
    "1600x912   4000k    128k"
    #"1920x1088  4000k    128k"
    )

    segment_target_duration=4       # try to create a new segment every X seconds
    max_bitrate_ratio=1.07          # maximum accepted bitrate fluctuations
    rate_monitor_buffer_ratio=1.5   # maximum buffer size between bitrate conformance checks
    playlist_type="event"           # Live stream
    dvr_window=8                    # DVR window of the live stream.

    target=${target_dir_mbr}

    
    FFMPEG="ffmpeg"
    [[ "${FFMPEG_CUSTOM_BINARY_PATH}" ]] && FFMPEG="${FFMPEG_CUSTOM_BINARY_PATH}/ffmpeg"

    #########################################################################

    mkdir -p ${target}


    key_frames_interval="$(echo "$CAPTURE_FRAMERATE*2" | bc || echo '')" # TODO: Parameterize
    key_frames_interval=${key_frames_interval:-50}
    key_frames_interval=$(echo `printf "%.1f\n" $(bc -l <<<"$key_frames_interval/10")`*10 | bc) # round
    key_frames_interval=${key_frames_interval%.*} # truncate to integer

    # static parameters that are similar for all renditions
    # static_params="-c:a aac -ar 48000 -c:v h264 -profile:v main -crf 20 -sc_threshold 0" # No audio required
    # static_params+=" -f hls"
    static_params+=" -g ${key_frames_interval} -keyint_min ${key_frames_interval} -hls_time ${segment_target_duration}"
    #static_params+=" -hls_playlist_type ${playlist_type}"
    # HLS rendition DVR window - sliding window.
    static_params+=" -hls_list_size ${dvr_window}"
    static_params+=" -hls_delete_threshold ${dvr_window}"
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

	CAPTURE_WIDTH=${width}
	CAPTURE_HEIGHT=${height}
	CAPTURE_BITRATE=$(bc -l <<< "${bandwidth}*${BITRATE_FACTOR}")

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

	remove_files="${remove_files} ${target}/${name}"
    done

    # create master playlist file
    echo -e "${master_playlist}" > ${target}/playlist.m3u8

    #COMMAND="raspivid --nopreview --verbose -w $CAPTURE_WIDTH -h $CAPTURE_HEIGHT --framerate $CAPTURE_FRAMERATE --bitrate $CAPTURE_BITRATE --timeout 0 --awb shade --exposure night --ISO 800 --ev 6 -ae 24,0x00,0x8080ff -a 1032 -a "${CAPTURE_BANNER}" -ih --output - | ${FFMPEG} ${misc_params} ${cmd}"
    COMMAND="raspividyuv --nopreview --verbose -w $CAPTURE_WIDTH -h $CAPTURE_HEIGHT --framerate $CAPTURE_FRAMERATE --timeout 0 --awb shade --exposure night --ISO 800 --ev 6 -ae 24,0x00,0x8080ff -a 1032 -a "${CAPTURE_BANNER}" --output - | ${FFMPEG} ${misc_params} ${cmd}"

    echo -e "Executing command:\n ${COMMAND}"

#    raspivid --nopreview --verbose \
#            --rgb \
#	    -w $CAPTURE_WIDTH \
#	    -h $CAPTURE_HEIGHT \
#	    --bitrate $CAPTURE_BITRATE \
#	    --framerate $CAPTURE_FRAMERATE \
#	    --timeout 0 --awb shade \
#	    --exposure night --ISO 800 --ev 6 \
#	    -ae 24,0x00,0x8080ff -a 1032 \
#	    -a "${CAPTURE_BANNER}" \
#	    -ih \
#	    --output - | \
#	    ${FFMPEG} ${misc_params} ${cmd}


    raspividyuv --nopreview --verbose \
	    -w $CAPTURE_WIDTH \
	    -h $CAPTURE_HEIGHT \
	    --framerate $CAPTURE_FRAMERATE \
	    --timeout 0 --awb shade \
	    --exposure night --ISO 200 --ev 6 \
	    -ae 24,0x00,0x8080ff -a 1032 \
	    -a "${CAPTURE_BANNER}" \
	    --output - | \
	    ${FFMPEG} -f rawvideo -pix_fmt yuv420p -s:v "${CAPTURE_WIDTH}x${CAPTURE_HEIGHT}" ${misc_params} ${cmd}

    

    echo "Done - encoded HLS is at ${target}/"
}

if [ $SBR_OR_MBR == $SBR ]; then
    generate_sbr_stream
else
    generate_mbr_stream
fi


trap "rm -rf $target_dir_sbr/live.m3u8 $target_dir_sbr/segments/*.ts ${remove_files}" EXIT SIGTERM SIGINT