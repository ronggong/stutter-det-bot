#pragma once

#include <vector>
#include <string>
#include <cstdio>
#include <limits>
#include <memory>

#include "onnxruntime_cxx_api.h"

class Sed
{
private:
    // OnnxRuntime resources
    Ort::Env env;
    Ort::SessionOptions session_options;
    std::shared_ptr<Ort::Session> session = nullptr;
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU);

private:
    void init_engine_threads(int inter_threads, int intra_threads);
    void init_onnx_model(const std::wstring& model_path);

public:
    std::vector<float> predict(const std::vector<std::vector<float>>& feat);

private:
    // Onnx model
    // Inputs
    std::vector<Ort::Value> ort_inputs;
    std::vector<const char*> input_node_names = { "speech", "speech_lengths" };

    int64_t speech_node_dims[3] = { 1, 0, 80 };
    const int64_t speech_lengths_node_dims[1] = { 1 };

    // Outputs
    std::vector<Ort::Value> ort_outputs;
    std::vector<const char*> output_node_names = { "output" };

public:
    // Construction
    Sed(const std::wstring ModelPath);

    static const std::unordered_map<size_t, std::string> sed_labels;
};