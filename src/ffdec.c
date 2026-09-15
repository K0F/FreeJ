#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <ffdec.h>

int want_quiet =1;

struct ffdec {
  int               videoStream;
  AVFormatContext   *pFormatCtx;
  AVCodecContext    *pCodecCtx;
  AVFrame           *pFrame;
  AVFrame           *pFrameFMT;
  struct SwsContext *pSWSCtx;
  AVPacket 	    packet;
  int               fFirstTime;

  int               ff_width;
  int               ff_height;
  double            ff_fps;
  uint8_t           *buffer;
  int               sc_width;
  int               sc_height;

  int               pt_status; // status/mode  for pthread
} ffdec;

//--------------------------------------------
// Manage video file (reading)
//--------------------------------------------

void init_ffmpeg() {
  /* in modern ffmpeg registration is automatic, nothing to do */
  if(want_quiet) av_log_set_level(AV_LOG_QUIET);
}

int get_width(ffdec_t *ffp) {
  ffdec_t *ff = (ffdec_t *) ffp;
  return ff->ff_width;
}

int get_height(ffdec_t *ffp) {
  ffdec_t *ff = (ffdec_t *) ffp;
  return ff->ff_height;
}

int get_scaled_width(ffdec_t *ffp) {
  ffdec_t *ff = (ffdec_t *) ffp;
  return ff->sc_width;
}

int get_scaled_height(ffdec_t *ffp) {
  ffdec_t *ff = (ffdec_t *) ffp;
  return ff->sc_height;
}

double get_fps(ffdec_t *ffp) {
  ffdec_t *ff = (ffdec_t *) ffp;
  return ff->ff_fps;
}

uint8_t *get_bufptr(ffdec_t *ffp) {
  ffdec_t *ff = (ffdec_t *) ffp;
  return ff->buffer;
}

int get_pt_status(ffdec_t *ffp) {
  ffdec_t *ff = (ffdec_t *) ffp;
  return ff->pt_status;
}

void init_moviebuffer(ffdec_t *ffp, int width, int height, int render_fmt) {
  ffdec_t *ff = (ffdec_t *) ffp;
  int     numBytes;

  ff->pFrameFMT=av_frame_alloc();
  if(ff->pFrameFMT==NULL) {
      fprintf( stderr, "Cannot allocate FMT buffer\n");
      exit( -1 );
  }

  if (!want_quiet)
    fprintf(stderr, "scaling %dx%d -> %dx%d\n", ff->pCodecCtx->width, ff->pCodecCtx->height, width, height);

  numBytes=av_image_get_buffer_size(render_fmt, width, height, 1);
  ff->buffer=(uint8_t *) malloc(numBytes ? numBytes : 1);

  av_image_fill_arrays(ff->pFrameFMT->data, ff->pFrameFMT->linesize, ff->buffer, render_fmt, width, height, 1);
  ff->pSWSCtx = sws_getContext(ff->pCodecCtx->width, ff->pCodecCtx->height, ff->pCodecCtx->pix_fmt, width, height, render_fmt, SWS_BICUBIC, NULL, NULL, NULL);
  ff->sc_width  = width;
  ff->sc_height = height;
  ff->pt_status |= 1;
  ff->pt_status |= 2;
}

void free_moviebuffer(ffdec_t *ffp) {
  ffdec_t *ff = (ffdec_t *) ffp;
  if (ff->buffer) free(ff->buffer);
}

