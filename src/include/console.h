
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <SDL/SDL.h>
#include <slang.h>

typedef struct _File_Line_Type {
  struct _File_Line_Type *next;
  struct _File_Line_Type *prev;
  char *data;			       /* pointer to line data */
  int color; // line color
} File_Line_Type;

class Context;
class Layer;
class Filter;

class Console {
 public:
  
  Console();
  ~Console();
  
  bool init(Context *freej);
  void close();
  void cafudda();

  void notice(char *msg);
  void error(char *msg);
  void warning(char *msg);
  void act(char *msg);
  void func(char *msg);

  
  Context *env;
  


 private:
  int x,y;

  void canvas();
  
  void layerprint();
  void layerlist();
  Layer *layer, *tmplay;
  int layercol;

  void filterprint();
  void filterlist();
  Filter *filter, *tmpfilt;

  void speedmeter();

  void statusline();

  void getkey();

  void scroll(char *msg,int color);
  void update_scroll();
  bool do_update_scroll;

  /* The SLscroll routines will use this structure. */
  void free_lines();
  File_Line_Type *create_line(char *buf);
  File_Line_Type *File_Lines;  
  SLscroll_Window_Type Line_Window;
  File_Line_Type *line, *last_line;
  unsigned int num_lines;
  
};

#endif
