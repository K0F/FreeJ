/*  FreeJ
 *  (c) Copyright 2001 Denis Roio aka jaromil <jaromil@dyne.org>
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

#ifndef __avi_h__
#define __avi_h__

#include <avifile.h>
#include <renderer.h>
#undef BIG_ENDIAN
#include <layer.h>

class AviLayer: public Layer {
 private:
  /* AviReadFile.h in avilib/lib */
  IAviReadFile *_avi;

  
  /* AviReadStream.h
     interesting methods:
     //Size of stream and one frame
     virtual framepos_t GetLength() const;
     virtual double GetLengthTime() const;
     virtual int GetFrameFlags(int* flags) const;
     virtual double GetFrameTime() const;
     virtual framepos_t GetPos() const;
     virtual StreamInfo* GetStreamInfo() const;
     virtual double GetTime(framepos_t frame = ERR) const;
     //Positioning in stream
     virtual int Seek(framepos_t pos);
     virtual int SeekTime(double pos);
     virtual framepos_t SeekToKeyFrame(framepos_t pos);
     virtual double SeekTimeToKeyFrame(double pos);
     virtual int SkipFrame();
     virtual int SkipTo(double pos);
     //Reading decompressed data
     virtual int SetDirection(bool d) { return -1; }
     virtual int SetOutputFormat(void* bi, uint_t size);
     virtual int ReadFrame(bool render = true);
     virtual CImage* GetFrame(bool readframe = false);
     virtual uint_t GetFrameSize() const;
     int ReadFrames(void* buffer, uint_t bufsize, uint_t samples,uint_t& samples_read, uint_t& bytes_read);
     int ReadDirect(void* buffer, uint_t bufsize, uint_t samples,uint_t& samples_read, uint_t& bytes_read, int* flags = 0);
     virtual framepos_t GetNextKeyFrame(framepos_t frame = ERR) const;
     virtual framepos_t GetPrevKeyFrame(framepos_t frame = ERR) const;
     virtual framepos_t SeekToNextKeyFrame();
     virtual framepos_t SeekToPrevKeyFrame(); */
  IAviReadStream *_stream;
  /* -------------------- */
  
  //  VideoRenderer *_rend;
  CImage *_img;
  const CodecInfo *_ci;

  fourcc_t fcc;
  BITMAPINFOHEADER bh;
    //BitmapInfo bh;
    //DEC_PARAM dp;

  unsigned int _samples_read;
  unsigned int _bytes_read;
  
  int _quality;

  double lsttime, curtime;
 bool paused;
 bool avi_dirty;
 int play_speed;
 int steps;
 void set_play_speed(int speed);
 int slow_frame,slow_frame_s;
 void set_slow_frame(int speed);

 bool yuvcc;

 public:
  AviLayer();
  ~AviLayer();
  
  bool init(Context *screen=NULL);
  bool open(char *file);
  void *feed();
  void close();

  framepos_t getpos();
  framepos_t setpos(framepos_t step);
  framepos_t forward(framepos_t step);
  framepos_t rewind(framepos_t step);
  framepos_t mark_in(framepos_t pos);
  framepos_t mark_in_now();
  framepos_t mark_out(framepos_t pos);
  framepos_t mark_out_now();
  framepos_t marker_in;
  framepos_t marker_out;
  
  void pause();
  //  void speedup();
  //  void slowdown();

  bool keypress(int key);
};

#endif