int open_movie(ffdec_t ** ffpx, char* movie_url) {
  int      i;
  AVCodec *pCodec;
  double   duration;
  long     frames;
  ffdec_t **ffp = (ffdec_t **) ffpx;
  if (! *ffp) *ffp   = (ffdec_t *) calloc(1,sizeof (ffdec_t));
  ffdec_t *ff   = (ffdec_t *) *ffp;

  ff->pt_status=8;
  ff->videoStream=-1;
  ff->fFirstTime=1;
  
  if(avformat_open_input(&ff->pFormatCtx, movie_url, NULL, NULL)!=0) {
    fprintf( stderr, "Cannot open video file: '%s'\n", movie_url);
    ff->pt_status=0;
    return -1;
  }

  if(avformat_find_stream_info(ff->pFormatCtx, NULL)<0) {
    fprintf( stderr, "Cannot find stream information in file %s\n", movie_url);
    ff->pt_status=0;
    return -1;
  }

  if (!want_quiet) av_dump_format(ff->pFormatCtx, 0, movie_url, 0);

  for(i=0; i<ff->pFormatCtx->nb_streams; i++) {
    if(ff->pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) {
      ff->videoStream=i;
      break;
    }
  }

  if(ff->videoStream==-1) {
    fprintf( stderr, "Cannot find a video stream in file %s\n", movie_url);
    ff->pt_status=0;
    return -1;
  }

  AVStream *av_stream = ff->pFormatCtx->streams[ff->videoStream];
  if(av_stream->avg_frame_rate.num && av_stream->avg_frame_rate.den)
    ff->ff_fps = (double) av_stream->avg_frame_rate.num / av_stream->avg_frame_rate.den;
  else if(av_stream->r_frame_rate.num && av_stream->r_frame_rate.den)
    ff->ff_fps = (double) av_stream->r_frame_rate.num / av_stream->r_frame_rate.den;
  else
    ff->ff_fps = 25.0;
  duration = (double) ( (int64_t) ff->pFormatCtx->duration / (int64_t) AV_TIME_BASE);
  frames = (long) (ff->ff_fps * duration);
  
  if (!want_quiet) {
    fprintf(stdout, "original frame rate: %g\n", ff->ff_fps);
    fprintf(stdout, "length in seconds: %g\n", duration);
    fprintf(stdout, "total frames: %ld\n", frames);
  }
  
  pCodec=avcodec_find_decoder(av_stream->codecpar->codec_id);
  if(pCodec==NULL) {
    fprintf( stderr, "Cannot find a codec for %s\n", movie_url);
    ff->pt_status=0;
    return -1;
  }

  ff->pCodecCtx=avcodec_alloc_context3(pCodec);
  if (avcodec_parameters_to_context(ff->pCodecCtx, av_stream->codecpar)<0) {
    fprintf(stderr, "Cannot set up codec for %s\n", movie_url);
    ff->pt_status=0;
    return -1;
  }
  
  ff->ff_width = ff->pCodecCtx->width;
  ff->ff_height = ff->pCodecCtx->height;

  ff->sc_width = ff->sc_height = 0;

  if(avcodec_open2(ff->pCodecCtx, pCodec, NULL)<0) {
    fprintf( stderr, "Cannot open the codec for file %s\n", movie_url);
    ff->pt_status=0;
    return -1;
  }
  
  ff->pFrame=av_frame_alloc();
  if(ff->pFrame==NULL) {
    fprintf(stderr, "Cannot allocate frame for %s\n", movie_url);
    ff->pt_status=0;
    return -1;
  }
  ff->pt_status=0;
  return 0;
}

