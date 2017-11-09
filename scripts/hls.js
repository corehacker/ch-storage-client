const spawn = require ('child_process').spawn;
const fs = require ('fs');

var base = "/home/pi/streaming/hls";
var live = base + "/live";
var fifo = base + "/live.h264";
var videoH = 720;
var videoW = 1280;
var videoFPS = 25;
var videoBitrate = 1800000;
var videoOutput = fifo;
var ffmpegInput = fifo;
var hlsMaster = base + "/live.m3u8";
var hlsSegment = base + "/live/%08d.ts";
var hlsDvrWindow = 100;

/*
 * rm -rf live live.h264 "$base/live"
 * mkdir -p live
 * #ln -s "$PWD/live" "$base/live"
 * mkfifo live.h264
 * raspivid -w 1280 -h 720 -fps 25 -vf -t 86400000 -b 1800000 -o live.h264 &
 */

/*
 *ffmpeg -y \
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
 */
var encoder = function () {
   var segments = [];
   var removed = new Map ();
   console.log ("Starting encoder...");
   var ffmpeg = spawn ('ffmpeg',
      ["-y", "-i", ffmpegInput, "-f", "s16le", "-i", "/dev/zero",
      "-r:a", "48000", "-ac", "2", "-c:v", "copy", "-c:a", "aac",
      "-b:a", "128k", "-map", "0:0", "-map", "1:0", "-f", "segment",
      "-segment_time", "8", "-segment_format", "mpegts",
      "-segment_list", hlsMaster, "-segment_list_size", hlsDvrWindow,
      "-segment_list_flags", "live", "-segment_list_type", "m3u8",
      "-segment_list_entry_prefix", "live/", hlsSegment]);

   ffmpeg.stdout.on ('data', (data) => {
      console.log (`stdout: ${data}`);
   });
   ffmpeg.stderr.on ('data', (data) => {
      console.log (`stderr: ${data}`);
   });

   fs.watch (live, (eventType, filename) => {
      if (eventType === "rename") {
         if (!removed.has (filename)) {
            console.log (eventType + " " + filename);
            if (segments.length === hlsDvrWindow) {
               var removedSegment = segments.shift ();
               removed.set (removedSegment, null);
               var path = live + "/" + removedSegment;
               console.log ("Removing segment: " + path);
               fs.unlink (path, function (err) {
                  if (!err) {
                     console.log ("Removed segment: " + path);
                  }
               });
            }
            segments.push (filename);
            console.log ("Window: " + segments);
         }
         else {
            removed.delete (filename);
         }
      }
   });
}

var camera = function () {
   console.log ("Starting camera...");
   var raspivid = spawn ('raspivid',
      ["-w", videoW, "-h", videoH, "-fps", videoFPS, "-vf", "-t", "86400000",
       "-b", videoBitrate, "-o", videoOutput]);
   console.log ("raspivid " + " -w " + videoW + " -h " + videoH + " -fps " +
      videoFPS + " -vf " + " -t " + "86400000" + " -b " + videoBitrate +
      " -o " + videoOutput);
   console.log ("Waiting to start encoder...");
   setTimeout (encoder, 2000);
};

var rm = spawn ('rm', ['-rf', 'live', fifo, live]);
rm.on ('close', (code) => {
  console.log ("rm -rf " + live + " " + fifo + " " + live);
  var mkdir = spawn ('mkdir', ['-p', live]);
  mkdir.on ('close', (code) => {
     console.log ("mkdir -p " + live);
     var mkfifo = spawn ('mkfifo', [fifo]);
     mkfifo.on ('close', (code) => {
        console.log ("mkfifo " + fifo);
        camera ();
     });
  });
});
