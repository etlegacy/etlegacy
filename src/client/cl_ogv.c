/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * Copyright (C) 2008 Stefan Langer <raute@users.sourceforge.net>
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file cl_ogv.c
 * @brief Theora decoding
 */

#include "client.h"

#if defined(FEATURE_THEORA)

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include <theora/theora.h>

#define OGG_BUFFER_SIZE 8 * 1024  //4096
#define OGG_SAMPLEWIDTH 2

#define SIZEOF_RAWBUFF 4 * 1024
#define MIN_AUDIO_PRELOAD 400   // in ms
#define MAX_AUDIO_PRELOAD 500   // in ms

typedef struct
{
	ogg_sync_state oy;          /* sync and verify incoming physical bitstream */
	ogg_stream_state os_audio;
	ogg_stream_state os_video;

	vorbis_dsp_state vd;        /* central working state for the packet->PCM decoder */
	vorbis_info vi;             /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment vc;          /* struct that stores all the bitstream user comments */

	theora_info th_info;        // dump_video.c(example decoder): ti
	theora_comment th_comment;  // dump_video.c(example decoder): tc
	theora_state th_state;      // dump_video.c(example decoder): td

	yuv_buffer th_yuvbuffer;

	ogg_int64_t VFrameCount;        // output video-stream
	ogg_int64_t Vtime_unit;
	int currentTime;                // input from Run-function

	byte audioBuffer[SIZEOF_RAWBUFF];            // audio buffer
} cin_ogv_t;

#define g_ogm ((cin_ogv_t *) cin->data)

/**
 * @brief OGV_LoadBlockToSync
 * @param[in,out] cin
 * @return !0 -> no data transferred
 */
static int OGV_LoadBlockToSync(cinematic_t *cin)
{
	int  r = -1;
	char *buffer;
	int  bytes;

	if (cin->file)
	{
		buffer       = ogg_sync_buffer(&g_ogm->oy, OGG_BUFFER_SIZE);
		bytes        = FS_Read(buffer, OGG_BUFFER_SIZE, cin->file);
		cin->offset += OGG_BUFFER_SIZE;
		ogg_sync_wrote(&g_ogm->oy, bytes);

		r = (bytes == 0);
	}

	return r;
}

/**
 * @brief OGV_LoadPagesToStreams
 * @param cin - unused
 * @return !0 -> no data transferred (or not for all Streams)
 */
static int OGV_LoadPagesToStreams(cinematic_t *cin)
{
	int              r          = -1;
	int              AudioPages = 0;
	int              VideoPages = 0;
	ogg_stream_state *osptr     = NULL;
	ogg_page         og;

	while (!AudioPages || !VideoPages)
	{
		if (ogg_sync_pageout(&g_ogm->oy, &og) != 1)
		{
			break;
		}

		if (g_ogm->os_audio.serialno == ogg_page_serialno(&og))
		{
			osptr = &g_ogm->os_audio;
			++AudioPages;
		}
		if (g_ogm->os_video.serialno == ogg_page_serialno(&og))
		{
			osptr = &g_ogm->os_video;
			++VideoPages;
		}

		if (osptr != NULL)
		{
			ogg_stream_pagein(osptr, &og);
		}
	}

	if (AudioPages && VideoPages)
	{
		r = 0;
	}

	return r;
}

/**
 * @brief OGV_LoadAudio
 * @param[in] cin
 * @return audio wants more packets ?
 */
