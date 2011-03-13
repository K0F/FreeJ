/*  FreeJ - Ogg/Vorbis/Theora encoder
 *
 *  (c) Copyright 2005 Silvano Galliani <kysucix@dyne.org>
 *                2007 Denis Rojo       <jaromil@dyne.org>
 *
 * ogg/vorbis/theora code learned from encoder_example and ffmpeg2theora
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

#ifdef WITH_OGGTHEORA
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <context.h>
#include <screen.h>
#include <audio_collector.h>

#include <oggtheora_encoder.h>
#include <iomanip>
using namespace std;
using std::setiosflags;

OggTheoraEncoder::OggTheoraEncoder() 
  : VideoEncoder() {
  func("OggTheoraEncoder object created");
  
  video_quality  = 16;  // it's ok for streaming
  audio_quality =  10; // it's ok for streaming

  //  picture_rgb = NULL;
  //  enc_rgb24 = NULL;

  use_audio = false;
  audio = NULL;
  audio_buf = NULL;
  m_buffStream = NULL;
  m_MixBuffer = NULL;
  m_MixBufferOperation = NULL;
  m_MixedRing = NULL;
  init_info(&oggmux);
  theora_comment_init(&oggmux.tc);


  set_name("encoder/theora");

}

OggTheoraEncoder::~OggTheoraEncoder() { // XXX TODO clear the memory !!
  func("OggTheoraEncoder:::~OggTheoraEncoder");
  
  oggmux_flush(&oggmux, 1);
  oggmux_close(&oggmux);
  
  //  if(enc_rgb24) free(enc_rgb24);

  if(audio_buf) free(audio_buf);
  if(m_MixBuffer) free(m_MixBuffer);
  if(m_MixBufferOperation) free(m_MixBufferOperation);
  if (m_buffStream) free(m_buffStream);
  if (m_MixedRing) ringbuffer_free(m_MixedRing);
}


bool OggTheoraEncoder::init (ViewPort *scr) {

  if(initialized) return true;

  screen = scr;

  oggmux.ringbuffer = ringbuffer;
  oggmux.bytes_encoded = 0;

  oggmux.audio_only = 0;

  if(use_audio && audio) {
    func("allocating encoder audio buffer of %u bytes",audio->buffersize);
    audio_buf = (float*)calloc(audio->buffersize, sizeof(float));

    m_MixVal = 1.0;	//sound mix level coef.
    oggmux.video_only = 0;
    oggmux.sample_rate = audio->samplerate;
    if (audio->Jack->m_ringbufferchannels)
      oggmux.channels = audio->Jack->m_ringbufferchannels;
    else
      oggmux.channels = 1;	//sets to 1 channel if there is no Jack output one
    oggmux.vorbis_quality = (double)audio_quality/100.0;	/*audio_quality / 100;*/
    oggmux.vorbis_bitrate = audio_bitrate;
    
    m_MixBuffer = (float*) malloc(4096 * 1024);
    m_MixBufferOperation = (float*) malloc(4096 * 1024 *2);
    
    m_buffStream = (float *)malloc(4096 * 512 * 4);	//size must be the same as audio_mix_ring declared in JackClient::Attach() 
    m_MixedRing = ringbuffer_create(4096 * 512 * 4);
  
  } else {

    oggmux.video_only = 1;
    use_audio = false;

  }


  /* Set up Theora encoder */

  int theora_quality = (int) ( (video_quality * 63) / 100);
  int w                   = screen->geo.w;
  int h                   = screen->geo.h;
  func("VideoEncoder: encoding theora to quality %u", theora_quality);
  /* Theora has a divisible-by-sixteen restriction for the encoded video size */
  /* scale the frame size up to the nearest /16 and calculate offsets */
  video_x = ( (w + 15) >> 4) << 4;
  video_y = ( (h + 15) >> 4) << 4;
  /* We force the offset to be even.
     This ensures that the chroma samples align properly with the luma
     samples. */
  frame_x_offset = ( (video_x - w ) / 2) &~ 1;
  frame_y_offset = ( (video_y - h ) / 2) &~ 1;


  /* video settings here */
  theora_info_init (&oggmux.ti);

  oggmux.ti.width                        = video_x;
  oggmux.ti.height                       = video_y;
  oggmux.ti.frame_width                  = screen->geo.w;
  oggmux.ti.frame_height                 = screen->geo.h;
  oggmux.ti.offset_x                     = frame_x_offset;
  oggmux.ti.offset_y                     = frame_y_offset;
  oggmux.ti.fps_numerator                = 25; // env->fps.fps;
  oggmux.ti.fps_denominator              = 1;
  oggmux.ti.aspect_numerator             = 0;
  oggmux.ti.aspect_denominator           = 0;
  oggmux.ti.colorspace                   = OC_CS_ITU_REC_470BG;
  //	oggmux.ti.colorspace                   = OC_CS_UNSPECIFIED;
  // #ifndef HAVE_64BIT
  oggmux.ti.pixelformat                  = OC_PF_420; // was OC_PF_420 with ccvt
  // #endif
  oggmux.ti.target_bitrate               = video_bitrate;
  oggmux.ti.quality                      = theora_quality;
  
  oggmux.ti.dropframes_p                 = 0; // was 0
  oggmux.ti.quick_p                      = 1;
  oggmux.ti.keyframe_auto_p              = 1;
  oggmux.ti.keyframe_frequency           = 64;
  oggmux.ti.keyframe_frequency_force     = 64;
  oggmux.ti.keyframe_data_target_bitrate = (unsigned int) (video_bitrate * 1.5);
  oggmux.ti.keyframe_auto_threshold      = 80;
  oggmux.ti.keyframe_mindistance         = 8;
  oggmux.ti.noise_sensitivity            = 1;
  oggmux.ti.sharpness                    = 1;

  oggmux_init(&oggmux);
  
  enc_y     = malloc( screen->geo.w * screen->geo.h);
  enc_u     = malloc((screen->geo.w * screen->geo.h) /2);
  enc_v     = malloc((screen->geo.w * screen->geo.h) /2);
  enc_yuyv   = (uint8_t*)malloc(  screen->geo.bytesize );
  
  act("initialization successful");
  initialized = true;
 	
  return true;
}

