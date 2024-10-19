#pragma once
#include "WorkerManager.h"
#include "windows.h"
#include <cstdint>
#include <iostream>
#include <cstdint>
#include "rawdata/rawdata_audio_helper_interface.h"
#include "zoom_sdk.h"
#include "zoom_sdk_raw_data_def.h"


using namespace std;
using namespace ZOOMSDK;

class ZoomSDKAudioRawDataDelegate :
	public IZoomSDKAudioRawDataDelegate
{
public:
	ZoomSDKAudioRawDataDelegate();
	virtual ~ZoomSDKAudioRawDataDelegate();
	virtual void onMixedAudioRawDataReceived(AudioRawData* data_);
	virtual void onOneWayAudioRawDataReceived(AudioRawData* data_, uint32_t node_id);
	virtual void onShareAudioRawDataReceived(AudioRawData* data_);
	virtual void onOneWayInterpreterAudioRawDataReceived(AudioRawData* data_, const zchar_t* pLanguageName);

private:
	std::unique_ptr<WorkerManager> manager_;
	uint32_t node_id_ = 0;
};