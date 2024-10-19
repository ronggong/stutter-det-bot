#include "WorkerManager.h"
#include "engine/SileroVad.h"
#include "audio/fbank.h"
#include "engine/Sed.h"
#include <stdexcept>
#include <iostream>
#if __cpp_lib_experimental_filesystem
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING 1;
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

WorkerManager::WorkerManager()
    : running_(false)
    , resampler_(32000, 16000, 1)
{}

WorkerManager::~WorkerManager() {
    stop();
}

void WorkerManager::start() {
    running_ = true;
    audioThread_ = std::thread(&WorkerManager::processAudioData, this);
}

void WorkerManager::stop() {
    running_ = false;
    if (audioThread_.joinable()) audioThread_.join();
}

ThreadSafeQueue<AudioData>& WorkerManager::getAudioQueue() {
    return audioDataQueue_;
}

void WorkerManager::processAudioData() {
    if (!vad_) {
        auto sileroVadPath = fs::current_path() / "models" / "silero_vad.onnx";
        if (!fs::exists(sileroVadPath)) {
            std::cerr << "SileroVad model not found in " << sileroVadPath.string() << std::endl;
            return;
        }

        vad_ = std::make_unique<VadIterator>(sileroVadPath.generic_wstring());
        speechTimestamps_ = vad_->get_speech_timestamps().size();

        fbank_ = std::make_unique<wenet::Fbank>(80, 16000, 400, 160); // 25ms frame, 10ms shift

        speechBuffer_ = { std::queue<float>(), 0, 2 * vad_->get_max_speech_samples() };

        //client_ = std::make_unique<TCPClient>("127.0.0.1", 12345);
    }

    if (!sed_) {
        auto sedPath = fs::current_path() / "models" / "sed.quant.onnx";
        if (!fs::exists(sedPath)) {
            std::cerr << "Sed model not found in " << sedPath.string() << std::endl;
            return;
        }

        sed_ = std::make_unique<Sed>(sedPath.generic_wstring());
    }


    while (running_) {
        AudioData rawData = audioDataQueue_.pop();

        // resample from 32kHz to 16kHz
        std::vector<float> floatData;
        AudioUtil::convertPCM16ToFloat(rawData.data_, floatData);

        size_t resampledSz = size_t(floatData.size() * resampler_.getRatio());
        float* resampledData = new float[resampledSz];
        resampler_.process(floatData.data(), floatData.size(), resampledData, resampledSz);

        // add samples to vad buffer
        // resampleSz=160, vadWindowSize=512
        int64_t vadWindowSize = vad_->get_window_size_samples();
        for (size_t i = 0; i < resampledSz; i++) {
            vadData_.push(resampledData[i]);
        }

        if (vadData_.size() >= vadWindowSize) {
            std::vector<float> vadData;
            for (size_t i = 0; i < vadWindowSize; i++) {
                vadData.push_back(vadData_.front());
                // make sure these are synced with vad data
                wavData_.push_back(vadData_.front() * 32768);
                speechBuffer_.data.push(vadData_.front() * 32768);
                vadData_.pop();
            }
            vad_->predict(vadData);
            // We have a speech segment detected
            if (vad_->get_speech_timestamps().size() > speechTimestamps_) {
                speechTimestamps_ = vad_->get_speech_timestamps().size();
                auto start = vad_->get_speech_timestamps().back().start;
                auto end = vad_->get_speech_timestamps().back().end;
                auto duration = end - start;

                if (speechBuffer_.offset > start) {
                    throw std::runtime_error("Speech buffer start index is greater than the detected speech segment start index");
                }
                auto relStart = start - speechBuffer_.offset;
                if (relStart + duration > speechBuffer_.data.size()) {
                    std::cout << "Speech buffer size " << speechBuffer_.data.size() << " start " << relStart << " duration " << duration << std::endl;
                    throw std::runtime_error("Speech segment duration is greater than the speech buffer size");
                }

                for (size_t i = 0; i < relStart; i++) {
                    speechBuffer_.data.pop();
                }

                // charge speech samples
                std::vector<float> fbankData;
                for (size_t i = 0; i < duration; i++) {
                    fbankData.push_back(speechBuffer_.data.front());
                    speechBuffer_.data.pop();
                }
                std::vector<std::vector<float>> feat;
                fbank_->Compute(fbankData, &feat);
                auto sedProb = sed_->predict(feat);

                speechBuffer_.offset = end;

                std::cout << "Speech segment detected start " << start << " dur " << duration << " fbank frames " << feat.size() << " Sed prob ";
                if (sedProb.size() != sedThreshold_.size()) {
                    throw std::runtime_error("Sed prob size is not equal to the threshold size");
                }

                std::string proba;
                std::string sendLabel;
                for (size_t i = 0; i < sedProb.size(); i++) {
                    proba += std::to_string(sedProb[i]) + " ";
                    if (sedProb[i] > sedThreshold_[i]) {
                        sendLabel += Sed::sed_labels.at(i) + " ";
                    }
                }
                std::cout << proba << std::endl;
                if (!sendLabel.empty()) {
                    sendLabel.pop_back();
                    //client_->sendMessage(sendLabel);
                }

                /*
                auto testWav = fs::current_path() / ("output_" + std::to_string(start) + "_" + std::to_string(end) +  ".wav");
                std::vector<int16_t> wavData(fbankData.size());
                for (size_t i = 0; i < fbankData.size(); i++) {
                    wavData[i] = (int16_t)fbankData[i];
                }
                const int16_t* data = wavData.data();
                writeWAVData(testWav.generic_string().c_str(), data, wavData.size() * 1 * sizeof(data[0]), 16000, 1);
                */
            }
        }

        delete[] resampledData;

        /*
        if (wavData_.size() >= 320000) {
            // write to wav file
            auto testWav = fs::current_path() / "output.wav";
            if (!fs::exists(testWav)) {
                std::vector<int16_t> wavData(wavData_.size());
                for (size_t i = 0; i < wavData_.size(); i++) {
                    wavData[i] = (int16_t)wavData_[i];
                }
                const int16_t* data = wavData.data();
                writeWAVData(testWav.generic_string().c_str(), data, wavData.size() * 1 * sizeof(data[0]), 16000, 1);
                wavData_.clear();
            }
        }
        */

    }
}