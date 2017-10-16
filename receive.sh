#!/bin/bash

gst-launch-1.0 -q \
    udpsrc port=5935 \
    '!' application/x-rtp,encoding-name=H264,payload=96 \
    '!' rtph264depay \
    '!' h264parse \
    '!' avdec_h264 \
    '!' autovideoconvert \
    '!' video/x-raw,format=GRAY8 \
    '!' fdsink sync=false fd=1 \
    | \
    ./expand-into-dots 255 200 0 \
    | \
    gst-launch-1.0 -q \
        fdsrc fd=0 \
        '!' videoparse width=792 height=216 framerate=21/1 format=15 \
        '!' autovideoconvert \
        '!' autovideosink sync=false