int open_camera(ffdec_t ** ffpx, char* device) {
  ffdec_t **ffp = (ffdec_t **) ffpx;
  if (! *ffp) *ffp   = (ffdec_t *) calloc(1,sizeof (ffdec_t));
  ffdec_t *ff   = (ffdec_t *) *ffp;
  ff->videoStream=-1;
  ff->pt_status=0;
  ff->fFirstTime=1;

  AVCodec *pCodec;
  AVInputFormat *fmt1;

  const char *video_grab_format = "video4linux2";
  fmt1 = av_find_input_format(video_grab_format);

  AVDictionary *options = NULL;
  av_dict_set(&options, "framerate", "30", 0);
  av_dict_set(&options, "video_size", "320x240", 0);

  /* open the input */
  if (avformat_open_input(&ff->pFormatCtx, device, fmt1, &options) < 0) {
    av_dict_free(&options);
    fprintf(stderr, "Could not find video grab device\n");
    return(-1);
  }
  av_dict_free(&options);

  if (avformat_find_stream_info(ff->pFormatCtx, NULL) < 0) {
    fprintf(stderr, "Could not find video grab parameters\n");
    return(-1);
  }

  /* by now video grab has one stream */
  ff->videoStream=0;
  for(int i=0; i<ff->pFormatCtx->nb_streams; i++) {
    if(ff->pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) {
      ff->videoStream=i;
      break;
    }
  }
  if (!want_quiet) av_dump_format(ff->pFormatCtx, 0, device, 0);

  AVStream *av_stream = ff->pFormatCtx->streams[ff->videoStream];
  if(av_stream->avg_frame_rate.num && av_stream->avg_frame_rate.den)
    ff->ff_fps = (double) av_stream->avg_frame_rate.num / av_stream->avg_frame_rate.den;
  else
    ff->ff_fps = 30.0;

  pCodec=avcodec_find_decoder(av_stream->codecpar->codec_id);
  if(pCodec==NULL) {
    fprintf( stderr, "Cannot find a codec for %s\n", device);
    return -1;
  }

  ff->pCodecCtx=avcodec_alloc_context3(pCodec);
  if (avcodec_parameters_to_context(ff->pCodecCtx, av_stream->codecpar)<0) {
    fprintf(stderr, "Cannot set up codec for %s\n", device);
    return -1;
  }

  ff->ff_width  = ff->pCodecCtx->width;
  ff->ff_height = ff->pCodecCtx->height;
  ff->sc_width = ff->sc_height = 0;

  if(avcodec_open2(ff->pCodecCtx, pCodec, NULL)<0) {
    fprintf( stderr, "Cannot open the codec for file %s\n", device);
    return -1;
  }
  
  ff->pFrame=av_frame_alloc();
  if(ff->pFrame==NULL) {
    fprintf(stderr, "Cannot allocate frame for %s\n", device);
    return -1;
  }
  return 0;
}

int open_ffmpeg(ffdec_t ** ffpx, char* file) {
  if(!strncmp(file, "/dev/", 5)) 
    return open_camera(ffpx, file);
  return open_movie(ffpx, file);
}

int decode_frame(ffdec_t *ffp) {
  ffdec_t *ff = ffp;
  int frameFinished=0;
 
  if(ff->fFirstTime) {
    ff->fFirstTime=0;
    avcodec_flush_buffers(ff->pCodecCtx);
  }

  do {  
    if(!frameFinished) {
      if(av_read_frame(ff->pFormatCtx, &ff->packet)<0) {
        fprintf(stderr,"End of file reached.\n");
        return 0;
      }
      if(ff->packet.stream_index != ff->videoStream) {
        av_packet_unref(&ff->packet);
        continue;
      }
    }
    if(avcodec_send_packet(ff->pCodecCtx, &ff->packet) < 0 &&
       avcodec_send_packet(ff->pCodecCtx, &ff->packet) != AVERROR(EAGAIN)) {
      fprintf(stderr,"Error sending packet to decoder\n");
      av_packet_unref(&ff->packet);
      return 0;
    }
    av_packet_unref(&ff->packet);
    frameFinished = (avcodec_receive_frame(ff->pCodecCtx, ff->pFrame) == 0);
    if(frameFinished) break;
    if (avcodec_receive_frame(ff->pCodecCtx, ff->pFrame) != AVERROR(EAGAIN))
      break;
  } while (!frameFinished);

  if(frameFinished) {
    if(ff->pSWSCtx)
      sws_scale(ff->pSWSCtx, (const uint8_t**) ff->pFrame->data, ff->pFrame->linesize, 0, ff->pCodecCtx->height, ff->pFrameFMT->data, ff->pFrameFMT->linesize);
  }
  return frameFinished;
}
  
void close_movie(ffdec_t *ffp) {
  ffdec_t *ff = ffp;

  if (ff->pSWSCtx) sws_freeContext(ff->pSWSCtx);
  if (ff->pFrameFMT) { av_frame_free(&ff->pFrameFMT); }
  if (ff->pFrame) { av_frame_free(&ff->pFrame); }
  if (ff->pCodecCtx) avcodec_free_context(&ff->pCodecCtx);
  if (ff->pFormatCtx) avformat_close_input(&ff->pFormatCtx);
}

