/* ****************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#include <tinyara/config.h>
#include <string.h>
#include "Decoder.h"
#include <debug.h>

#define PV_SUCCESS 0
#define PV_FAILURE -1

namespace media {

Decoder::Decoder()
{
#ifdef CONFIG_AUDIO_CODEC
	memset(&mPlayer, 0, sizeof(pv_player_t));
	if (pv_player_init(&mPlayer, CONFIG_AUDIO_CODEC_RINGBUFFER_SIZE, this, _configFunc, nullptr, nullptr) !=
		PV_SUCCESS) {
		meddbg("Error! pv_player_init failed!\n");
	}
#endif
}

Decoder::Decoder(const Decoder *source)
{
#ifdef CONFIG_AUDIO_CODEC
	mPlayer = source->mPlayer;
#endif
}

Decoder::~Decoder()
{
#ifdef CONFIG_AUDIO_CODEC
	if (pv_player_finish(&mPlayer) != PV_SUCCESS) {
		meddbg("Error! pv_player_finish failed!\n");
	}
#endif
}

size_t Decoder::pushData(unsigned char *buf, size_t size)
{
#ifdef CONFIG_AUDIO_CODEC
	size_t rmax = getAvailSpace();
	if (size > rmax) {
		meddbg("Error!! data is larger than rmax\n");
		size = rmax;
	}

	return pv_player_pushdata(&mPlayer, buf, size);
#endif
	return 0;
}

/**
 * @brief   Get decoded PCM sample frames
 * @remarks
 * @param   buf:  pointer to buffer for output
 * @param   size: input/output parameter, in bytes.
 *                input-size means the capability of 'buf', and also be the requested size for output.
 *                output-size means the real output data size.
 * @param   sampleRate:  Sample rate (Hz) of the output PCM samples
 * @param   channels:  Number of channels of the output PCM samples
 * @return  true  - output some data to 'buf', and output-size in the range of (0, input-size];
 *          false - no output, and need push more audio data for decoding.
 * @see
 */
bool Decoder::getFrame(unsigned char *buf, size_t *size, unsigned int *sampleRate, unsigned short *channels)
{
#ifdef CONFIG_AUDIO_CODEC
	// Record pcm data fetched from audio decoder.
	// Currently, pcm_data_t.samples point to the static buffer configured in Decoder::_configFunc(),
	// so we can keep this pcm_data_t info before next pv_player_frame_decode() called.
	static pcm_data_t pcm = {0};

	// Get input buffer max size (it's also the request size)
	size_t max = *size;

	// Check 2 bytes aligned
	assert((max % 2) == 0);

	// Reset ouput *size 0
	*size = 0;

	/*
	 * Need to get the enough data to parse data format.
	 */
	if (mPlayer.audio_type == AUDIO_TYPE_UNKNOWN) {
		mPlayer.audio_type = _get_audio_type(mPlayer.rbsp);

		if (mPlayer.audio_type) {
			if (_init_decoder(&mPlayer) != 0) {
				meddbg("Error! _init_decoder failed!\n");
				return false;
			}
		}
	}

	if (mPlayer.audio_type == AUDIO_TYPE_UNKNOWN) {
		medvdbg("Decoder: Unknown audio_type\n");
		return false;
	}

	while (1) {
		if (pcm.samples) {
			#define BYTES_PER_SAMPLE sizeof(signed short)
			// Move remained data to decoder output buffer
			size_t nSamples = (max - *size) / BYTES_PER_SAMPLE;
			if (pcm.length  <= nSamples) {
				// Move all data
				memcpy(buf + *size, pcm.samples, pcm.length * BYTES_PER_SAMPLE);
				*size += pcm.length * BYTES_PER_SAMPLE;
				// Clear record
				pcm.samples = nullptr;
				pcm.length = 0;

				if (*size == max) {
					break;
				}
			} else {
				memcpy(buf + *size, pcm.samples, nSamples * BYTES_PER_SAMPLE);
				*size = max;
				// Update remaind data record
				pcm.samples += nSamples;
				pcm.length -= nSamples;
				break;
			}
			#undef BYTES_PER_SAMPLE
		}

		// Decode more
		if (!pv_player_get_frame(&mPlayer)) {
			// Need push more data for further decoding.
			// In this case, *size < max.
			break;
		}

		if (pv_player_frame_decode(&mPlayer, &pcm) == PV_FAILURE) {
			// Decoding failed
			break;
		}
	}

	if (*size == 0) {
		return false;
	}

	*sampleRate = pcm.samplerate;
	*channels = pcm.channels;
	return true;
#endif
	return false;
}

bool Decoder::empty()
{
#ifdef CONFIG_AUDIO_CODEC
	return pv_player_dataspace_is_empty(&mPlayer);
#endif
	return false;
}

size_t Decoder::getAvailSpace()
{
#ifdef CONFIG_AUDIO_CODEC
	return rb_avail(mPlayer.rbsp->rbp);
#endif
	return 0;
}

#ifdef CONFIG_AUDIO_CODEC
int Decoder::_configFunc(void *user_data, int audio_type, void *dec_ext)
{
	/* To-do: Below buffer size and channel count must be calculated correctly. */
	static const int TARGET_SAMPLE_RATE = 16000;
	static const int TARGET_SOUND_TRACK = 2;
	static uint8_t inputBuf[4096];
	static int16_t outputBuf[4096];

	switch (audio_type) {
	case AUDIO_TYPE_MP3: {
		tPVMP3DecoderExternal *mp3_ext = (tPVMP3DecoderExternal *)dec_ext;
		mp3_ext->equalizerType = flat;
		mp3_ext->crcEnabled = false;
		mp3_ext->pInputBuffer = inputBuf;
		mp3_ext->pOutputBuffer = outputBuf;
		mp3_ext->outputFrameSize = sizeof(outputBuf) / sizeof(int16_t);
		return 0;
	}

	case AUDIO_TYPE_AAC: {
		tPVMP4AudioDecoderExternal *aac_ext = (tPVMP4AudioDecoderExternal *)dec_ext;
		aac_ext->outputFormat = OUTPUTFORMAT_16PCM_INTERLEAVED;
		aac_ext->desiredChannels = TARGET_SOUND_TRACK;
		aac_ext->pInputBuffer = inputBuf;
		aac_ext->pOutputBuffer = outputBuf;
		aac_ext->aacPlusEnabled = 1;
		return 0;
	}

#ifdef CONFIG_CODEC_LIBOPUS
	case AUDIO_TYPE_OPUS: {
		opus_dec_external_t *ext = (opus_dec_external_t *)dec_ext;
		ext->pInputBuffer = inputBuf;
		ext->inputBufferMaxLength = sizeof(inputBuf);
		ext->pOutputBuffer = outputBuf;
		ext->outputBufferMaxLength = sizeof(outputBuf);
		ext->desiredSampleRate = TARGET_SAMPLE_RATE;
		ext->desiredChannels = TARGET_SOUND_TRACK;
		return 0;
	}
#endif

	default:
		meddbg("Error! Not supported audio format!\n");
		return -1;
	}
}
#endif

} // namespace media
