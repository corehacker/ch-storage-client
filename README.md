# ch-storage-client

[![Build Status](https://travis-ci.org/corehacker/ch-storage-client.png?branch=master)](https://travis-ci.org/corehacker/ch-storage-client)

* Capture on camera enabled Raspberry Pi.
* Encode using ffmpeg to initividual TS segments (HLS).
* The storage client code monitors the file system for new TS files and uploads them to server (over HTTP).
* All the parameters are configurable.
