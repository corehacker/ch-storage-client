# ch-storage-client

[![Build Status](https://dev.azure.com/prakashsandeep/prakashsandeep/_apis/build/status/corehacker.ch-storage-client)](https://dev.azure.com/prakashsandeep/prakashsandeep/_build/latest?definitionId=3)

* Capture on camera enabled Raspberry Pi.
* Encode using ffmpeg to initividual TS segments (HLS).
* The storage client code monitors the file system for new TS files and uploads them to server (over HTTP).
* All the parameters are configurable.
