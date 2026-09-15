/*  FreeJ - FFMpeg encoder
 *
 *  (c) Copyright 2005 Silvano Galliani aka kysucix <kysucix@dyne.org>
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
 *
 * "$Id$"
 *
 */

#include <config.h>

// ffmpeg encoder is temporarly deactivated until we fix it
#undef WITH_AVCODEC

#ifdef WITH_AVCODEC

#include <ffmpeg_encoder.h>
#include <string.h>
#include <context.h>
#include <sdl_screen.h>

FFmpegEncoder::FFmpegEncoder() 
  :VideoEncoder(), encoder_ctx(NULL), sws_ctx(NULL) {

  func("FFmpegEncoder object created");

  set_name("encoder/ffmpeg");

}
FFmpegEncoder::~FFmpegEncoder() {
	func("FFmpegEncoder:::~FFmpegEncoder");

	avcodec_free_context(&encoder_ctx);

	free_av_objects();

	// finish writing the file
	av_write_trailer(afc);

	if (!(afc->oformat->flags & AVFMT_NOFILE)) {
		/* close the output file */
		avio_closep(&afc->pb);
	}
	/* free the format context */
	avformat_free_context(afc);
	afc = NULL;
}

void FFmpegEncoder::free_av_objects() {
  if(picture) av_frame_free(&picture);
  if(tmp_picture) { av_free(tmp_picture); tmp_picture = NULL; }
  if(video_outbuf) { av_free(video_outbuf); video_outbuf = NULL; }
}

bool FFmpegEncoder::init(Context *_env) { 
  if(this->is_running()) return true;
  if(!_init(_env)) return false;
  
  const AVOutputFormat *aof = av_guess_format(NULL,filename,NULL);
  if(!aof) {
    error("Encoder::init::Can't find a correct format for %s. I will use avi",filename);
    aof=av_guess_format("avi",NULL,NULL);
  }
  
  // create format context
  afc = avformat_alloc_context();
  if(!afc) {
    error("FFmpegEncoder::init::Error allocating AVFormatContext!");
    return false;
  }
  afc->oformat=aof;
  snprintf(afc->url, sizeof(afc->url), "%s", filename);
  
  // find the video encoder first to set codec id
  enum AVCodecID codec_id = aof->video_codec;
  if (codec_id == AV_CODEC_ID_NONE)
    codec_id = AV_CODEC_ID_MPEG2VIDEO;

  const AVCodec *encoder = avcodec_find_encoder(codec_id);
  if(!encoder) {
    error("FFmpegEncoder:init::Couldn't find encoder");
    return false;
  }

  // create stream and set parameter
  video_stream = avformat_new_stream(afc, encoder);
  if(!video_stream) {
    error("FFmpegEncoder::init::Error allocating stream!");
    return false;
  }

  // allocate and configure encoder context
  encoder_ctx = avcodec_alloc_context3(encoder);
  if(!encoder_ctx) {
    error("FFmpegEncoder::init::Error allocating encoder context!");
    return false;
  }

  set_encoding_parameter();
  
  if (avcodec_open2(encoder_ctx, encoder, NULL) < 0) {
    error("FFmpegEncoder:init::Couldn't open codec");
    return false;
  }

  if (avcodec_parameters_from_context(video_stream->codecpar, encoder_ctx) < 0) {
    error("FFmpegEncoder:init::Couldn't copy codec parameters to stream");
    return false;
  }

  video_outbuf = NULL;
  if (!(afc->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&afc->pb, filename, AVIO_FLAG_WRITE) < 0) {
      fprintf(stderr, "Could not open '%s'\n", filename);
      return false;
    }
  }

  /* write the stream header */
  if (avformat_write_header(afc, NULL) < 0) {
    error("FFmpegEncoder::init::Error writing header");
    return false;
  }

  // DEBUG stuff
  av_dump_format(afc, 0, filename, 1);

  /* allocate the encoded raw picture */
  picture = av_frame_alloc();
  if (!picture) {
    error("Could not allocate picture\n");
    return false;
  }
  picture->format = encoder_ctx->pix_fmt;
  picture->width  = encoder_ctx->width;
  picture->height = encoder_ctx->height;
  
  if (av_frame_get_buffer(picture, 32) < 0) {
    error("Could not allocate picture buffer\n");
    return false;
  }

  /* if the output format is not YUV420P, then a temporary YUV420P
     picture is needed too */
  tmp_picture = NULL;
  if (encoder_ctx->pix_fmt != AV_PIX_FMT_YUV420P) {
    sws_ctx = sws_getContext(encoder_ctx->width, encoder_ctx->height, AV_PIX_FMT_RGB32,
			     encoder_ctx->width, encoder_ctx->height,
			     AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if (!sws_ctx) {
      error("Could not allocate sws context");
      return false;
    }
    video_outbuf_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
						 encoder_ctx->width, encoder_ctx->height, 1);
    tmp_picture = (uint8_t *)av_malloc(video_outbuf_size);
    if (!tmp_picture) {
      return false;
    }
  }

  open();

  initialized = true;
  return true;
}

