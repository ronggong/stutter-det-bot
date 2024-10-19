#include "Sed.h"

const std::unordered_map<size_t, std::string> Sed::sed_labels = {
        {0, "/p"},
        {1, "/b"},
        {2, "/r"},
        {3, "[]"},
        {4, "/i"}
};

void Sed::init_engine_threads(int inter_threads, int intra_threads)
{
    // The method should be called in each thread/proc in multi-thread/proc work
    session_options.SetIntraOpNumThreads(intra_threads);
    session_options.SetInterOpNumThreads(inter_threads);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
};

void Sed::init_onnx_model(const std::wstring& model_path)
{
    // Init threads = 1 for 
    init_engine_threads(1, 1);
    // Load model
    session = std::make_shared<Ort::Session>(env, model_path.c_str(), session_options);
};

std::vector<float> Sed::predict(const std::vector<std::vector<float>>& feat)
{
    // Infer
    // Create ort tensors
    std::vector<float> speech;
    for (size_t i = 0; i < feat.size(); i++) {
        speech.insert(speech.end(), feat[i].begin(), feat[i].end());
    }
    speech_node_dims[1] = int64_t(feat.size());
    std::vector<int32_t> speech_lengths = { int32_t(feat.size()) };
    Ort::Value speech_ort = Ort::Value::CreateTensor<float>(
        memory_info, speech.data(), speech.size(), speech_node_dims, 3);
    Ort::Value speech_lengths_ort = Ort::Value::CreateTensor<int32_t>(
        memory_info, speech_lengths.data(), speech_lengths.size(), speech_lengths_node_dims, 1);

    // Clear and add inputs
    ort_inputs.clear();
    ort_inputs.emplace_back(std::move(speech_ort));
    ort_inputs.emplace_back(std::move(speech_lengths_ort));

    // Infer
    ort_outputs = session->Run(
        Ort::RunOptions{ nullptr },
        input_node_names.data(), ort_inputs.data(), ort_inputs.size(),
        output_node_names.data(), output_node_names.size());

    // Output probability & update h,c recursively
    float* sed_prob = ort_outputs[0].GetTensorMutableData<float>();
    auto type_info = ort_outputs[0].GetTensorTypeAndShapeInfo();

    int output_dim = type_info.GetShape()[1];
    std::vector<float> output(sed_prob, sed_prob + output_dim);
    return output;
};

Sed::Sed(const std::wstring ModelPath)
{
    init_onnx_model(ModelPath);
};