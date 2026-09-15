/*  FreeJ
 *  (c) Copyright 2001 Silvano Galliani aka kysucix <kysucix@dyne.org>
 *
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
#include <iostream>
#include <config.h>

#ifdef WITH_FFMPEG

#include <samplerate.h>

#include <math.h>

#include <string.h>

#include <screen.h>
#include <context.h>
#include <jutils.h>
#ifdef WITH_AUDIO
#include <jack/jack.h>
#endif
#include <ringbuffer.h>
#include <video_layer.h>

#include <jsparser_data.h>

#include <factory.h>
//#define DEBUG 1

// our objects are allowed to be created trough the factory engine
FACTORY_REGISTER_INSTANTIATOR(Layer, VideoLayer, MovieLayer, ffmpeg);

VideoLayer::VideoLayer()
  :Layer(), av_frame(NULL), audio_frame(NULL), rgba_frame(NULL) {

  grab_dv=false;
  set_name("VID");
  frame_number=0;
  av_buf=NULL;
  avformat_context=NULL;
  packet_len=0;
  frame_rate=0;
  play_speed=1;
  play_speed_control=1;
  seekable=true;
  to_seek = -1;

  audio_resampled_buf_len=0;
  audio_float_buf = NULL;
  audio_resampled_buf = NULL;

  video_codec_ctx = NULL;
  video_index = -1;
  video_codec = NULL;

  audio_codec_ctx = NULL;
  audio_index = -1;
  audio_codec = NULL;
  audio_buf = NULL;

  use_audio = true;

  backward_control=false;
  deinterlace_buffer = NULL;
  video_clock = 0;
  frame_fifo.length = 0;
  jsclass = &video_layer_class;

  eos = new DumbCallback();
}

VideoLayer::~VideoLayer() {
	notice("Closing video %s", get_filename());
	delete eos;
	stop();
	close();
}

/*
 * lickme.txt!
 */

bool VideoLayer::_init() {
  func("VideoLayer::init");
  
  rgba_frame = av_frame_alloc();
  av_frame = av_frame_alloc();
  audio_frame = av_frame_alloc();
  
  /* init variables */
  paused=false;
  user_play_speed=1;
  deinterlace_buffer=NULL;
  deinterlaced=false;
  
  mark_in=NO_MARK;
  mark_out=NO_MARK;
  return true;
}

int VideoLayer::new_frame(AVFrame **frame) {
	*frame = av_frame_alloc();
	if(!*frame)
		return -1;
	(*frame)->format = AV_PIX_FMT_RGB32;
	(*frame)->width = video_codec_ctx->width;
	(*frame)->height = video_codec_ctx->height;
	if (av_frame_get_buffer(*frame, 32) < 0) {
		av_frame_free(frame);
		return -1;
	}
	return 0;

}
void VideoLayer::free_frame(AVFrame **frame) {
	if(frame && *frame) {
		av_frame_free(frame);
		*frame = NULL;
	}
	if (audio_float_buf) free (audio_float_buf);
	if (audio_resampled_buf) free (audio_resampled_buf);
}

