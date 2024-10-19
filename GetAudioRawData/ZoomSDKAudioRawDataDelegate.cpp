#include "ZoomSDKAudioRawDataDelegate.h"
#include <cstdint>
#include <iostream>
#include <fstream>
#include "rawdata/rawdata_audio_helper_interface.h"
#include "windows.h"
#include "zoom_sdk_def.h" 




using namespace std;
using namespace ZOOM_SDK_NAMESPACE;

ZoomSDKAudioRawDataDelegate::ZoomSDKAudioRawDataDelegate() : manager_(std::make_unique<WorkerManager>())
{
	manager_->start();
}
ZoomSDKAudioRawDataDelegate::~ZoomSDKAudioRawDataDelegate()
{
}
void ZoomSDKAudioRawDataDelegate::onOneWayAudioRawDataReceived(AudioRawData* audioRawData, uint32_t node_id)
{
	//std::cout << "Received onOneWayAudioRawDataReceived" << std::endl;
	//add your code here
}
void ZoomSDKAudioRawDataDelegate::onShareAudioRawDataReceived(AudioRawData* data_)
{
}
void ZoomSDKAudioRawDataDelegate::onOneWayInterpreterAudioRawDataReceived(AudioRawData* data_, const zchar_t* pLanguageName)
{
}
void ZoomSDKAudioRawDataDelegate::onMixedAudioRawDataReceived(AudioRawData* data_)
{
	AudioData audioData(data_->GetBuffer(), data_->GetBufferLen(), data_->GetSampleRate(), data_->GetChannelNum());
	manager_->getAudioQueue().push(audioData);

}