bool FFmpegEncoder::write_frame() {
	env->screen->lock();

	func("FFmpegEncoder():write_frame()");
	int ret=0;

	SdlScreen *sdl_screen = (SdlScreen *)env->screen;

	if (sws_ctx) {
		/* convert RGB32 screen to YUV420P */
		uint8_t *src_data[4] = {(uint8_t *)sdl_screen->screen->pixels, NULL, NULL, NULL};
		int src_linesize[4] = {sdl_screen->w * 4, 0, 0, 0};
		uint8_t *dst_data[4] = {NULL};
		int dst_linesize[4] = {0};

		av_image_fill_arrays(dst_data, dst_linesize, tmp_picture,
				     AV_PIX_FMT_YUV420P,
				     encoder_ctx->width, encoder_ctx->height, 1);

		sws_scale(sws_ctx, src_data, src_linesize, 0, encoder_ctx->height,
			  dst_data, dst_linesize);

		av_frame_unref(picture);
		av_image_fill_arrays(picture->data, picture->linesize, tmp_picture,
				     AV_PIX_FMT_YUV420P,
				     encoder_ctx->width, encoder_ctx->height, 1);
	} else {
		/* direct YUV420P input: fill picture from screen surface */
		/* NOTE: this path assumes the screen surface is already YUV420P */
		av_frame_unref(picture);
		picture->data[0] = (uint8_t *)sdl_screen->screen->pixels;
		picture->linesize[0] = sdl_screen->w;
	}

	picture->pts = frame_count++;

	AVPacket *av_packet = av_packet_alloc();
	if (!av_packet) {
		error("FFmpegEncoder :: Error allocating packet");
		env->screen->unlock();
		return false;
	}

	ret = avcodec_send_frame(encoder_ctx, picture);
	if (ret < 0) {
		error("FFmpegEncoder :: Error sending frame for encoding");
		av_packet_free(&av_packet);
		env->screen->unlock();
		return false;
	}

	ret = avcodec_receive_packet(encoder_ctx, av_packet);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		av_packet_free(&av_packet);
		env->screen->unlock();
		return true;
	}
	if (ret < 0) {
		error("FFmpegEncoder :: Error encoding frame");
		av_packet_free(&av_packet);
		env->screen->unlock();
		return false;
	}

	// write to disk
	ret = av_write_frame(afc, av_packet);
	av_packet_free(&av_packet);

	if(ret != 0) {
		error("FFmpegEncoder :: Error while writing frame number %d",frame_count);
		env->screen->unlock();
		return false;
	}
	env->screen->unlock();
	return true;
}

void FFmpegEncoder::open() { 
	/* nothing to do here now, header is already written */
}


void FFmpegEncoder::set_encoding_parameter() { 

	/** Set video codec from the format context */
	encoder_ctx->codec_id = AV_CODEC_ID_MPEG2VIDEO;

	encoder_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

	/* sample parameters */
	encoder_ctx->bit_rate = 2000000;

	/* set resolution accordingly to the global Viewport*/
	encoder_ctx->width = env->screen->w;
	encoder_ctx->height = env->screen->h;

	encoder_ctx->gop_size = 12;

	if (encoder_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO){
		encoder_ctx->mb_decision=2;
	}
	// some formats want stream headers to be separate
	if(!strcmp(afc->oformat->name, "mp4") || !strcmp(afc->oformat->name, "mov") || !strcmp(afc->oformat->name, "3gp"))
		encoder_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}
#endif