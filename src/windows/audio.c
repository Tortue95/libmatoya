// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

#include <math.h>

#include <windows.h>

#include <initguid.h>
DEFINE_GUID(CLSID_MMDeviceEnumerator,  0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator,   0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient,          0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioRenderClient,    0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);
DEFINE_GUID(IID_IMMNotificationClient, 0x7991EEC9, 0x7E89, 0x4D85, 0x83, 0x90, 0x6C, 0x70, 0x3C, 0xEC, 0x60, 0xC0);

#define COBJMACROS
#include <mmdeviceapi.h>
#include <audioclient.h>

#define AUDIO_CHANNELS    2
#define AUDIO_SAMPLE_SIZE sizeof(int16_t)
#define AUDIO_BUFFER_SIZE ((1 * 1000 * 1000 * 1000) / 100) // 1 second

struct MTY_Audio {
	bool playing;
	bool notification_init;
	uint32_t sample_rate;
	uint32_t min_buffer;
	uint32_t max_buffer;
	UINT32 buffer_size;
	IMMDeviceEnumerator *enumerator;
	IMMNotificationClient notification;
	IAudioClient *client;
	IAudioRenderClient *render;

	bool com;
	DWORD com_thread;
};

static bool AUDIO_DEVICE_CHANGED;


// "Overridden" IMMNotificationClient