bool VideoLayer::open(const char *file) {
  int err=0;
  video_index=-1;
  audio_index=-1;
  func("VideoLayer::open(%s)",file);

  const AVInputFormat *av_input_format = NULL;
  AVDictionary *options = NULL;

  /* handle firewire cam */
  if( strncasecmp (file, "/dev/ieee1394/",14) == 0) {
    notice ("VideoLayer::found dv1394 device!\n");
    grab_dv = true;
    av_input_format = av_find_input_format("dv1394");
    av_dict_set(&options, "standard", "pal", 0);
    file="";
  }

  /**
   * Open media with libavformat
   */
  err = avformat_open_input (&avformat_context, file, av_input_format, &options);
  av_dict_free(&options);
  if (err < 0) {
    error("VideoLayer :: open(%s) - can't open. Error %d", file, err);
    return false;
  }
  func("VideoLayer :: file opened with success");

  /**
   * Find info with libavformat
   */
  err = avformat_find_stream_info(avformat_context, NULL);
  if (err < 0) {
    error("VideoLayer :: could not find stream info");
    return false;
  }
  func("VideoLayer :: stream info found");

  /**
   * Open codec if we find a video stream or audio stream
   */
  unsigned int i;
  for(i=0; i < avformat_context -> nb_streams; i++) {
    avformat_stream = avformat_context -> streams[i];

    switch(avformat_stream->codecpar->codec_type) {

    case AVMEDIA_TYPE_VIDEO:
      video_index = i;
      video_codec = avcodec_find_decoder (avformat_stream->codecpar->codec_id);
      if(video_codec==NULL) {
	error("VideoLayer :: Could not find a suitable codec");
	return false;
      }

      video_codec_ctx = avcodec_alloc_context3(video_codec);
      if(!video_codec_ctx) {
	error("VideoLayer :: Could not allocate codec context");
	return false;
      }
      if (avcodec_parameters_to_context(video_codec_ctx,
					avformat_stream->codecpar) < 0) {
	error("VideoLayer :: Could not copy codec parameters");
	return false;
      }
      video_codec_ctx->thread_count = 1;

      if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
	error("VideoLayer :: Could not open codec");
	return false;

      } else { // correctly opened
	if(avformat_stream->avg_frame_rate.den &&
	   avformat_stream->avg_frame_rate.num)
	  frame_rate = av_q2d(avformat_stream->avg_frame_rate);
	else if(avformat_stream->r_frame_rate.den &&
		avformat_stream->r_frame_rate.num)
	  frame_rate = av_q2d(avformat_stream->r_frame_rate);
	else {
	  AVRational rational = video_codec_ctx->time_base;
	  if(rational.den)
	    frame_rate = av_q2d(av_inv_q(rational));
	  else
	    frame_rate = 25;
	}

	/* set the layer fps */
	fps.set(frame_rate);
	/* this saves only file without full path! */
	set_filename (file);

	act ("%s (codec: %s) has resolution %dx%d and framerate %f",
	     get_filename(), video_codec->name,
	     video_codec_ctx->width, video_codec_ctx->height, frame_rate);

	break;
      }
      
      break; // //////////////// end of video section

    case AVMEDIA_TYPE_AUDIO:
      audio_index = i;
      func ("VideoLayer :: audio id=%i", audio_index);

      if(!use_audio) break;

      audio_codec = avcodec_find_decoder(avformat_stream->codecpar->codec_id);
      if(audio_codec==NULL) {
	error("VideoLayer :: Could not find a suitable codec for audio");
	break;
      }
      audio_codec_ctx = avcodec_alloc_context3(audio_codec);
      if(!audio_codec_ctx) {
	error("VideoLayer :: Could not allocate audio codec context");
	break;
      }
      if (avcodec_parameters_to_context(audio_codec_ctx,
					avformat_stream->codecpar) < 0) {
	error("VideoLayer :: Could not copy audio codec parameters");
	break;
      }
      if (avcodec_open2(audio_codec_ctx, audio_codec, NULL) < 0) {
	error("VideoLayer :: Could not open codec for audio");
	break;

      } else { // correctly opened
	audio_buf = (uint8_t*)calloc(AVCODEC_MAX_AUDIO_FRAME_SIZE, 1);

	audio_channels = audio_codec_ctx->channels;
	audio_samplerate = audio_codec_ctx->sample_rate;

	audio_float_buf = (float*) calloc((size_t)audio_channels *
					  AVCODEC_MAX_AUDIO_FRAME_SIZE,
					  sizeof(float));

	act("VideoLayer :: audio stream (codec: %s) has %u channels at samplerate %u",
	    audio_codec->name, audio_channels, audio_samplerate);

      }
      break; // /////// end of audio section


    case AVMEDIA_TYPE_SUBTITLE:
      act("stream has also subtitles");
      break;
    case AVMEDIA_TYPE_ATTACHMENT:
      act("stream has also attachment");
      break;
    case AVMEDIA_TYPE_DATA:
      act("stream has also a data carrier");
      break;
    default:
      act("stream has also an unknown codec stream");
      break;
    }

  } // done looking for streams

  if (video_index < 0) {
    error("VideoLayer :: Could not open codec");
    return false;
  }

  full_filename = strdup (file);

  geo.init(video_codec_ctx->width, video_codec_ctx->height, 32);
  func("VideoLayer :: w[%u] h[%u] size[%u]", geo.w, geo.h, geo.bytesize);
  func("VideoLayer :: frame_rate[%f]",frame_rate);

  // initialize picture
  if( new_frame(&rgba_frame) < 0) {
    error("VideoLayer::error allocating picture");
    return false;
  }