static qboolean OGV_LoadAudio(cinematic_t *cin)
{
	qboolean     anyDataTransferred = qtrue;
	float        **pcm;
	int          frames, frameNeeded;
	int          i, j;
	short        *ptr;
	ogg_packet   op;
	vorbis_block vb;

	Com_Memset(&op, 0, sizeof(op));
	Com_Memset(&vb, 0, sizeof(vb));
	vorbis_block_init(&g_ogm->vd, &vb);

	while (anyDataTransferred && g_ogm->currentTime + MAX_AUDIO_PRELOAD > (int)(g_ogm->vd.granulepos * 1000 / g_ogm->vi.rate))
	{
		anyDataTransferred = qfalse;

		if ((frames = vorbis_synthesis_pcmout(&g_ogm->vd, &pcm)) > 0)
		{
			// vorbis -> raw
			ptr = (short *)g_ogm->audioBuffer;

			frameNeeded = (SIZEOF_RAWBUFF) / (OGG_SAMPLEWIDTH * g_ogm->vi.channels);

			if (frames < frameNeeded)
			{
				frameNeeded = frames;
			}

			for (i = 0; i < frameNeeded; i++)
			{
				for (j = 0; j < g_ogm->vi.channels; j++)
				{
					*(ptr++) = (short)((pcm[j][i] >= -1.0f && pcm[j][i] <= 1.0f) ? pcm[j][i] * 32767.f : 32767 * ((pcm[j][i] > 0.0f) - (pcm[j][i] < 0.0f)));
				}
			}

			// tell libvorbis how many samples we actually consumed (we ate them all!)
			vorbis_synthesis_read(&g_ogm->vd, frameNeeded);

			if (!(cin->flags & CIN_silent))
			{
				S_RawSamples(0, frameNeeded, g_ogm->vi.rate, OGG_SAMPLEWIDTH, g_ogm->vi.channels, g_ogm->audioBuffer, 1.0f, 1.0f);
			}

			anyDataTransferred = qtrue;
		}

		if (!anyDataTransferred)
		{
			// op -> vorbis
			if (ogg_stream_packetout(&g_ogm->os_audio, &op))
			{
				if (vorbis_synthesis(&vb, &op) == 0)
				{
					vorbis_synthesis_blockin(&g_ogm->vd, &vb);
				}
				anyDataTransferred = qtrue;
			}
		}
	}

	vorbis_block_clear(&vb);

	return (qboolean)(g_ogm->currentTime + MIN_AUDIO_PRELOAD > (int)(g_ogm->vd.granulepos * 1000 / g_ogm->vi.rate));
}

/*
 * @brief OGV_FindSizeShift
 * @param[in] x
 * @param[in] y
 * @return
 *
 * @note Unused
static int OGV_FindSizeShift(int x, int y)
{
    int             i;

    for(i = 0; (y >> i); ++i)
    {
        if(x == (y >> i))
        {
            return i;
        }
    }

    return -1;
}
*/

/*
 * @brief OGV_CheckFrame
 * @param[in] yuv
 * @param[in] info
 * @return
 *
 * @note Unused
static qboolean OGV_CheckFrame(yuv_buffer *yuv, theora_info *info)
{
    int yWShift, uvWShift;
    int yHShift, uvHShift;

    yWShift = OGV_FindSizeShift(yuv->y_width, info->width);
    uvWShift = OGV_FindSizeShift(yuv->uv_width, info->width);
    yHShift = OGV_FindSizeShift(yuv->y_height, info->height);
    uvHShift = OGV_FindSizeShift(yuv->uv_height, info->height);

    if (yWShift < 0 || uvWShift < 0 || yHShift < 0 || uvHShift < 0)
    {
        Com_Error(ERR_DROP, "Theora unexpected resolution in a yuv-Frame\n");
        return qfalse;
    }

    return qtrue;
}
*/

/**
 * @brief OGV_yuv_to_rgb24
 * @param[in] yuv
 * @param[in] info
 * @param[out] output
 * @return
 */
