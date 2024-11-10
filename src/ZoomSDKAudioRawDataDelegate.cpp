#include "ZoomSDKAudioRawDataDelegate.h"
#include <cstdint>
#include <iostream>
#include <fstream>
#include "rawdata/rawdata_audio_helper_interface.h"
#include "windows.h"
#include "zoom_sdk_def.h"
#include "Utils.h"



ZoomSDKAudioRawDataDelegate::ZoomSDKAudioRawDataDelegate(WebSocketSender* wsSender)
	: manager_(std::make_unique<WorkerManager>())
{
	manager_->start(wsSender);
}
ZoomSDKAudioRawDataDelegate::~ZoomSDKAudioRawDataDelegate()
{
}
void ZoomSDKAudioRawDataDelegate::onOneWayAudioRawDataReceived(AudioRawData* data_, uint32_t node_id)
{
	if (participantsController_ != nullptr) {
		auto userInfo = participantsController_->GetUserByUserID(node_id);
		if (wcharToString(userInfo->GetUserName()) == userName_) {
			AudioData audioData(data_->GetBuffer(), data_->GetBufferLen(), data_->GetSampleRate(), data_->GetChannelNum());
			manager_->getAudioQueue().push(audioData);
		}
	}
}
void ZoomSDKAudioRawDataDelegate::onShareAudioRawDataReceived(AudioRawData* data_)
{
}
void ZoomSDKAudioRawDataDelegate::onOneWayInterpreterAudioRawDataReceived(AudioRawData* data_, const zchar_t* pLanguageName)
{
}
void ZoomSDKAudioRawDataDelegate::onMixedAudioRawDataReceived(AudioRawData* data_)
{
}