void OggTheoraEncoder::setMixCoef(float val)
{
  m_MixVal = val;
}

int OggTheoraEncoder::Mux(int nfr)
{
  int sizeFirst = 0;
  int sizeVideo = 0;
  float *ringPtr, *mixPtr;
  memset (m_MixBuffer, 0, 4096 * 1024);
  memset (m_MixBufferOperation, 0, 4096 * 1024 * 2);
  
  if (sizeFirst = ringbuffer_read_space(audio->Jack->first))
  {
    size_t rv = ringbuffer_read(audio->Jack->first, (char *)m_MixBuffer, sizeFirst);
    if (rv != sizeFirst)
    {
      std::cerr << "----- Problems reading the in_ring" << std::endl;
      return (false);
    }
    if (sizeVideo = ringbuffer_read_space(audio->Jack->audio_mix_ring))	//
    {
      size_t rf = ringbuffer_read(audio->Jack->audio_mix_ring, (char *)m_MixBufferOperation, sizeVideo);
      if (rf != sizeVideo)
      {
	std::cerr << "----- Problems reading the audio_mix_ring" << std::endl;
	return (false);
      }
      mixPtr = m_MixBufferOperation;
      ringPtr = m_MixBuffer;
      for (int j=0; j < rf/sizeof(float); j++, mixPtr++, ringPtr++)
      {
	if (oggmux.channels > 1)
	  *mixPtr++ += *ringPtr * m_MixVal;	//both channels
	*mixPtr += *ringPtr * m_MixVal;		//reduce input level
      }
      if (ringbuffer_write_space (m_MixedRing) >= sizeVideo)
      {
	size_t rb = ringbuffer_write (m_MixedRing, (char *)m_MixBufferOperation, sizeVideo);
	if (rb != sizeVideo)
	{
	  std::cerr << "---" << rb << " : au lieu de :" << sizeVideo \
	      << " octets ecrits dans le ringbuffer !!" << std::endl;
	  return (0);
	}
      }
      else
      {
	std::cerr << "------ not enough memory in audio_mix_ring buffer !!!" << std::endl;
	return (0);
      }
    }
    else
    {
      mixPtr = m_MixBufferOperation;
      ringPtr = m_MixBuffer;
      for (int j=0; j < sizeFirst/sizeof(float); j++, mixPtr++, ringPtr++)
      {
	if (oggmux.channels > 1)
	  *mixPtr++ += *ringPtr * m_MixVal;	//both channels
	*mixPtr += *ringPtr * m_MixVal;		//reduce audio input level on all channels
      }
      if (ringbuffer_write_space (m_MixedRing) >= sizeFirst)
      {
	size_t rf = ringbuffer_write (m_MixedRing, (char *)m_MixBufferOperation, sizeFirst*oggmux.channels);
	if (rf != sizeFirst*oggmux.channels)
	{
	  std::cerr << "---" << rf << " : au lieu de :" << sizeFirst*oggmux.channels \
	      << " octets ecrits dans le ringbuffer !!" << std::endl;
	  return (0);
	}
	return (sizeFirst*oggmux.channels);
      }
    }
  }
  else	//no sound in the first ring buffer
  {
    if (sizeVideo = ringbuffer_read_space(audio->Jack->audio_mix_ring))	//
    {
      size_t rv = ringbuffer_read(audio->Jack->audio_mix_ring, (char *)m_MixBufferOperation, sizeVideo);
      if (rv != sizeVideo)
      {
	std::cerr << "----- Problems reading the audio_mix_ring" << std::endl;
	return (0);
      }
    }
    if ((ringbuffer_write_space (m_MixedRing) >= sizeVideo) && sizeVideo)
    {
      size_t rv = ringbuffer_write (m_MixedRing, (char *)m_MixBufferOperation, sizeVideo);
      if (rv != sizeVideo)
      {
	std::cerr << "---" << rv << " : au lieu de :" << sizeVideo \
	    << " octets ecrits dans le ringbuffer !!" << std::endl;
	return (0);
      }
    }
  }
  return (sizeVideo);
}