#ifdef WITH_SWSCALE
  img_convert_ctx =
    sws_getContext(geo.w, geo.h, video_codec_ctx->pix_fmt, geo.w, geo.h,
		   AV_PIX_FMT_RGB32, SWS_BICUBIC, 
		   NULL, NULL, NULL);
#endif

  // initialize frame fifo 
  if(  new_fifo() < 0) {
    error("VideoLayer::error allocating fifo");
    return false;
  }

  // feed() function is called 25 times for second so we must correct the speed
  // TODO user should be able to select the clock speed
  if (play_speed != 25) {
    play_speed -= (25 / frame_rate);
    if ( frame_rate ==1)
      play_speed = 0;
  }
  func ("VideoLayer :: play_speed: %d",play_speed);

  opened = true;

  type = VIDEOLAYER;
  
  return true;
}

static int fill_s16_audio(uint8_t *dst, int max_bytes, AVFrame *frame) {
  int channels = frame->channels;
  int nb = frame->nb_samples;
  int needed = nb * channels * 2;
  if (needed > max_bytes) return -1;

  if (frame->format == AV_SAMPLE_FMT_S16) {
    memcpy(dst, frame->data[0], needed);
  } else if (frame->format == AV_SAMPLE_FMT_S16P) {
    int16_t *out = (int16_t*) dst;
    for (int c = 0; c < channels; c++) {
      int16_t *src = (int16_t*) frame->data[c];
      for (int i = 0; i < nb; i++)
	out[i*channels+c] = src[i];
    }
  } else if (frame->format == AV_SAMPLE_FMT_FLT) {
    float *src = (float*) frame->data[0];
    int16_t *out = (int16_t*) dst;
    for (int i = 0; i < nb*channels; i++)
      out[i] = (int16_t)(src[i]*32767.0f);
  } else if (frame->format == AV_SAMPLE_FMT_FLTP) {
    int16_t *out = (int16_t*) dst;
    for (int c = 0; c < channels; c++) {
      float *src = (float*) frame->data[c];
      for (int i = 0; i < nb; i++)
	out[i*channels+c] = (int16_t)(src[i]*32767.0f);
    }
  } else {
    return -1;
  }
  return needed;
}