void free_ff(ffdec_t **ffpx) {
  ffdec_t *ff = *ffpx;
  free(ff);
  *ffpx = NULL;
}

void close_and_free_ff(ffdec_t **ffpx) {
  ffdec_t *ff = *ffpx;
#if 0
  int timeout=250;
  while (--timeout && ff->pt_status&10) usleep(20000);// do not free while thread is active
#endif
  if (ff) {
    close_movie(ff);
    free_moviebuffer(ff);
  }
  if (ffpx) {
    free_ff(ffpx);
    ffpx = NULL;
  }
}

void limit_size(ffdec_t *ffp) {
#ifdef FIX_AMOEBA_SIZE
  ff_width=384; ff_height=288;
#else
#define MAX_X (1024)
#define MAX_Y (300)
  ffdec_t *ff = (ffdec_t *) ffp;
  while(ff->ff_width > MAX_X || ff->ff_height > MAX_Y)  {
    if (ff->ff_width > MAX_X)  {
      ff->ff_height = ff->ff_height * MAX_X/ff->ff_width;
      ff->ff_width = MAX_X;
    }

    if (ff->ff_height > MAX_Y)  {
      ff->ff_width = ff->ff_width * MAX_Y/ff->ff_height;
      ff->ff_height = MAX_Y;
    }
  }
#endif
  ff->ff_width=((ff->ff_width + 15) >>4)<<4;
  ff->ff_height=((ff->ff_height + 15) >>4)<<4;
}

// ---------------------------------------------------------------------------

void stride_memcpy(void *dst, const void *src, int width, int height, int dstStride, int srcStride) {
  int i;
  if(dstStride == srcStride)
    memcpy(dst, src, srcStride*height);
  else for(i=0; i<height; i++) {
    memcpy(dst, src, width);
    src+= srcStride;
    dst+= dstStride;
  }
}


void yuv_memcpy(void *dst, const void *src, int src_width, int src_height, int out_width, int out_height, int xoff, int yoff) {
  int out_size = out_width*out_height;
  int src_size = src_width*src_height;
  stride_memcpy(dst+xoff+out_width*yoff, src, src_width, src_height, out_width, src_width);

  stride_memcpy(dst+ out_size                + (xoff>>1) + ((out_width*yoff)>>2),
                src+ src_size                , src_width>>1, src_height>>1, out_width>>1, src_width>>1);
  stride_memcpy(dst+ out_size + (out_size>>2)+ (xoff>>1) + ((out_width*yoff)>>2),
                src+ src_size + (src_size>>2), src_width>>1, src_height>>1, out_width>>1, src_width>>1);
}



void yuv_letterbox(void *dst, const void *src, int src_width, int src_height, int out_width, int out_height) {
  if (src_width == out_width && src_height == out_height) {
    memcpy(dst, src, src_width*src_height*3/2);
    return;
  }
  // assert(src_width  <= out_width);
  // assert(src_height <= out_height);

  int xoff=((out_width-src_width)>>1)& ~0x01;
  int yoff=((out_height-src_height)>>1)& ~0x01;
  int out_size = out_width*out_height;
  int src_size = src_width*src_height;
  stride_memcpy(dst+xoff+out_width*yoff, src, src_width, src_height, out_width, src_width);

  stride_memcpy(dst+ out_size                + (xoff>>1) + ((out_width*yoff)>>2),
                src+ src_size                , src_width>>1, src_height>>1, out_width>>1, src_width>>1);
  stride_memcpy(dst+ out_size + (out_size>>2)+ (xoff>>1) + ((out_width*yoff)>>2),
                src+ src_size + (src_size>>2), src_width>>1, src_height>>1, out_width>>1, src_width>>1);
}

void init_letterbox(uint8_t *d, int w, int h) {
  int i;
  for (i=0;i<w*h;i++) d[i]=0;
  for (i=0;i<w*h/2;i++) d[w*h+i] = 0x80;
}

