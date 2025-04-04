/**
 * libdatachannel client example
 * Copyright (c) 2019-2020 Paul-Louis Ageneau
 * Copyright (c) 2019 Murat Dogan
 * Copyright (c) 2020 Will Munn
 * Copyright (c) 2020 Nico Chatzi
 * Copyright (c) 2020 Lara Mackey
 * Copyright (c) 2020 Erik Cota-Robles
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "nlohmann/json.hpp"

#include "h264fileparser.hpp"
#include "opusfileparser.hpp"
#include "helpers.hpp"
#include "ArgParser.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>

#include <chrono>

using namespace rtc;
using namespace std;
using namespace std::chrono_literals;

using json = nlohmann::json;

template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

/// all connected clients
unordered_map<string, shared_ptr<Client>> clients{};

/// Creates peer connection and client representation
/// @param config Configuration
/// @param wws Websocket for signaling
/// @param id Client ID
/// @returns Client
shared_ptr<Client> createPeerConnection(const Configuration &config,
                                        weak_ptr<WebSocket> wws,
                                        string id);

/// Creates stream
/// @param h264Samples Directory with H264 samples
/// @param fps Video FPS
/// @param opusSamples Directory with opus samples
/// @returns Stream object
shared_ptr<Stream> createStream(const string h264Samples, const unsigned fps, const string opusSamples);

/// Add client to stream
/// @param client Client
/// @param adding_video True if adding video
void addToStream(shared_ptr<Client> client, bool isAddingVideo);

/// Start stream
void startStream();

/// Main dispatch queue
DispatchQueue MainThread("Main");

/// Audio and video stream
optional<shared_ptr<Stream>> avStream = nullopt;

const string defaultRootDirectory = "../../../examples/streamer/samples/";
const string defaultH264SamplesDirectory = defaultRootDirectory + "h264/";
string h264SamplesDirectory = defaultH264SamplesDirectory;
const string defaultOpusSamplesDirectory = defaultRootDirectory + "opus/";
string opusSamplesDirectory = defaultOpusSamplesDirectory;
const string defaultIPAddress = "127.0.0.1";
const uint16_t defaultPort = 8000;
string ip_address = defaultIPAddress;
uint16_t port = defaultPort;

/// Incomming message handler for websocket
/// @param message Incommint message
/// @param config Configuration
/// @param ws Websocket
void wsOnMessage(json message, Configuration config, shared_ptr<WebSocket> ws) {
    auto it = message.find("id");
    if (it == message.end())
        return;
    string id = it->get<string>();

    it = message.find("type");
    if (it == message.end())
        return;
    string type = it->get<string>();

    if (type == "request") {
        clients.emplace(id, createPeerConnection(config, make_weak_ptr(ws), id));
    } else if (type == "answer") {
        if (auto jt = clients.find(id); jt != clients.end()) {
            auto pc = jt->second->peerConnection;
            auto sdp = message["sdp"].get<string>();
            auto description = Description(sdp, type);
            pc->setRemoteDescription(description);
        }
    }
}

typedef void (on_sample_t)(char* buffer, int len) ;

const char startcode[] = {0x00, 0x00, 0x00, 0x01};
static
void* receiver_thread(void* arg)

{
    on_sample_t* on_sample = (on_sample_t*)arg;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if( sock < 0 )
	{
		printf("socket failed\n");
		return NULL;
	}

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12346);
    inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr.s_addr));

    	// connect
	if( connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0 )
	{
		printf("connect failed\n");
		return NULL;
	}
    printf("Connected!\n");

    // if (bind(sock, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
    //     perror("bind failed");
    //     exit(EXIT_FAILURE);
    // }
    std::vector<char> data;
    char buffer[1024 * 1024] = {0};
    while (true)
    {
        // struct sockaddr_in client_addr = {0};
   
    
    
        int n = recv(sock, (char *)buffer, sizeof(buffer), 0);
        // printf("ppack->u32Flag = %d\n", ppack->u32Flag);
        if (n < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }
        printf("Received: %d bytes\n", n);
        
        if (memcmp(buffer, startcode, 4) == 0) {
            printf("Startcode\n");
            
            if (data.size() > 0)
            on_sample(data.data(), data.size());

            data.clear();
        }

        data.insert(data.end(), buffer, buffer + n);
    }


    // close(sock);
    pthread_exit(NULL);
}

int frame = 0;
int started = 0;
void* on_sample_cb(char* buffer, int len)
{
    // Process the received sample
   
    // the 5th byte is the NAL Header
    // e.g. 0x00 0x00 0x00 0x01 <nal_header> 
    // the zeros are the start code
    int type = buffer[4] & 0x1F;
    // if (type == 1 || type == 5)
    //     frame += 1;
    

    printf("Received sample of length: %d type: %d\n", len, type);

    printf("Sample bytes: ");
 
    
    int i = 0;
    for (i = 0; i < len && i < 10; i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");


    // // Sample time in 90KHz clock
    // int sampleTime = frame * 90000 / 30;
    // printf("Sample Time: %lu\n", sampleTime);

    
    // printf("Sample size in hex: %lx, %lx\n", sample.size(), ntohl(sample.size()));

    vector<ClientTrack> tracks{};
    std::vector<std::byte> data((std::byte*) buffer, (std::byte*)buffer + len);

  
    // get all clients with Ready state
    for(auto id_client: clients) {
        auto id = id_client.first;
        auto client = id_client.second;

        
        if (client->getState() == Client::State::Ready) {
    
            if (client->video.has_value()) {
                printf("Sending\n");
                if (!started && type != 7)
                    continue;
                started = 1;
                if (type == 1 || type == 5)
                    frame += 1;
    // Sample time in 90KHz clock
    int sampleTime = frame * 90000 / 30;
    printf("Sample Time: %lu\n", sampleTime);
                std::shared_ptr<ClientTrackData> td = client->video.value();

                // if (type == 1 || type == 5)
                    td->track->sendFrame(data, std::chrono::duration<double, std::micro>(sampleTime));
                // else
                //     td->track->send
              
            }

        }
    }

    return NULL;
}

int main(int argc, char **argv) try {
    bool enableDebugLogs = false;
    bool printHelp = false;
    int c = 0;
    auto parser = ArgParser({{"a", "audio"}, {"b", "video"}, {"d", "ip"}, {"p","port"}}, {{"h", "help"}, {"v", "verbose"}});
    auto parsingResult = parser.parse(argc, argv, [](string key, string value) {
        if (key == "audio") {
            opusSamplesDirectory = value + "/";
        } else if (key == "video") {
            h264SamplesDirectory = value + "/";
        } else if (key == "ip") {
            ip_address = value;
        } else if (key == "port") {
            port = atoi(value.data());
        } else {
            cerr << "Invalid option --" << key << " with value " << value << endl;
            return false;
        }
        return true;
    }, [&enableDebugLogs, &printHelp](string flag){
        if (flag == "verbose") {
            enableDebugLogs = true;
        } else if (flag == "help") {
            printHelp = true;
        } else {
            cerr << "Invalid flag --" << flag << endl;
            return false;
        }
        return true;
    });
    if (!parsingResult) {
        return 1;
    }

    if (printHelp) {
        cout << "usage: stream-h264 [-a opus_samples_folder] [-b h264_samples_folder] [-d ip_address] [-p port] [-v] [-h]" << endl
        << "Arguments:" << endl
        << "\t -a " << "Directory with opus samples (default: " << defaultOpusSamplesDirectory << ")." << endl
        << "\t -b " << "Directory with H264 samples (default: " << defaultH264SamplesDirectory << ")." << endl
        << "\t -d " << "Signaling server IP address (default: " << defaultIPAddress << ")." << endl
        << "\t -p " << "Signaling server port (default: " << defaultPort << ")." << endl
        << "\t -v " << "Enable debug logs." << endl
        << "\t -h " << "Print this help and exit." << endl;
        return 0;
    }
    if (enableDebugLogs) {
        InitLogger(LogLevel::Debug);
    }

    Configuration config;
    string stunServer = "stun:stun.l.google.com:19302";
    cout << "STUN server is " << stunServer << endl;
    config.iceServers.emplace_back(stunServer);
    config.disableAutoNegotiation = true;

    string localId = "server";
    cout << "The local ID is: " << localId << endl;

    auto ws = make_shared<WebSocket>();

    ws->onOpen([]() { cout << "WebSocket connected, signaling ready" << endl; });

    ws->onClosed([]() { cout << "WebSocket closed" << endl; });

    ws->onError([](const string &error) { cout << "WebSocket failed: " << error << endl; });

    ws->onMessage([&](variant<binary, string> data) {
        if (!holds_alternative<string>(data))
            return;

        json message = json::parse(get<string>(data));
        MainThread.dispatch([message, config, ws]() {
            wsOnMessage(message, config, ws);
        });
    });
    const string url = "ws://" + ip_address + ":" + to_string(port) + "/" + localId;
    cout << "URL is " << url << endl;
    ws->open(url);

    cout << "Waiting for signaling to be connected..." << endl;
    while (!ws->isOpen()) {
        if (ws->isClosed())
            return 1;
        this_thread::sleep_for(100ms);
    }

    pthread_t t1 = {0};
    int res = pthread_create(&t1, NULL, &receiver_thread, (void*)&on_sample_cb);


    while (true) {
        string id;
        cout << "Enter to exit" << endl;
        cin >> id;
        cin.ignore();
        cout << "exiting" << endl;
        break;
    }

    cout << "Cleaning up..." << endl;
    return 0;

} catch (const std::exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
    return -1;
}

shared_ptr<ClientTrackData> addVideo(const shared_ptr<PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const string cname, const string msid, const function<void (void)> onOpen) {
    auto video = Description::Video(cname);
    video.addH264Codec(payloadType);
    video.addSSRC(ssrc, cname, msid, cname);
    auto track = pc->addTrack(video);
    // create RTP configuration
    auto rtpConfig = make_shared<RtpPacketizationConfig>(ssrc, cname, payloadType, H264RtpPacketizer::ClockRate);
    // create packetizer
    auto packetizer = make_shared<H264RtpPacketizer>(NalUnit::Separator::StartSequence, rtpConfig);
    // add RTCP SR handler
    auto srReporter = make_shared<RtcpSrReporter>(rtpConfig);
    packetizer->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = make_shared<RtcpNackResponder>();
    packetizer->addToChain(nackResponder);
    // set handler
    track->setMediaHandler(packetizer);
    track->onOpen(onOpen);
    auto trackData = make_shared<ClientTrackData>(track, srReporter);
    return trackData;
}

shared_ptr<ClientTrackData> addAudio(const shared_ptr<PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const string cname, const string msid, const function<void (void)> onOpen) {
    auto audio = Description::Audio(cname);
    audio.addOpusCodec(payloadType);
    audio.addSSRC(ssrc, cname, msid, cname);
    auto track = pc->addTrack(audio);
    // create RTP configuration
    auto rtpConfig = make_shared<RtpPacketizationConfig>(ssrc, cname, payloadType, OpusRtpPacketizer::DefaultClockRate);
    // create packetizer
    auto packetizer = make_shared<OpusRtpPacketizer>(rtpConfig);
    // add RTCP SR handler
    auto srReporter = make_shared<RtcpSrReporter>(rtpConfig);
    packetizer->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = make_shared<RtcpNackResponder>();
    packetizer->addToChain(nackResponder);
    // set handler
    track->setMediaHandler(packetizer);
    track->onOpen(onOpen);
    auto trackData = make_shared<ClientTrackData>(track, srReporter);
    return trackData;
}

// Create and setup a PeerConnection
shared_ptr<Client> createPeerConnection(const Configuration &config,
                                                weak_ptr<WebSocket> wws,
                                                string id) {
    auto pc = make_shared<PeerConnection>(config);
    auto client = make_shared<Client>(pc);

    pc->onStateChange([id](PeerConnection::State state) {
        cout << "State: " << state << endl;
        if (state == PeerConnection::State::Disconnected ||
            state == PeerConnection::State::Failed ||
            state == PeerConnection::State::Closed) {
            // remove disconnected client
            MainThread.dispatch([id]() {
                clients.erase(id);
            });
        }
    });

    pc->onGatheringStateChange(
        [wpc = make_weak_ptr(pc), id, wws](PeerConnection::GatheringState state) {
        cout << "Gathering State: " << state << endl;
        if (state == PeerConnection::GatheringState::Complete) {
            if(auto pc = wpc.lock()) {
                auto description = pc->localDescription();
                json message = {
                    {"id", id},
                    {"type", description->typeString()},
                    {"sdp", string(description.value())}
                };
                // Gathering complete, send answer
                if (auto ws = wws.lock()) {
                    ws->send(message.dump());
                }
            }
        }
    });

    client->video = addVideo(pc, 102, 1, "video-stream", "stream1", [id, wc = make_weak_ptr(client)]() {
        MainThread.dispatch([wc]() {
            if (auto c = wc.lock()) {
                // c->setState(Client::State::Ready);
                addToStream(c, true);
            }
        });
        cout << "Video from " << id << " opened" << endl;
    });

    // client->audio = addAudio(pc, 111, 2, "audio-stream", "stream1", [id, wc = make_weak_ptr(client)]() {
    //     MainThread.dispatch([wc]() {
    //         if (auto c = wc.lock()) {
    //             addToStream(c, false);
    //         }
    //     });
    //     cout << "Audio from " << id << " opened" << endl;
    // });

    auto dc = pc->createDataChannel("ping-pong");
    dc->onOpen([id, wdc = make_weak_ptr(dc)]() {
        if (auto dc = wdc.lock()) {
            dc->send("Ping");
        }
    });

    dc->onMessage(nullptr, [id, wdc = make_weak_ptr(dc)](string msg) {
        cout << "Message from " << id << " received: " << msg << endl;
        if (auto dc = wdc.lock()) {
            dc->send("Ping");
        }
    });
    client->dataChannel = dc;

    pc->setLocalDescription();
    return client;
};

/// Create stream
shared_ptr<Stream> createStream(const string h264Samples, const unsigned fps, const string opusSamples) {
    // video source
    auto video = make_shared<H264FileParser>(h264Samples, fps, true);
    // audio source
    auto audio = make_shared<OPUSFileParser>(opusSamples, true);

    auto stream = make_shared<Stream>(video, audio);
    // set callback responsible for sample sending
    stream->onSample([ws = make_weak_ptr(stream)](Stream::StreamSourceType type, uint64_t sampleTime, rtc::binary sample) {
        printf("Sample Time: %lu\n", sampleTime);
        printf("Sample bytes: ");

        int i = 0;
        for (auto byte: sample) {
            printf("%02x ", byte);
            if (i++ > 10) {
                break;
            }
        }
        printf("\n");

        printf("Sample size in hex: %lx, %lx\n", sample.size(), ntohl(sample.size()));

        vector<ClientTrack> tracks{};
        string streamType = type == Stream::StreamSourceType::Video ? "video" : "audio";
        // get track for given type
        function<optional<shared_ptr<ClientTrackData>> (shared_ptr<Client>)> getTrackData = [type](shared_ptr<Client> client) {
            return type == Stream::StreamSourceType::Video ? client->video : client->audio;
        };
        // get all clients with Ready state
        for(auto id_client: clients) {
            auto id = id_client.first;
            auto client = id_client.second;
            auto optTrackData = getTrackData(client);
            if (client->getState() == Client::State::Ready && optTrackData.has_value()) {
                auto trackData = optTrackData.value();
                tracks.push_back(ClientTrack(id, trackData));
            }
        }
        if (!tracks.empty()) {
            for (auto clientTrack: tracks) {
                auto client = clientTrack.id;
                auto trackData = clientTrack.trackData;

                cout << "Sending " << streamType << " sample with size: " << to_string(sample.size()) << " to " << client << endl;
                try {
                    // send sample
                    trackData->track->sendFrame(sample, std::chrono::duration<double, std::micro>(sampleTime));
                } catch (const std::exception &e) {
                    cerr << "Unable to send "<< streamType << " packet: " << e.what() << endl;
                }
            }
        }
        MainThread.dispatch([ws]() {
            if (clients.empty()) {
                // we have no clients, stop the stream
                if (auto stream = ws.lock()) {
                    stream->stop();
                }
            }
        });
    });
    return stream;
}

/// Start stream
void startStream() {
    shared_ptr<Stream> stream;
    if (avStream.has_value()) {
        stream = avStream.value();
        if (stream->isRunning) {
            // stream is already running
            return;
        }
    } else {
        stream = createStream(h264SamplesDirectory, 30, opusSamplesDirectory);
        avStream = stream;
    }
    stream->start();
}

/// Send previous key frame so browser can show something to user
/// @param stream Stream
/// @param video Video track data
void sendInitialNalus(shared_ptr<Stream> stream, shared_ptr<ClientTrackData> video) {
    auto h264 = dynamic_cast<H264FileParser *>(stream->video.get());
    auto initialNalus = h264->initialNALUS();

    // send previous NALU key frame so users don't have to wait to see stream works
    if (!initialNalus.empty()) {
        const double frameDuration_s = double(h264->getSampleDuration_us()) / (1000 * 1000);
        const uint32_t frameTimestampDuration = video->sender->rtpConfig->secondsToTimestamp(frameDuration_s);
        video->sender->rtpConfig->timestamp = video->sender->rtpConfig->startTimestamp - frameTimestampDuration * 2;
        video->track->send(initialNalus);
        video->sender->rtpConfig->timestamp += frameTimestampDuration;
        // Send initial NAL units again to start stream in firefox browser
        video->track->send(initialNalus);
    }
}

/// Add client to stream
/// @param client Client
/// @param adding_video True if adding video
void addToStream(shared_ptr<Client> client, bool isAddingVideo) {

        // Audio and video tracks are collected now
        assert(client->video.has_value() && client->audio.has_value());
        auto video = client->video.value();

        if (avStream.has_value()) {
            sendInitialNalus(avStream.value(), video);
        }

        client->setState(Client::State::Ready);
    if (client->getState() == Client::State::Ready) {
        startStream();
    }
}