void *VideoLayer::feed() {
  int got_picture=0;
  int ret=0;
  bool got_it=false;

  double now = get_master_clock();


  if(paused)
    return rgba_frame->data[0];

  /**
   * follow user video loop
   */
  if(mark_in!=NO_MARK && mark_out!=NO_MARK && seekable) {
    if (now >= mark_out)
      seek((int64_t)mark_in * AV_TIME_BASE);
  }
  
  // operate seek if was requested
  if(to_seek>=0) {
    seek(to_seek);
    to_seek = -1;
  }
    
  got_it=false;
  
  while (!got_it) {
    
    
    if(packet_len<=0) {
      /**
       * Read one packet from the media and put it in pkt
       */
      while(1) {
#ifdef DEBUG
	func("av_read_frame ...");
#endif
	ret = av_read_frame(avformat_context, &pkt);

#ifdef DEBUG
	if(pkt.stream_index == video_index)
	  std::cout << "video read packet";
	else if(pkt.stream_index == audio_index)
	  std::cout << "audio read packet";
	std::cout << " pkt.data=" << pkt.data;
	std::cout << " pkt.size=" << pkt.size;
	std::cout << " pkt.pts/dts=" << pkt.pts << "/" << pkt.dts << std::endl;
	std::cout << "pkt.duration=" << pkt.duration;
	std::cout << " avformat_context->start_time=" << avformat_context->start_time;
	std::cout << " avformat_context->duration=" << avformat_context->duration/AV_TIME_BASE << std::endl;
	std::cout << "avformat_context->duration=" << avformat_context->duration << std::endl;
#endif
	
	/**
	 * check eof and loop
	 */
	if(ret!= 0) {	//does not enter if data are available
	  eos->notify();
	  ret = seek(avformat_context->start_time);
	  if (ret < 0) {
	    error("VideoLayer::could not loop file");
	    return rgba_frame->data[0];
	  }
	  continue;
	} else if( (pkt.stream_index == video_index) 
		   || ((use_audio) && (pkt.stream_index == audio_index)) )
	  break; /* exit loop */
      }
    } // loop break after a known index is found
    
    
    frame_number++;
    
    /**
     * Decode video
     */
    if(pkt.stream_index == video_index) {
      
      if (avcodec_send_packet(video_codec_ctx, &pkt) < 0) {
	func("error sending packet to decoder");
	av_packet_unref(&pkt);
	packet_len=0;
	continue;
      }
      av_packet_unref(&pkt);
      packet_len=0;

      if (avcodec_receive_frame(video_codec_ctx, av_frame) == 0) {
	got_it=true;
	avformat_stream=avformat_context->streams[video_index];
	
	/** Deinterlace input if requested */
	if(deinterlaced)
	  deinterlace(av_frame);
	
#ifdef WITH_SWSCALE
	sws_scale(img_convert_ctx, av_frame->data, av_frame->linesize,
		  0, video_codec_ctx->height,
		  rgba_frame->data, rgba_frame->linesize);	  
#else
	/** yuv2rgb */
	// without swscale the old img_convert() is gone in modern ffmpeg
	error("VideoLayer::compiled without swscale, cannot convert frames");
	return NULL;
#endif
	if(fifo_position == FIFO_SIZE)
	  fifo_position=0;
	
	/* workaround since sws_scale conversion from YUV
	   returns an buffer RGBA with alpha set to 0x0  */
	{
	  int bufsize = ( rgba_frame->linesize[0] * video_codec_ctx->height ) /4;
	  int32_t *pbuf =  (int32_t*)rgba_frame->data[0];
	  
	  for(; bufsize>0; bufsize--) {
	    *pbuf = (*pbuf | alpha_bitmask);
	    pbuf++;
	  }
	} 
	
	jmemcpy(frame_fifo.frames[fifo_position]->data[0],
		rgba_frame->data[0],
		rgba_frame->linesize[0] * video_codec_ctx->height);
	
	fifo_position++;
      }
    } // end video packet decoding
    

    ////////////////////////
    // audio packet decoding
    else if((use_audio) && (pkt.stream_index == audio_index)) {
      if(audio_codec_ctx && screen) {
	if (avcodec_send_packet(audio_codec_ctx, &pkt) < 0) {
	  av_packet_unref(&pkt);
	  packet_len=0;
	  continue;
	}
	av_packet_unref(&pkt);
	packet_len=0;

	while (avcodec_receive_frame(audio_codec_ctx, audio_frame) == 0) {
	  int data_size = fill_s16_audio(audio_buf,
					 AVCODEC_MAX_AUDIO_FRAME_SIZE,
					 audio_frame);
	  if (data_size > 0) {
	    int samples = data_size/sizeof(uint16_t);
	    long unsigned int m_SampleRate = screen->m_SampleRate?*(screen->m_SampleRate):48000;
	    double m_ResampleRatio = (double)(m_SampleRate)/(double)audio_samplerate; 
	    long unsigned max_buf = ceil(AVCODEC_MAX_AUDIO_FRAME_SIZE * m_ResampleRatio * audio_channels);

	    if (audio_resampled_buf_len < max_buf) {
	      if (audio_resampled_buf) free (audio_resampled_buf);
	      audio_resampled_buf = (float*) malloc(max_buf * sizeof(float));
	      audio_resampled_buf_len = max_buf;
	    }

	    src_short_to_float_array ((const short*) audio_buf, audio_float_buf, samples);
	    if (m_ResampleRatio == 1.0) 
	    {
	      ringbuffer_write(screen->audio, (const char*)audio_float_buf,  samples*sizeof(float));
	    } 
	    else 
	    {
	      SRC_DATA src_data;
	      int offset = 0;

	      do {
		src_data.input_frames  = samples/audio_channels;
		src_data.output_frames = audio_resampled_buf_len/audio_channels - offset;
		src_data.end_of_input  = 0;
		src_data.src_ratio     =  m_ResampleRatio;
		src_data.input_frames_used = 0;
		src_data.output_frames_gen = 0;
		src_data.data_in       = audio_float_buf + offset; 
		src_data.data_out      = audio_resampled_buf + offset;

		src_simple (&src_data, SRC_SINC_MEDIUM_QUALITY, audio_channels) ;
		ringbuffer_write(screen->audio,
				 (const char*)audio_resampled_buf,
				 src_data.output_frames_gen * audio_channels *sizeof(float));

		offset += src_data.input_frames_used * audio_channels;
		samples -= src_data.input_frames_used * audio_channels;

		if (samples>0)
		  warning("resampling left: %i < %i",
			  src_data.input_frames_used, samples/audio_channels);

	      } while (samples > audio_channels);
	    }
	  }
	}
      } else {
	av_packet_unref(&pkt);
	packet_len=0;
      }
    }
    
  } // end of while(!got_it)
  
  return frame_fifo.frames[fifo_position-1]->data[0];
}