static qboolean OGV_yuv_to_rgb24(yuv_buffer *yuv, theora_info *info, uint32_t *output)
{
	int i, j;
	int uv_ki = yuv->y_width / yuv->uv_width;
	int uv_kj = yuv->y_height / yuv->uv_height;

	int y_offset  = info->offset_x + yuv->y_stride * info->offset_y;
	int uv_offset = info->offset_x / uv_ki + yuv->uv_stride * info->offset_y / uv_kj;
	int y_p, uv_p;

	/*
	if(!OGV_CheckFrame(yuv, info))
	{
	    return qfalse;
	}
	*/

	//FIXME: this is slow as fuck, might want to rewrite and use precalculated tables
	for (j = 0; j < info->height; j++)
	{
		y_p  = y_offset + j * yuv->y_stride;
		uv_p = uv_offset + j / uv_kj * yuv->uv_stride;

		for (i = 0; i < info->width; i++)
		{
			//http://en.wikipedia.org/wiki/YUV
			int y = yuv->y[y_p] - 16;
			int u = yuv->u[uv_p] - 128;
			int v = yuv->v[uv_p] - 128;
			int r = Com_ByteClamp((y * 298 + 409 * v + 128) >> 8);
			int g = Com_ByteClamp((y * 298 - 100 * v - 208 * u + 128) >> 8);
			int b = Com_ByteClamp((y * 298 + 516 * u + 128) >> 8);

			*output = (uint32_t)LittleLong((r) | (g << 8) | (b << 16) | (255 << 24));
			++output;

			y_p++;
			if (i % uv_ki == uv_ki - 1)
			{
				uv_p++;
			}
		}
	}

	return qtrue;
}

/**
 * @brief OGV_NextNeededVFrame
 * @param cin - unused
 * @return
 */
static int OGV_NextNeededVFrame(cinematic_t *cin)
{
	return (int)(g_ogm->currentTime * (ogg_int64_t) 10000 / g_ogm->Vtime_unit);
}

/**
 * @brief OGV_LoadVideoFrame
 * @param[in,out] cin
 * @return
 */
static int OGV_LoadVideoFrame(cinematic_t *cin)
{
	int        r = 0;
	ogg_packet op;

	Com_Memset(&op, 0, sizeof(op));

	while (!r && (ogg_stream_packetout(&g_ogm->os_video, &op)))
	{
		ogg_int64_t th_frame;

		theora_decode_packetin(&g_ogm->th_state, &op);

		th_frame = theora_granule_frame(&g_ogm->th_state, g_ogm->th_state.granulepos);

		if ((g_ogm->VFrameCount < th_frame && th_frame >= OGV_NextNeededVFrame(cin)) || !cin->frameBuffer[0])
		{
			if (theora_decode_YUVout(&g_ogm->th_state, &g_ogm->th_yuvbuffer))
			{
				continue;
			}

			if (cin->frameWidth != g_ogm->th_info.width || cin->frameHeight != g_ogm->th_info.height)
			{
				cin->frameWidth  = g_ogm->th_info.width;
				cin->frameHeight = g_ogm->th_info.height;
				Com_DPrintf("Theora new resolution %dx%d\n", cin->frameWidth, cin->frameHeight);
			}

			if (cin->frameBufferSize < g_ogm->th_info.width * g_ogm->th_info.height)
			{

				cin->frameBufferSize = g_ogm->th_info.width * g_ogm->th_info.height;

				/* Free old output buffer */
				if (cin->frameBuffer[0])
				{
					Com_Dealloc(cin->frameBuffer[0]);
					cin->frameBuffer[0] = NULL;
				}

				/* Allocate the new buffer */
				cin->frameBuffer[0] = (unsigned char *)Com_Allocate(cin->frameBufferSize * 4);
				if (cin->frameBuffer[0] == NULL)
				{
					cin->frameBufferSize = 0;
					r                    = -2;
					break;
				}
			}

			if (OGV_yuv_to_rgb24(&g_ogm->th_yuvbuffer, &g_ogm->th_info, (unsigned int *) cin->frameBuffer[0]))
			{
				r                  = 1;
				g_ogm->VFrameCount = th_frame;
			}
			else
			{
				r = -1;
			}
		}
	}

	return r;
}

