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


//--------------------------------------------
// Manage video file (encoding)
//--------------------------------------------

FILE *outputfile = NULL;
int outbuf_size = 0;
uint8_t *outbuf;
AVCodecContext    *outCodecCtx;

int        movie_width;
int        movie_height;
int        render_fmt = AV_PIX_FMT_YUV420P; // PIX_FMT_ARGB;

static int flush_encoder(void) {
  int out_size = 0;
  AVPacket pkt;
  int got = 0;

  avcodec_send_frame(outCodecCtx, NULL);
  while (avcodec_receive_packet(outCodecCtx, &pkt) == 0) {
    got = 1;
    fwrite(pkt.data, 1, pkt.size, outputfile);
    av_packet_unref(&pkt);
  }
  return out_size;
}

int open_output_file(const char *file_name, int inW, int inH, int inFramerate) {
  outbuf_size = 1024*1024;
  int bitrate = 512 * 1024;
  movie_width = inW;
  movie_height= inH;

  AVCodec *codec;

//codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
  codec = avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO);
//codec = avcodec_find_encoder(AV_CODEC_ID_XVID);
//codec = avcodec_find_encoder(AV_CODEC_ID_DVVIDEO);
//codec = avcodec_find_encoder(AV_CODEC_ID_XVID);
//codec = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);

  outCodecCtx= avcodec_alloc_context3(codec);
  if(!outCodecCtx) {
    fprintf(stderr, "could not allocate encode context.\n");
    return -1;
  }

  outCodecCtx->bit_rate = bitrate;
  outCodecCtx->rc_buffer_size = outbuf_size;
  outCodecCtx->rc_min_rate = bitrate;
  outCodecCtx->rc_max_rate = bitrate;
  outCodecCtx->sample_aspect_ratio = (AVRational){1,1};
  outCodecCtx->width = movie_width;
  outCodecCtx->height = movie_height;
  outCodecCtx->time_base= (AVRational){1,inFramerate};
  outCodecCtx->gop_size = 30;
//outCodecCtx->max_b_frames=1;
  outCodecCtx->pix_fmt = render_fmt;
  outCodecCtx->flags |= AV_CODEC_FLAG_QSCALE;
//outCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (avcodec_open2(outCodecCtx, codec, NULL) < 0) {
    fprintf(stderr, "could not initiate encoder codec.\n");
    return -1;
  }

  outputfile = fopen(file_name, "wb");
  if(outputfile == NULL) {
    fprintf(stderr, "Couldn't open outputfile: %s\n", file_name);
    return -1;
  }
  outbuf = (uint8_t *) malloc(outbuf_size);
  return 0;
}


void encode_frame (AVFrame *picture) {
  static int max_size=0;
  AVPacket pkt;
  if (outputfile == NULL) return;
  av_init_packet(&pkt);
  pkt.data = outbuf;
  pkt.size = outbuf_size;

  if (avcodec_send_frame(outCodecCtx, picture) < 0) {
    fprintf(stderr, "encoding frame failed\n");
    return;
  }
  if (avcodec_receive_packet(outCodecCtx, &pkt) == 0) {
    int out_size = pkt.size;
    fwrite(pkt.data, 1, out_size, outputfile);
    if (out_size>max_size) {
      max_size=out_size;
      if (100.0*out_size/outbuf_size > 50)
        fprintf(stderr, "\nmax buf : %i/%i (%.1f%%)\n",out_size,outbuf_size,100.0*out_size/outbuf_size);
    }
    av_packet_unref(&pkt);
  }
}

#include <theora/theora.h> // we use theoras yuv_buffer & oggenc's argb_to_420 method
int yuv_copy__argb_to_420(void *b_rgb, size_t b_rgb_stride, size_t width, size_t height, size_t offset_x, size_t offset_y, yuv_buffer *dst);
AVFrame* picture = NULL;

void encode_buffer(uint8_t *buffer) { 

  if (!picture) {

    picture= av_frame_alloc();
    if(!picture) {
      fprintf(stderr, "could not allocate picture\n");
      return;
    }

    int size = movie_width * movie_height;
    uint8_t* picture_buf = malloc((size * 3) / 2); /* size for YUV 420 */

    picture->format = AV_PIX_FMT_YUV420P;
    picture->width = movie_width;
    picture->height = movie_height;
    picture->data[0] = picture_buf;
    picture->data[1] = picture->data[0] + size;
    picture->data[2] = picture->data[1] + size / 4;
    picture->linesize[0] = movie_width;
    picture->linesize[1] = movie_width / 2;
    picture->linesize[2] = movie_width / 2;
  }
  
  yuv_buffer myYUV;
  myYUV.y= picture->data[0];
  myYUV.u= picture->data[1];
  myYUV.v= picture->data[2];
  yuv_copy__argb_to_420(buffer, movie_width*4, movie_width, movie_height, 0, 0, & myYUV);

  //memcpy(picture->data[0], buffer, (movie_width * movie_height *3) /2);

  encode_frame(picture);
}

void close_output_file(void) {
  if (outputfile == NULL) {
    return;
  }

  flush_encoder();

  free(outbuf);
  fclose(outputfile);
  outputfile = NULL;
  if (picture) { 
    AVFrame *p = picture;
    free(p->data[0]);
    picture = NULL;
    av_frame_free(&p);
  }
  if (outCodecCtx)
    avcodec_free_context(&outCodecCtx);
}