#pragma once

#include <vector>
#include <algorithm>
#include <soxr.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fstream>
#include <iostream>


struct AudioData
{
    std::vector<char> data_;
    unsigned int sampleRate_;
    unsigned int channels_;

    AudioData(const char* data, unsigned int length, unsigned int sampleRate, unsigned int channels)
        : sampleRate_(sampleRate), channels_(channels)
    {
        data_.resize(length);
        std::copy(data, data + length, data_.begin());
    }
};

class AudioUtil
{
public:
    static void convertPCM16ToFloat(const std::vector<char>& pcmData, std::vector<float>& floatData);
};

class Resampler
{
public:
    Resampler(const double irate, const double orate, unsigned channels);
    ~Resampler();

    void process(const float* ibuf, const size_t ilen, float* obuf, size_t& olen);

    double getRatio() const { return ratio_; }

private:
    soxr_t soxr_;
    double ratio_;
};

namespace wav {

    struct WavHeader {
        char riff[4];  // "riff"
        unsigned int size;
        char wav[4];  // "WAVE"
        char fmt[4];  // "fmt "
        unsigned int fmt_size;
        uint16_t format;
        uint16_t channels;
        unsigned int sample_rate;
        unsigned int bytes_per_second;
        uint16_t block_size;
        uint16_t bit;
        char data[4];  // "data"
        unsigned int data_size;
    };

    class WavReader {
    public:
        WavReader() : data_(nullptr) {}
        explicit WavReader(const std::string& filename) { Open(filename); }

        bool Open(const std::string& filename) {
            FILE* fp = fopen(filename.c_str(), "rb"); //文件读取
            if (NULL == fp) {
                std::cout << "Error in read " << filename;
                return false;
            }

            WavHeader header;
            fread(&header, 1, sizeof(header), fp);
            if (header.fmt_size < 16) {
                printf("WaveData: expect PCM format data "
                    "to have fmt chunk of at least size 16.\n");
                return false;
            }
            else if (header.fmt_size > 16) {
                int offset = 44 - 8 + header.fmt_size - 16;
                fseek(fp, offset, SEEK_SET);
                fread(header.data, 8, sizeof(char), fp);
            }
            // check "riff" "WAVE" "fmt " "data"

            // Skip any sub-chunks between "fmt" and "data".  Usually there will
            // be a single "fact" sub chunk, but on Windows there can also be a
            // "list" sub chunk.
            while (0 != strncmp(header.data, "data", 4)) {
                // We will just ignore the data in these chunks.
                fseek(fp, header.data_size, SEEK_CUR);
                // read next sub chunk
                fread(header.data, 8, sizeof(char), fp);
            }

            if (header.data_size == 0) {
                int offset = ftell(fp);
                fseek(fp, 0, SEEK_END);
                header.data_size = ftell(fp) - offset;
                fseek(fp, offset, SEEK_SET);
            }

            num_channel_ = header.channels;
            sample_rate_ = header.sample_rate;
            bits_per_sample_ = header.bit;
            int num_data = header.data_size / (bits_per_sample_ / 8);
            data_ = new float[num_data]; // Create 1-dim array
            num_samples_ = num_data / num_channel_;

            std::cout << "num_channel_    :" << num_channel_ << std::endl;
            std::cout << "sample_rate_    :" << sample_rate_ << std::endl;
            std::cout << "bits_per_sample_:" << bits_per_sample_ << std::endl;
            std::cout << "num_samples     :" << num_data << std::endl;
            std::cout << "num_data_size   :" << header.data_size << std::endl;

            switch (bits_per_sample_) {
            case 8: {
                char sample;
                for (int i = 0; i < num_data; ++i) {
                    fread(&sample, 1, sizeof(char), fp);
                    data_[i] = static_cast<float>(sample) / 32768;
                }
                break;
            }
            case 16: {
                int16_t sample;
                for (int i = 0; i < num_data; ++i) {
                    fread(&sample, 1, sizeof(int16_t), fp);
                    data_[i] = static_cast<float>(sample) / 32768;
                }
                break;
            }
            case 32:
            {
                if (header.format == 1) //S32
                {
                    int sample;
                    for (int i = 0; i < num_data; ++i) {
                        fread(&sample, 1, sizeof(int), fp);
                        data_[i] = static_cast<float>(sample) / 32768;
                    }
                }
                else if (header.format == 3) // IEEE-float
                {
                    float sample;
                    for (int i = 0; i < num_data; ++i) {
                        fread(&sample, 1, sizeof(float), fp);
                        data_[i] = static_cast<float>(sample);
                    }
                }
                else {
                    printf("unsupported quantization bits\n");
                }
                break;
            }
            default:
                printf("unsupported quantization bits\n");
                break;
            }

            fclose(fp);
            return true;
        }