static HRESULT STDMETHODCALLTYPE _IMMNotificationClient_QueryInterface(IMMNotificationClient *This,
	REFIID riid, void **ppvObject)
{
	if (IsEqualGUID(&IID_IMMNotificationClient, riid) || IsEqualGUID(&IID_IUnknown, riid)) {
		*ppvObject = This;
		return S_OK;

	} else {
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

static ULONG STDMETHODCALLTYPE _IMMNotificationClient_AddRef(IMMNotificationClient *This)
{
	return 1;
}

static ULONG STDMETHODCALLTYPE _IMMNotificationClient_Release(IMMNotificationClient *This)
{
	return 1;
}

static HRESULT STDMETHODCALLTYPE _IMMNotificationClient_OnDeviceStateChanged(IMMNotificationClient *This,
	LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE _IMMNotificationClient_OnDeviceAdded(IMMNotificationClient *This,
	LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE _IMMNotificationClient_OnDeviceRemoved(IMMNotificationClient *This,
	LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE _IMMNotificationClient_OnDefaultDeviceChanged(IMMNotificationClient *This,
	EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
{
	if (role == eConsole)
		AUDIO_DEVICE_CHANGED = true;

	return S_OK;
}

static HRESULT STDMETHODCALLTYPE _IMMNotificationClient_OnPropertyValueChanged(IMMNotificationClient *This,
	LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
	return S_OK;
}

static CONST_VTBL IMMNotificationClientVtbl _IMMNotificationClient = {
	.QueryInterface         = _IMMNotificationClient_QueryInterface,
	.AddRef                 = _IMMNotificationClient_AddRef,
	.Release                = _IMMNotificationClient_Release,
	.OnDeviceStateChanged   = _IMMNotificationClient_OnDeviceStateChanged,
	.OnDeviceAdded          = _IMMNotificationClient_OnDeviceAdded,
	.OnDeviceRemoved        = _IMMNotificationClient_OnDeviceRemoved,
	.OnDefaultDeviceChanged = _IMMNotificationClient_OnDefaultDeviceChanged,
	.OnPropertyValueChanged = _IMMNotificationClient_OnPropertyValueChanged,
};


// Audio

static void audio_device_destroy(MTY_Audio *ctx)
{
	if (ctx->client && ctx->playing)
		IAudioClient_Stop(ctx->client);

	if (ctx->render)
		IAudioRenderClient_Release(ctx->render);

	if (ctx->client)
		IAudioClient_Release(ctx->client);

	ctx->client = NULL;
	ctx->render = NULL;
	ctx->playing = false;
}

static HRESULT audio_device_create(MTY_Audio *ctx)
{
	IMMDevice *device = NULL;
	HRESULT e = IMMDeviceEnumerator_GetDefaultAudioEndpoint(ctx->enumerator, eRender, eConsole, &device);
	if (e != S_OK) {
		MTY_Log("'IMMDeviceEnumerator_GetDefaultAudioEndpoint' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IMMDevice_Activate(device, &IID_IAudioClient, CLSCTX_ALL, NULL, &ctx->client);
	if (e != S_OK) {
		MTY_Log("'IMMDevice_Activate' failed with HRESULT 0x%X", e);
		goto except;
	}

	WAVEFORMATEX pwfx = {0};
	pwfx.wFormatTag = WAVE_FORMAT_PCM;
	pwfx.nChannels = AUDIO_CHANNELS;
	pwfx.nSamplesPerSec = ctx->sample_rate;
	pwfx.wBitsPerSample = AUDIO_SAMPLE_SIZE * 8;
	pwfx.nBlockAlign = pwfx.nChannels * pwfx.wBitsPerSample / 8;
	pwfx.nAvgBytesPerSec = pwfx.nSamplesPerSec * pwfx.nBlockAlign;

	e = IAudioClient_Initialize(ctx->client, AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
		AUDIO_BUFFER_SIZE, 0, &pwfx, NULL);
	if (e != S_OK) {
		MTY_Log("'IAudioClient_Initialize' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IAudioClient_GetBufferSize(ctx->client, &ctx->buffer_size);
	if (e != S_OK) {
		MTY_Log("'IAudioClient_GetBufferSize' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IAudioClient_GetService(ctx->client, &IID_IAudioRenderClient, &ctx->render);
	if (e != S_OK) {
		MTY_Log("'IAudioClient_GetService' failed with HRESULT 0x%X", e);
		goto except;
	}

	except:

	if (device)
		IMMDevice_Release(device);

	if (e != S_OK)
		audio_device_destroy(ctx);

	return e;
}

MTY_Audio *MTY_AudioCreate(uint32_t sampleRate, uint32_t minBuffer, uint32_t maxBuffer)
{
	MTY_Audio *ctx = MTY_Alloc(1, sizeof(MTY_Audio));
	ctx->sample_rate = sampleRate;

	uint32_t frames_per_ms = lrint((float) sampleRate / 1000.0f);
	ctx->min_buffer = minBuffer * frames_per_ms;
	ctx->max_buffer = maxBuffer * frames_per_ms;

	HRESULT e = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (e != S_FALSE && e != S_OK) {
		MTY_Log("'CoInitializeEx' failed with HRESULT 0x%X", e);
		goto except;
	}

	ctx->com = true;
	ctx->com_thread = GetCurrentThreadId();

	e = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
		&IID_IMMDeviceEnumerator, &ctx->enumerator);
	if (e != S_OK) {
		MTY_Log("'CoCreateInstance' failed with HRESULT 0x%X", e);
		goto except;
	}

	ctx->notification.lpVtbl = &_IMMNotificationClient;
	e = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(ctx->enumerator, &ctx->notification);
	if (e != S_OK) {
		MTY_Log("'IMMDeviceEnumerator_RegisterEndpointNotificationCallback' failed with HRESULT 0x%X", e);
		goto except;
	}
	ctx->notification_init = true;

	e = audio_device_create(ctx);
	if (e != S_OK)
		goto except;

	except:

	if (e != S_OK)
		MTY_AudioDestroy(&ctx);

	return ctx;
}

void MTY_AudioDestroy(MTY_Audio **audio)
{
	if (!audio || !*audio)
		return;

	MTY_Audio *ctx = *audio;

	audio_device_destroy(ctx);

	if (ctx->notification_init)
		IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(ctx->enumerator, &ctx->notification);

	if (ctx->enumerator)
		IMMDeviceEnumerator_Release(ctx->enumerator);

	if (ctx->com) {
		if (GetCurrentThreadId() == ctx->com_thread) {
			CoUninitialize();

		} else {
			MTY_Log("MTY_Audio context should not be destroyed on a "
				"different thread from where it was created");
		}
	}

	MTY_Free(ctx);
	*audio = NULL;
}

static uint32_t audio_get_queued_frames(MTY_Audio *ctx)
{
	if (!ctx->client)
		return ctx->buffer_size;

	UINT32 padding = 0;
	if (IAudioClient_GetCurrentPadding(ctx->client, &padding) == S_OK)
		return padding;

	return ctx->buffer_size;
}

static void audio_play(MTY_Audio *ctx)
{
	if (!ctx->client)
		return;

	if (!ctx->playing) {
		HRESULT e = IAudioClient_Start(ctx->client);
		if (e != S_OK) {
			MTY_Log("'IAudioClient_Start' failed with HRESULT 0x%X", e);
			return;
		}

		ctx->playing = true;
	}
}

void MTY_AudioReset(MTY_Audio *ctx)
{
	if (!ctx->client)
		return;

	if (ctx->playing) {
		HRESULT e = IAudioClient_Stop(ctx->client);
		if (e != S_FALSE && e != S_OK) {
			MTY_Log("'IAudioClient_Stop' failed with HRESULT 0x%X", e);
			return;
		}

		e = IAudioClient_Reset(ctx->client);
		if (e != S_FALSE && e != S_OK) {
			MTY_Log("'IAudioClient_Reset' failed with HRESULT 0x%X", e);
			return;
		}

		ctx->playing = false;
	}
}

static bool audio_handle_device_change(MTY_Audio *ctx)
{
	if (AUDIO_DEVICE_CHANGED) {
		audio_device_destroy(ctx);

		// When the default device is in the process of changing, this may fail
		for (uint8_t x = 0; audio_device_create(ctx) != S_OK && x < 5; x++)
			MTY_Sleep(100);

		if (!ctx->client)
			return false;

		AUDIO_DEVICE_CHANGED = false;
	}

	return true;
}

uint32_t MTY_AudioGetQueued(MTY_Audio *ctx)
{
	return lrint((float) audio_get_queued_frames(ctx) / ((float) ctx->sample_rate / 1000.0f));
}

void MTY_AudioQueue(MTY_Audio *ctx, const int16_t *frames, uint32_t count)
{
	if (!audio_handle_device_change(ctx))
		return;

	uint32_t queued = audio_get_queued_frames(ctx);

	// Stop playing and flush if we've exceeded the maximum buffer or underrun
	if (ctx->playing && (queued > ctx->max_buffer || queued == 0))
		MTY_AudioReset(ctx);

	if (ctx->buffer_size - queued >= count) {
		BYTE *buffer = NULL;
		HRESULT e = IAudioRenderClient_GetBuffer(ctx->render, count, &buffer);

		if (e == S_OK) {
			memcpy(buffer, frames, count * AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE);
			IAudioRenderClient_ReleaseBuffer(ctx->render, count, 0);
		}

		// Begin playing again when the minimum buffer has been reached
		if (!ctx->playing && queued + count >= ctx->min_buffer)
			audio_play(ctx);
	}
}