void calc_letterbox(int src_w, int src_h, int out_w, int out_h, int *sca_w, int *sca_h) {
  if (src_w*out_h > src_h*out_w) {
    (*sca_w)=out_w;
    (*sca_h)=(int)round((float)out_w*(float)src_h/(float)src_w);
  } else {
    (*sca_h)=out_h;
    (*sca_w)=(int)round((float)out_h*(float)src_w/(float)src_h);
  }
  (*sca_w)=(((*sca_w) + 1) >>1)<<1;
  (*sca_h)=(((*sca_h) + 1) >>1)<<1;
}

/* With select() the timeout is an UPPER bound (it waits at most.. - and
 * uses a signal to wake up), while [nano|u]sleep the time is a LOWER bound
 * (it waits at least for the given time). */
void select_sleep (int usec) {
  struct timeval tv;
  if (usec<0) return;
  if (usec<1000000) {
    tv.tv_sec = 0; 
    tv.tv_usec = usec;
  } else {
    tv.tv_sec  = usec/1000000;
    tv.tv_usec = usec%1000000;
  }
  select(0, NULL, NULL, NULL, &tv);
}


#define THREADED

#ifdef THREADED
#include <pthread.h>

struct fftarg {
  void **ffpx;
  char *movie_url;
  int w,h,render_fmt;
};

void *ffdec_decode_thread(void *ffpx) {
	ffdec_t **ffp = (ffdec_t **) ffpx;
	ffdec_t *ff = (ffdec_t *) *ffp;
	
	if (!decode_frame(ff)) {
		fprintf(stderr,"decoding failed. closing!\n");
		ff->pt_status|=4;
	}
	
	ff->pt_status&=~2;
	return(0);
}

void *ffdec_open_thread(void *arg) {
  struct fftarg *a = (struct fftarg*) arg;
  ffdec_t **ffpx = (ffdec_t **) a->ffpx;
  ffdec_t *ff = *ffpx;
    
  if (open_movie(ffpx, a->movie_url)) {
    //fprintf(stderr,"movie open failed!\n");
    ff->pt_status|=4;
  } else {
    int xw,xh;
#if 0
    calc_letterbox(get_width(ff),get_height(ff),a->w,a->h,&xw,&xh);  // XXX
#else
    xw=a->w; xh=a->h;
#endif
    if (!want_quiet)
	    printf("letterbox: %dx%d\n",xw,xh);
    init_moviebuffer(ff, xw, xh, a->render_fmt);
//  fprintf(stderr,"movie opened: %s!\n",a->movie_url);
    ff->pt_status|=1;
#if 1
    ff->pt_status |= 2;
    //ffdec_decode_thread(ffp);
#endif
  }
  free(a);
  return(0);
}

int ffdec_thread(ffdec_t **ffpx, char *movie_url, int w, int h, int render_fmt) {
  ffdec_t **ffp = (ffdec_t **) ffpx;
  pthread_t thread_id_tt;
  pthread_attr_t thread_att;
  pthread_attr_init(&thread_att);
  pthread_attr_setdetachstate(&thread_att,PTHREAD_CREATE_DETACHED);

  if (*ffp) {
    ffdec_t *ff   = (ffdec_t *) *ffp;
    if ((ff->pt_status & 4 )) {
      close_and_free_ff(ffpx);
      return(-1);
    }
  }

  if (!*ffp && movie_url) {
    struct fftarg *args = malloc(sizeof(struct fftarg));
    args->ffpx=(void *)ffpx;
    args->movie_url=movie_url;
    args->w=w; args->h=h;
    args->render_fmt = render_fmt;
    if (pthread_create(&thread_id_tt, &thread_att, ffdec_open_thread, args)) {
      free(args);
    }
    pthread_attr_destroy(&thread_att);
    return 0;
  }

  if (*ffp) {
    ffdec_t *ff   = (ffdec_t *) *ffp;
    if ((ff->pt_status & 7 ) == 1) {
      ff->pt_status |= 2;
      if (pthread_create(&thread_id_tt, &thread_att, ffdec_decode_thread, ffp)) {
        fprintf(stderr, "thread creation failed.");
        ff->pt_status&=~2;
      }
      pthread_attr_destroy(&thread_att);
      return 0;
    }
    //printf("DEBUG0 %i %i\n", ff->pt_status, render_fmt); 
  }
 return 1;
}

#endif