#include <iostream>
#include <cstdint>
#include "AudioUtil.h"

void AudioUtil::convertPCM16ToFloat(const std::vector<char>& pcmData, std::vector<float>& floatData) {
    auto length = pcmData.size();

    // Ensure the length is a multiple of 2 (since each sample is 2 bytes)
    if (length % 2 != 0) {
        std::cerr << "Invalid PCM data length." << std::endl;
        return;
    }

    // Calculate the number of samples
    size_t numSamples = length / 2;

    // Resize the output vector to hold the float samples
    floatData.resize(numSamples);

    // Convert each 16-bit sample to float
    //for (std::size_t i = 0; i < numSamples; ++i) {
    //    // Extract the 16-bit sample (assuming little-endian format)
    //    int16_t sample = static_cast<int16_t>(pcmData[2 * i] | (pcmData[2 * i + 1] << 8));

    //    // Convert the 16-bit sample to float in the range -1.0 to 1.0
    //    floatData[i] = float(sample) / 32768.0f;
    //}
    for (size_t i = 0; i < length; i += 2) {
        int16_t sample = *(int16_t*)(&pcmData[0] + i);
        floatData[i / 2] = sample / 32768.0f;
    }
}

Resampler::Resampler(const double irate, const double orate, unsigned channels)
    : ratio_(orate / irate)
{
    soxr_error_t error;
    soxr_ = soxr_create(irate, orate, channels, &error, NULL, NULL, NULL);
    if (error) {
        std::cerr << "Failed to create resampler: " << soxr_strerror(error) << std::endl;
    }
}

Resampler::~Resampler() {
    soxr_delete(soxr_);
}

void Resampler::process(const float* ibuf, const size_t ilen, float* obuf, size_t& olen) {
    size_t odone;
    soxr_error_t error = soxr_process(soxr_, ibuf, ilen, NULL, obuf, olen, &odone);

    if (error) {
        std::cerr << "Failed to resample process: " << soxr_strerror(error) << std::endl;
    }

    if (odone < olen) {
        olen = odone;
        std::cerr << "Resampler did not process all samples, processed samples " << odone << std::endl;
    }
}