void VideoLayer::close() {
  if(frame_number!=0)
    av_packet_unref(&pkt);

  if(video_codec_ctx) {
    func("close video codec");
    avcodec_free_context(&video_codec_ctx);
  }
  
  if(audio_codec_ctx) {
    func("close audio codec");
    avcodec_free_context(&audio_codec_ctx);
    if(audio_buf) {
      func("free audio buffer");
      free(audio_buf);
      audio_buf=NULL;
    }
  }

  if(img_convert_ctx) {
    sws_freeContext(img_convert_ctx);
    img_convert_ctx=NULL;
  }
  
  if(avformat_context) {
    avformat_close_input(&avformat_context);
  }

  free_fifo();
  if(rgba_frame) free_frame(&rgba_frame);
  if(av_frame) { av_frame_free(&av_frame); av_frame=NULL; }
  if(audio_frame) { av_frame_free(&audio_frame); audio_frame=NULL; }
  if(deinterlace_buffer) { av_free(deinterlace_buffer); deinterlace_buffer=NULL; }
}

/*
 * allocate fifo
 */
int VideoLayer::new_fifo() {
  fifo_position=0;
	int ret;
	// loop throught fifo
	for ( int s = 0; s < FIFO_SIZE; s++) {
		ret = new_frame(&frame_fifo.frames[s]);
		if ( ret < 0)
			return -1;
		frame_fifo.length++;
	}
	return 0;
}

void VideoLayer::free_fifo() {
	for ( int s = 0; s < frame_fifo.length; s++) {
		free_frame(&frame_fifo.frames[s]);
	}
}
bool VideoLayer::set_mark_in() {
	if (mark_in == NO_MARK) {
		mark_in = get_master_clock();
		notice("mark_in: %f", mark_in);
	}
	else {
		mark_in = NO_MARK;
		notice("mark_in deleted");
	}
	return true;
}
bool VideoLayer::set_mark_out() {
	if (mark_out == NO_MARK) {
		mark_out = get_master_clock();
		notice("mark_out: %f", mark_out);
	}
	else {
		mark_out = NO_MARK;
		notice("mark_out deleted");
	}
	return true;
}

