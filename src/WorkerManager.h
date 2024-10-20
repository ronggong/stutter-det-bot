#pragma once

#include "ThreadSafeQueue.h"
#include <thread>
#include <atomic>
#include <memory>
#include <queue>
#include "audio/AudioUtil.h"
#include "web/WSSender.h"


class VadIterator;
class Sed;

namespace wenet {
    class Fbank;
}

class WorkerManager {
public:
    WorkerManager();
    ~WorkerManager();

    void start();
    void stop();

    ThreadSafeQueue<AudioData>& getAudioQueue();

private:
	void processAudioData(WebSocketSender* wsSender);

    std::thread audioThread_;
    std::atomic<bool> running_;

    ThreadSafeQueue<AudioData> audioDataQueue_;

    std::unique_ptr<Resampler> resampler_;
    std::unique_ptr<VadIterator> vad_;
    std::queue<float> vadData_;
    std::vector<float> wavData_;
    int speechTimestamps_ = -1;
    std::unique_ptr<wenet::Fbank> fbank_;

    struct SpeechBuffer {
        std::queue<float> data;
        size_t offset; // start index of sample stream
        size_t maxSz; // max size of the buffer
    } speechBuffer_;

    std::unique_ptr<Sed> sed_;
    std::vector<float> sedThreshold_{ 0.42, 0.35, 0.37, 0.37, 0.4 };

    //std::unique_ptr<TCPClient> client_;
	std::unique_ptr<WebSocketSender> wsSender_;
};