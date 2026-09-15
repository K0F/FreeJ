/*  FreeJ
 *  (c) Copyright 2001 Silvano Galliani aka kysucix <silvano.galliani@poste.it>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __avcodec_h__
#define __avcodec_h__

#include <config.h>
#include <inttypes.h>

extern "C" {
#ifdef HAVE_LIBAVCODEC_AVCODEC_H
#   include <libavcodec/avcodec.h>
#elif defined(HAVE_FFMPEG_AVCODEC_H)
#   include <ffmpeg/avcodec.h>
#else
#   include <avcodec.h>
#endif

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
#   include <libavformat/avformat.h>
#elif defined(HAVE_FFMPEG_AVFORMAT_H)
#   include <ffmpeg/avformat.h>
#else
#   include <avformat.h>
#endif

#ifdef WITH_SWSCALE
#ifdef HAVE_LIBSWSCALE_SWSCALE_H
#   include <libswscale/swscale.h>
#elif defined(HAVE_FFMPEG_SWSCALE_H)
#   include <ffmpeg/swscale.h>
#else
#   include <swscale.h>
#endif
#endif

#   include <libavutil/imgutils.h>
#   include <libavutil/opt.h>
}
#include <layer.h>
#define INBUF_SIZE 4096
#define NO_MARK -1
#define FIFO_SIZE 2

#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000
#endif

#include <callback.h>

#include <factory.h>

class VideoLayer: public Layer {

    public:
	VideoLayer();
	~VideoLayer();


	bool open(const char *file);
	void *feed();
	void close();

	int64_t to_seek;

	bool relative_seek(double increment);

	bool set_mark_in();
	bool set_mark_out();
	void pause();

	// quick hack for EOS callback
	bool add_eos_call(DumbCall *c) { return eos->add_call(c); }
	bool rem_eos_call(DumbCall *c) { return eos->rem_call(c); }

	bool use_audio;

	int audio_channels;
	int audio_samplerate;

 protected:
	bool _init();

    private:
	AVCodec *avcodec;
	const AVInputFormat *fmt;
	AVFormatContext *avformat_context;
	AVStream *avformat_stream;
	AVFrame *rgba_frame;
	AVPacket pkt;


	AVCodecContext *video_codec_ctx;
	AVCodec *video_codec;

	AVCodecContext *audio_codec_ctx;
	AVCodec *audio_codec;
	uint8_t *audio_buf;
	double audio_size;

	AVFrame *av_frame;
	AVFrame *audio_frame;
#ifdef WITH_SWSCALE
	struct SwsContext *img_convert_ctx;
#endif

	uint8_t *av_buf;
	uint8_t *deinterlace_buffer;
	int packet_len;
	double packet_pts;
	unsigned char *ptr;
	double video_last_P_pts;
	double video_clock;
	double video_current_pts;
	double video_current_pts_time;

	/* audio resample buffer */
	float *audio_float_buf;
	float *audio_resampled_buf;
	unsigned long audio_resampled_buf_len;

	int frame_number;
	int picture_number;

	/**
	 * application variables
	 */
	struct frame_fifo_t {
		AVFrame *frames[FIFO_SIZE];
		int picture_type[FIFO_SIZE];
		int length;
	} frame_fifo;
	int fifo_position;
	bool deinterlaced;
	bool backward_control;
	bool paused;
	bool seekable;
	bool grab_dv;
	double mark_in;
	double mark_out;
	int user_play_speed;
	float play_speed;
	int play_speed_control;


	char *full_filename;

	int video_index;
	int audio_index;

	FILE *fp;

	/** private methods */
	int seek(int64_t timestamp);

	void set_speed(int speed);
	double get_master_clock();
	void deinterlace(AVFrame *frame);
	int new_fifo();
	void free_fifo();
	int new_frame(AVFrame **frame);
	void free_frame(AVFrame **frame);

	DumbCallback *eos;

    FACTORY_ALLOWED
};

#endif
