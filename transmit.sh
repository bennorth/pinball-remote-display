#!/bin/bash

sleep 0.5

(sleep 0.5; sigrok-cli -d fx2lafw -C D0,D1,D2,D3 -c samplerate=4mhz --continuous -O binary -o /proc/self/fd/1 ) \
    | ./decode-frame raw-twinkles.bin \
    | gst-launch-1.0 \
        fdsrc fd=0 \
        '!' videoparse width=128 height=32 framerate=21/1 format=25 \
        '!' autovideoconvert \
        '!' x264enc tune=zerolatency byte-stream=true bitrate=160 \
        '!' h264parse \
        '!' rtph264pay \
        '!' udpsink sync=false host=192.168.0.255 port=5935