/**
 * @brief OGV_LoadFrame
 * @param[in] cin
 * @return
 */
static qboolean OGV_LoadFrame(cinematic_t *cin)
{
	qboolean anyDataTransferred = qtrue;
	qboolean needVOutputData    = qtrue;
	qboolean audioWantsMoreData = qfalse;
	int      status;

	while (anyDataTransferred && (needVOutputData || audioWantsMoreData))
	{
		anyDataTransferred = qfalse;
		{
			if (needVOutputData && (status = OGV_LoadVideoFrame(cin)))
			{
				needVOutputData = qfalse;
				if (status > 0)
				{
					anyDataTransferred = qtrue;
				}
				else
				{
					// error (we don't need any videodata and we had no transferred)
					anyDataTransferred = qfalse;
				}
			}

			if (needVOutputData || audioWantsMoreData)
			{
				// try to transfer Pages to the audio- and video-Stream
				if (OGV_LoadPagesToStreams(cin))
				{
					// try to load a datablock from file
					anyDataTransferred |= !OGV_LoadBlockToSync(cin);
				}
				else
				{
					// successful OGV_LoadPagesToStreams()
					anyDataTransferred = qtrue;
				}
			}

			// load all Audio after loading new pages ...
			if (g_ogm->VFrameCount > 1)  // wait some videoframes (it's better to have some delay, than a lagy sound)
			{
				audioWantsMoreData = OGV_LoadAudio(cin);
			}
		}
	}

	return (qboolean) !!anyDataTransferred;
}

/**
 * @brief OGV_UpdateCinematic
 * @param[in,out] cin
 * @param[in] time
 */
void OGV_UpdateCinematic(cinematic_t *cin, int time)
{
	if (!cin->startTime)
	{
		cin->startTime = time;
	}

	g_ogm->currentTime = time - cin->startTime;

	time = time - cin->startTime + 20;

	cin->currentData.dirty = qfalse;

	while (!g_ogm->VFrameCount || time >= (int)(g_ogm->VFrameCount * g_ogm->Vtime_unit / 10000))
	{
		cin->currentData.dirty = qtrue;
		if (!OGV_LoadFrame(cin))
		{
			// EOF reached
			cin->currentData.image = NULL;
			return;
		}
	}

	cin->currentData.image  = cin->frameBuffer[0];
	cin->currentData.height = cin->frameHeight;
	cin->currentData.width  = cin->frameWidth;

	return;
}

/**
 * @brief OGV_StartRead
 * @param[in,out] cin
 * @return
 */