        int num_channel() const { return num_channel_; }
        int sample_rate() const { return sample_rate_; }
        int bits_per_sample() const { return bits_per_sample_; }
        int num_samples() const { return num_samples_; }

        ~WavReader() {
            delete[] data_;
        }

        const float* data() const { return data_; }

    private:
        int num_channel_;
        int sample_rate_;
        int bits_per_sample_;
        int num_samples_;  // sample points per channel
        float* data_;
    };

    class WavWriter {
    public:
        WavWriter(const float* data, int num_samples, int num_channel,
            int sample_rate, int bits_per_sample)
            : data_(data),
            num_samples_(num_samples),
            num_channel_(num_channel),
            sample_rate_(sample_rate),
            bits_per_sample_(bits_per_sample) {}

        void Write(const std::string& filename) {
            // On Windows we need to use wb mode!!
            FILE* fp = fopen(filename.c_str(), "wb");
            // init char 'riff' 'WAVE' 'fmt ' 'data'
            WavHeader header;
            char wav_header[44] = { 0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57,
                                   0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00,
                                   0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00 };
            memcpy(&header, wav_header, sizeof(header));
            header.channels = num_channel_;
            header.bit = bits_per_sample_;
            header.sample_rate = sample_rate_;
            header.data_size = num_samples_ * num_channel_ * (bits_per_sample_ / 8);
            header.size = sizeof(header) - 8 + header.data_size;
            header.bytes_per_second =
                sample_rate_ * num_channel_ * (bits_per_sample_ / 8);
            header.block_size = num_channel_ * (bits_per_sample_ / 8);

            fwrite(&header, 1, sizeof(header), fp);

            for (int i = 0; i < num_samples_; ++i) {
                for (int j = 0; j < num_channel_; ++j) {
                    switch (bits_per_sample_) {
                    case 8: {
                        char sample = static_cast<char>(data_[i * num_channel_ + j]);
                        fwrite(&sample, 1, sizeof(sample), fp);
                        break;
                    }
                    case 16: {
                        int16_t sample = static_cast<int16_t>(data_[i * num_channel_ + j]);
                        fwrite(&sample, 1, sizeof(sample), fp);
                        break;
                    }
                    case 32: {
                        int sample = static_cast<int>(data_[i * num_channel_ + j]);
                        fwrite(&sample, 1, sizeof(sample), fp);
                        break;
                    }
                    }
                }
            }
            fclose(fp);
        }

    private:
        const float* data_;
        int num_samples_;  // total float points in data_
        int num_channel_;
        int sample_rate_;
        int bits_per_sample_;
    };
};

template <typename T>
void write(std::ofstream& stream, const T& t) {
    stream.write((const char*)&t, sizeof(T));
}

template <typename SampleType>
void writeWAVData(const char* outFile, SampleType* buf, size_t bufSize, int sampleRate, short channels)
{
    std::ofstream stream(outFile, std::ios::binary);                // Open file stream at "outFile" location

    /* Header */
    stream.write("RIFF", 4);                                        // sGroupID (RIFF = Resource Interchange File Format)
    write<int>(stream, 36 + bufSize);                               // dwFileLength
    stream.write("WAVE", 4);                                        // sRiffType

    /* Format Chunk */
    stream.write("fmt ", 4);                                        // sGroupID (fmt = format)
    write<int>(stream, 16);                                         // Chunk size (of Format Chunk)
    write<short>(stream, 1);                                        // Format (1 = PCM)
    write<short>(stream, channels);                                 // Channels
    write<int>(stream, sampleRate);                                 // Sample Rate
    write<int>(stream, sampleRate * channels * sizeof(SampleType)); // Byterate
    write<short>(stream, channels * sizeof(SampleType));            // Frame size aka Block align
    write<short>(stream, 8 * sizeof(SampleType));                   // Bits per sample

    /* Data Chunk */
    stream.write("data", 4);                                        // sGroupID (data)
    stream.write((const char*)&bufSize, 4);                         // Chunk size (of Data, and thus of bufferSize)
    stream.write((const char*)buf, bufSize);                        // The samples DATA!!!
}