int OggTheoraEncoder::encode_frame() {
  static int written = 0;
  encode_video ( 0);
  if (use_audio)
  {
	float *ptr = m_buffStream;
	int sizeFilled = 0;
	size_t rv = 0;
	sizeFilled = Mux(1024);
// 	std::cerr << "--sizeFilled :" << sizeFilled << std::endl;
	if (int rf = ringbuffer_read_space (m_MixedRing))
	{
// 	  std::cerr << "--rf :" << rf << std::endl;
	  double rff = 0;
	  if (rf > (4096 * 512 * 4))
	    rf  = 4096 * 512 * 4;
	  else
	  {
	    rff = ceil(rf/sizeof(float));
	    rff = (rff*sizeof(float)) - sizeof(float);	//take the bigest number divisible by 4 and lower than rf (ringbuffer available datas)
	  }
// 	  std::cerr << "--rff :" << rff << std::endl;
	  if (rff >= ((1024 * sizeof(float) * oggmux.channels - 1)))	// > to 1024 frames in stereo
	  {
	    if ((rv = ringbuffer_read(m_MixedRing, (char *)m_buffStream, (size_t)rff)) == 0)
	    {
	      std::cerr << "------impossible de lire dans le audio_mix_ring ringbuffer !!!"\
		    << " rff:" << rff << " rv:" << rv << endl;
	    }
	    else if (rv != rff)
	    {
	      std::cerr << "------pas assez lu dans audio_mix_ring ringbuffer !!!"\
		    << " rff:" << rff << " rv:" << rv << std::endl << std::flush;
	    }
 	    encode_audio ( 0, rv);
	  }
	}
  }
  
  oggmux_flush(&oggmux, 0);

  bytes_encoded = oggmux.video_bytesout + oggmux.audio_bytesout;	//total from the beginning

  audio_kbps = oggmux.akbps;
  video_kbps = oggmux.vkbps;

  // just pass the reference for the status
  status = &oggmux.status[0];
  return bytes_encoded;
}



int OggTheoraEncoder::encode_video( int end_of_stream) {
  yuv_buffer          yuv;
  
  /* take picture and convert it to yuv420 */

  // picture was feeded in the right format by feed_video
  
  /* Theora is a one-frame-in,one-frame-out system; submit a frame
     for compression and pull out the packet */
  yuv.y_width   = video_x;
  yuv.y_height  = video_y;
  //  yuv.y_stride  = picture_yuv->linesize [0];
  yuv.y_stride = video_x;
  
  yuv.uv_width  = video_x >> 1;
  yuv.uv_height = video_y >> 1;
  //  yuv.uv_stride = picture_yuv->linesize [1];
  yuv.uv_stride = video_x >> 1;

   yuv.y = (uint8_t *) enc_y;
   yuv.u = (uint8_t *) enc_u;
   yuv.v = (uint8_t *) enc_v;
  
  /* encode image */
  oggmux_add_video (&oggmux, &yuv, end_of_stream);
  return 1;
}

int OggTheoraEncoder::encode_audio( int end_of_stream, size_t sizeInBuff) {

#ifdef WITH_AUDIO
  //  num = env->audio->framesperbuffer*env->audio->channels*sizeof(int16_t);
//   func("going to encode %u bytes of audio", audio->buffersize);
  ///// QUAAAA
  //  oggmux_add_audio (oggmux_info *info, int16_t * readbuffer, int bytesread, int samplesread,int e_o_s);
  //  oggmux_add_audio(&oggmux, env->audio->input,
  //		   read,
  //		   read / env->audio->channels /2,
  //		   end_of_stream );
  //  audio->get_audio(audio_buf);

    oggmux_add_audio(&oggmux, m_buffStream,
  		   sizeInBuff,
  		   sizeInBuff/(sizeof(float)*oggmux.channels), //read / oggmux.channels,
  		   end_of_stream );
#endif

  return 1;
}




#endif
