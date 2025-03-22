# Streaming H264 and opus

This example streams H264 and opus<sup id="a1">[1](#f1)</sup> samples to the connected browser client.

## Start the example signaling server

```sh
$ python3 examples/signaling-server-python/signaling-server.py
$ node examples/signaling-server-nodejs signaling-server.js
```

## Start a web server

```sh
$ cd examples/streamer
$ python3 -m http.server --bind 127.0.0.1 8080
```

## Start the streamer

```sh
$ cd build/examples/streamer
$ ./streamer
```
Arguments:

- `-a` Directory with OPUS samples (default: *../../../../examples/streamer/samples/opus/*).
- `-b` Directory with H264 samples (default: *../../../../examples/streamer/samples/h264/*).
- `-d` Signaling server IP address (default: 127.0.0.1).
- `-p` Signaling server port (default: 8000).
- `-v` Enable debug logs.
- `-h` Print this help and exit.

You can now open the example at the web server URL [http://127.0.0.1:8080](http://127.0.0.1:8080).

## Generating H264 and Opus samples

You can generate H264 and Opus sample with *samples/generate_h264.py* and *samples/generate_opus.py* respectively. This require ffmpeg, python3 and kaitaistruct library to be installed. Use `-h`/`--help` to learn more about arguments.

<b id="f1">1</b> Opus samples are generated from music downloaded at [bensound](https://www.bensound.com). [↩](#a1)


## Example Exchange


```
SDP
Offer
v=0
o=rtc 4034089392 0 IN IP4 127.0.0.1
s=-
t=0 0
a=group:BUNDLE video-stream audio-stream 0
a=group:LS video-stream audio-stream
a=msid-semantic:WMS *
a=ice-options:ice2,trickle
a=fingerprint:sha-256 C7:79:34:08:6C:DD:5E:D0:F5:97:3A:24:15:9E:08:D9:4A:37:96:69:55:C3:E4:3D:C1:51:B6:02:0A:15:E4:49
m=video 43298 UDP/TLS/RTP/SAVPF 102
c=IN IP4 192.168.0.107
a=mid:video-stream
a=sendonly
a=ssrc:1 cname:video-stream
a=ssrc:1 msid:stream1 video-stream
a=msid:stream1 video-stream
a=rtcp-mux
a=rtpmap:102 H264/90000
a=rtcp-fb:102 nack
a=rtcp-fb:102 nack pli
a=rtcp-fb:102 goog-remb
a=fmtp:102 profile-level-id=42e01f;packetization-mode=1;level-asymmetry-allowed=1
a=setup:actpass
a=ice-ufrag:MjuY
a=ice-pwd:YMNSRjqsKIOvKLshUM4/tG
a=candidate:1 1 UDP 2122317823 192.168.0.107 43298 typ host
a=candidate:2 1 UDP 1686109951 69.76.242.39 43298 typ srflx raddr 0.0.0.0 rport 0
a=end-of-candidates
m=audio 43298 UDP/TLS/RTP/SAVPF 111
c=IN IP4 192.168.0.107
a=mid:audio-stream
a=sendonly
a=ssrc:2 cname:audio-stream
a=ssrc:2 msid:stream1 audio-stream
a=msid:stream1 audio-stream
a=rtcp-mux
a=rtpmap:111 opus/48000/2
a=fmtp:111 minptime=10;maxaveragebitrate=96000;stereo=1;sprop-stereo=1;useinbandfec=1
a=setup:actpass
a=ice-ufrag:MjuY
a=ice-pwd:YMNSRjqsKIOvKLshUM4/tG
m=application 43298 UDP/DTLS/SCTP webrtc-datachannel
c=IN IP4 192.168.0.107
a=mid:0
a=sendrecv
a=sctp-port:5000
a=max-message-size:262144
a=setup:actpass
a=ice-ufrag:MjuY
a=ice-pwd:YMNSRjqsKIOvKLshUM4/tG
Answer
v=0
o=- 7094681694355270390 2 IN IP4 127.0.0.1
s=-
t=0 0
a=group:BUNDLE video-stream audio-stream 0
a=msid-semantic: WMS
m=video 9 UDP/TLS/RTP/SAVPF 102
c=IN IP4 0.0.0.0
a=rtcp:9 IN IP4 0.0.0.0
a=candidate:671207156 1 udp 2113937151 b328855b-0086-4433-84be-84e45ae95fee.local 52988 typ host generation 0 network-cost 999
a=ice-ufrag:qVVq
a=ice-pwd:oXtfDHE80UgcvfqWTpjvcKMF
a=ice-options:trickle
a=fingerprint:sha-256 D5:8B:95:8B:37:75:BE:46:7B:28:58:2E:68:03:38:3C:00:81:50:94:BE:B1:87:C2:65:68:38:20:DF:6C:53:EB
a=setup:active
a=mid:video-stream
a=recvonly
a=rtcp-mux
a=rtpmap:102 H264/90000
a=rtcp-fb:102 goog-remb
a=rtcp-fb:102 nack
a=rtcp-fb:102 nack pli
a=fmtp:102 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f
m=audio 9 UDP/TLS/RTP/SAVPF 111
c=IN IP4 0.0.0.0
a=rtcp:9 IN IP4 0.0.0.0
a=ice-ufrag:qVVq
a=ice-pwd:oXtfDHE80UgcvfqWTpjvcKMF
a=ice-options:trickle
a=fingerprint:sha-256 D5:8B:95:8B:37:75:BE:46:7B:28:58:2E:68:03:38:3C:00:81:50:94:BE:B1:87:C2:65:68:38:20:DF:6C:53:EB
a=setup:active
a=mid:audio-stream
a=recvonly
a=rtcp-mux
a=rtpmap:111 opus/48000/2
a=fmtp:111 minptime=10;useinbandfec=1
m=application 9 UDP/DTLS/SCTP webrtc-datachannel
c=IN IP4 0.0.0.0
a=ice-ufrag:qVVq
a=ice-pwd:oXtfDHE80UgcvfqWTpjvcKMF
a=ice-options:trickle
a=fingerprint:sha-256 D5:8B:95:8B:37:75:BE:46:7B:28:58:2E:68:03:38:3C:00:81:50:94:BE:B1:87:C2:65:68:38:20:DF:6C:53:EB
a=setup:active
a=mid:0
a=sctp-port:5000
a=max-message-size:262144
```