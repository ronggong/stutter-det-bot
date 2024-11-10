#pragma once
#include "WorkerManager.h"
#include "windows.h"
#include <cstdint>
#include <iostream>
#include <cstdint>
#include "rawdata/rawdata_audio_helper_interface.h"
#include "zoom_sdk.h"
#include "zoom_sdk_raw_data_def.h"
#include <meeting_service_components/meeting_audio_interface.h>
#include <meeting_service_components/meeting_participants_ctrl_interface.h>


using namespace std;
using namespace ZOOMSDK;

class WebSocketSender;

class ZoomSDKAudioRawDataDelegate :
	public IZoomSDKAudioRawDataDelegate
{
public:
	ZoomSDKAudioRawDataDelegate(WebSocketSender* wsSender);
	virtual ~ZoomSDKAudioRawDataDelegate();
	virtual void onMixedAudioRawDataReceived(AudioRawData* data_);
	virtual void onOneWayAudioRawDataReceived(AudioRawData* data_, uint32_t node_id);
	virtual void onShareAudioRawDataReceived(AudioRawData* data_);
	virtual void onOneWayInterpreterAudioRawDataReceived(AudioRawData* data_, const zchar_t* pLanguageName);

	void setParticipantsController(IMeetingParticipantsController* participantsController) { participantsController_ = participantsController; }
	void setUserName(string& userName) { userName_ = userName; }

private:
	std::unique_ptr<WorkerManager> manager_;
	IMeetingParticipantsController* participantsController_;
	string userName_;
};