#include "WorkerManager.h"
//#include "engine/SileroVad.h"
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
    : running_(false) {}

WorkerManager::~WorkerManager() {
    stop();
}

void WorkerManager::start(WebSocketSender* wsSender) {
    running_ = true;
	audioThread_ = std::thread(&WorkerManager::processAudioData, this, wsSender);
}

void WorkerManager::stop() {
    running_ = false;
    if (audioThread_.joinable()) audioThread_.join();
	//wsSender_->join();
}

ThreadSafeQueue<AudioData>& WorkerManager::getAudioQueue() {
    return audioDataQueue_;
}

void WorkerManager::processAudioData(WebSocketSender* wsSender) {

    if (!sed_) {
        resampler_ = std::make_unique<Resampler>(32000, 16000, 1);

        speechBuffer_ = { std::queue<float>(), 0, 16000 * 5 };

        fbank_ = std::make_unique<wenet::Fbank>(80, 16000, 400, 160); // 25ms frame, 10ms shift

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

        size_t resampledSz = size_t(floatData.size() * resampler_->getRatio());
        float* resampledData = new float[resampledSz];
        resampler_->process(floatData.data(), floatData.size(), resampledData, resampledSz);

		for (size_t i = 0; i < resampledSz; i++) {
			speechBuffer_.data.push(resampledData[i] * 32768);
			//wavData_.push_back(resampledData[i] * 32768);
		}

		if (speechBuffer_.data.size() >= speechBuffer_.maxSz) {
            // charge speech samples
            std::vector<float> fbankData;
            for (size_t i = 0; i < speechBuffer_.maxSz; i++) {
                fbankData.push_back(speechBuffer_.data.front());
                speechBuffer_.data.pop();
            }
            std::vector<std::vector<float>> feat;
            fbank_->Compute(fbankData, &feat);
            auto sedProb = sed_->predict(feat);

            std::cout << "Segment start " << speechBuffer_.offset << " end " <<
                speechBuffer_.offset + speechBuffer_.maxSz <<
                " fbank frames " << feat.size() << " Sed prob (non stuttering, stuttering) ";

            speechBuffer_.offset += speechBuffer_.maxSz;

            if (sedProb.size() != 2) {
                throw std::runtime_error("Sed prob size is not 2");
            }

            std::string proba;
            for (size_t i = 0; i < sedProb.size(); i++) {
                proba += std::to_string(sedProb[i]) + " ";
            }
            std::cout << proba << std::endl;
            if (sedProb[1] > sedProb[0]) {
                std::string sedLabel = "Stuttering detected.";
				std::cout << sedLabel << std::endl;
                wsSender->sendMessage(sedLabel);
            }

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
				std::cout << "Wav file written to " << testWav.string() << std::endl;
            }
        }
        */
    }
}