bool VideoLayer::relative_seek(double increment) {
	int ret=0;
	double current_time=get_master_clock();
	current_time += increment;
	/**
	 * Check the seek time is correct!
	 */
	if (current_time < 0)  // beginning
		current_time = 0;
	/**
	 * Forward in video as a loop
	 */
	else  { // beginning
		while(current_time > (avformat_context -> duration / AV_TIME_BASE))  {
			current_time = current_time - (avformat_context->duration / AV_TIME_BASE);
		}
	}

	ret = seek ((int64_t) current_time * AV_TIME_BASE);
	if (ret < 0) {
		error ("Can't seek file: %s", get_filename());
		return false;
	}
	else
		notice("seek to %.1f\%",current_time);
	return true;
}
/**
 * Warning! doesn't lock feed
 */
int VideoLayer::seek(int64_t timestamp) {
  /* return value */
  int ret=0;
  bool seeking_at_beginning_of_stream=false;

  if(timestamp==avformat_context->start_time)
    seeking_at_beginning_of_stream=true;
  if (video_codec && strcmp(video_codec->name,"rawvideo")==0)
    seekable=false;

  if(!seekable) {
    if(seeking_at_beginning_of_stream) {
      /** close and reopen the stream*/
      {
	close();
	open(full_filename);
      }
      return 0;
    }
    else {
      if(video_codec)
	warning("%s video is not seekable!",video_codec->name);
      return -1;
    }
  }

  /**
   * HERE sick
   */
  func("SEEKING");
  ret = av_seek_frame(avformat_context, video_index,timestamp,
		      AVSEEK_FLAG_BACKWARD);

  if(ret<0) {
    seekable=false;
    if(seeking_at_beginning_of_stream) {
      /** close and reopen the stream*/
      {
	close();
	open(full_filename);
	return 0;
      }
    }
  }
  else { // seek with success
    // Flush buffers, should be called when seeking or when swicthing to a different stream.
    if (video_codec_ctx)
	avcodec_flush_buffers(video_codec_ctx);
    if (audio_codec_ctx)
	avcodec_flush_buffers(audio_codec_ctx);
  }
  return 0;
}
double VideoLayer::get_master_clock() {
	double delta = (av_gettime() - video_current_pts_time) / 1000000.0;
	return (video_current_pts+delta);
}
void VideoLayer::pause() {
  if(paused)
    paused=false;
  else
    paused=true;
  notice("pause : %s",(paused)?"on":"off");
}
void VideoLayer::deinterlace(AVFrame *frame) {
  int w = video_codec_ctx->width;
  int h = video_codec_ctx->height;
  int size;
  
  size = av_image_get_buffer_size(video_codec_ctx->pix_fmt, w, h, 1);
  
  /* allocate only first time */
  if(deinterlace_buffer==NULL)
    deinterlace_buffer = (uint8_t *)av_malloc(size);
  if (!deinterlace_buffer)
    return ;
  
  AVFrame *tmp = av_frame_alloc();
  if(!tmp)
    return;
  av_image_fill_arrays(tmp->data, tmp->linesize, deinterlace_buffer,
		       video_codec_ctx->pix_fmt, w, h, 1);
  
  /* simple vertical blend deinterlace: each output line is the average
     of the current line and the one above it */
  for(int p = 0; p < 3; p++) {
    int ph = (p == 0) ? h : h/2;
    int pw = (p == 0) ? w : w/2;
    for(int y = 1; y < ph; y++) {
      int yt = y - 1;
      for(int x = 0; x < pw; x++) {
	uint8_t a = frame->data[p][y * frame->linesize[p] + x];
	uint8_t b = frame->data[p][yt * frame->linesize[p] + x];
	tmp->data[p][y * tmp->linesize[p] + x] = (uint8_t)((a + b) / 2);
      }
    }
    /* first line: copy as-is */
    memcpy(tmp->data[p], frame->data[p], (size_t)pw);
  }

  av_frame_unref(frame);
  av_frame_ref(frame, tmp);
  av_frame_free(&tmp);
  return;
}

#endif