qboolean OGV_StartRead(cinematic_t *cin)
{
	int        status;
	ogg_page   og;
	ogg_packet op;
	int        i;

	cin->data = Com_Allocate(sizeof(cin_ogv_t));
	Com_Memset(cin->data, 0, sizeof(cin_ogv_t));

	ogg_sync_init(&g_ogm->oy);  /* Now we can read pages */

	//FIXME? can serialno be 0 in ogg? (better way to check inited?)
	//TODO: support for more than one audio stream? / detect files with one stream(or without correct ones)
	while (!g_ogm->os_audio.serialno || !g_ogm->os_video.serialno)
	{
		if (ogg_sync_pageout(&g_ogm->oy, &og) == 1)
		{
			if (strstr((char *)(og.body + 1), "vorbis"))
			{
				//FIXME? better way to find audio stream
				if (g_ogm->os_audio.serialno)
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: more than one audio stream, in ogm-file(%s) ... we will stay at the first one\n", cin->name);
				}
				else
				{
					ogg_stream_init(&g_ogm->os_audio, ogg_page_serialno(&og));
					ogg_stream_pagein(&g_ogm->os_audio, &og);
				}
			}
			if (strstr((char *)(og.body + 1), "theora"))
			{
				if (g_ogm->os_video.serialno)
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: more than one video stream, in ogm-file(%s) ... we will stay at the first one\n", cin->name);
				}
				else
				{
					ogg_stream_init(&g_ogm->os_video, ogg_page_serialno(&og));
					ogg_stream_pagein(&g_ogm->os_video, &og);
				}
			}
		}
		else if (OGV_LoadBlockToSync(cin))
		{
			break;
		}
	}

	if (!g_ogm->os_audio.serialno)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Haven't found a audio(vorbis) stream in ogm-file (%s)\n", cin->name);
		return qfalse;
	}

	if (!g_ogm->os_video.serialno)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Haven't found a video stream in ogm-file (%s)\n", cin->name);
		return qfalse;
	}

	//load vorbis header
	vorbis_info_init(&g_ogm->vi);
	vorbis_comment_init(&g_ogm->vc);
	i = 0;
	while (i < 3)
	{
		status = ogg_stream_packetout(&g_ogm->os_audio, &op);
		if (status < 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Corrupt ogg packet while loading vorbis-headers, ogm-file(%s)\n", cin->name);
			return qfalse;
		}
		if (status > 0)
		{
			status = vorbis_synthesis_headerin(&g_ogm->vi, &g_ogm->vc, &op);
			if (i == 0 && status < 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: This Ogg bitstream does not contain Vorbis audio data, ogm-file(%s)\n", cin->name);
				return qfalse;
			}
			++i;
		}
		else if (OGV_LoadPagesToStreams(cin))
		{
			if (OGV_LoadBlockToSync(cin))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: Couldn't find all vorbis headers before end of ogm-file (%s)\n", cin->name);
				return qfalse;
			}
		}
	}

	vorbis_synthesis_init(&g_ogm->vd, &g_ogm->vi);

	// Do init
	{
		theora_info_init(&g_ogm->th_info);
		theora_comment_init(&g_ogm->th_comment);

		i = 0;
		while (i < 3)
		{
			status = ogg_stream_packetout(&g_ogm->os_video, &op);
			if (status < 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: Corrupt ogg packet while loading theora-headers, ogm-file(%s)\n", cin->name);
				return qfalse;
			}
			else if (status > 0)
			{
				status = theora_decode_header(&g_ogm->th_info, &g_ogm->th_comment, &op);
				if (i == 0 && status != 0)
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: This Ogg bitstream does not contain theora data, ogm-file(%s)\n", cin->name);
					return qfalse;
				}
				++i;
			}
			else if (OGV_LoadPagesToStreams(cin))
			{
				if (OGV_LoadBlockToSync(cin))
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: Couldn't find all theora headers before end of ogm-file (%s)\n", cin->name);
					return qfalse;
				}
			}
		}

		theora_decode_init(&g_ogm->th_state, &g_ogm->th_info);

		g_ogm->Vtime_unit = ((ogg_int64_t) g_ogm->th_info.fps_denominator * 1000 * 10000 / g_ogm->th_info.fps_numerator);
	}

	Com_DPrintf("Theora init done (%s)\n", cin->name);
	return qtrue;
}

/**
 * @brief OGV_StopVideo
 * @param[in,out] cin
 */
void OGV_StopVideo(cinematic_t *cin)
{
	if (cin->data)
	{
		theora_clear(&g_ogm->th_state);
		theora_comment_clear(&g_ogm->th_comment);
		theora_info_clear(&g_ogm->th_info);

		vorbis_dsp_clear(&g_ogm->vd);
		vorbis_comment_clear(&g_ogm->vc);
		vorbis_info_clear(&g_ogm->vi);  /* must be called last (comment from vorbis example code) */

		ogg_stream_clear(&g_ogm->os_audio);
		ogg_stream_clear(&g_ogm->os_video);

		ogg_sync_clear(&g_ogm->oy);

		Com_Dealloc(cin->data);
		cin->data = NULL;
	}
}
#endif
