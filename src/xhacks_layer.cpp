#include <config.h>
#ifdef WITH_XHACKS

#include <xhacks_layer.h>
#include <X11/Xlib.h> 
#include <X11/Xutil.h>
#include <assert.h>   
#include <unistd.h>
#include <string.h>
#include <context.h>
#include <jutils.h>
#include <sys/types.h>
#include <signal.h>

#define NIL (0)       // A name for the void pointer

  Display *dpy;
  Window back_win;
  //Pixmap back_win;
  XImage *img;	
  GC gc;
  bool paused;

XHacksLayer::XHacksLayer() 
  :Layer() {
  //_avi = NULL;
  //_stream = NULL;
  setname("XHL");
}

XHacksLayer::~XHacksLayer() {
  close();
}

bool XHacksLayer::init(Context *scr) {
  func("XHacksLayer::init");
  if(scr) freej = scr;
     _init(freej, freej->screen->w, freej->screen->h, freej->screen->bpp);

  buffer = jalloc(buffer,geo.size);
//  img=XCreatePixmap(dpy, back_win, freej->screen->w, freej->screen->h, 32);
//img = XGetImage(dpy, back_win, 0, 0, geo.w, geo.h, ~0L, ZPixmap);
//buffer=img->data;
//buffer=*(&img->data);
  return true;
}

/*
 *  XWindowAttributes xgwa;
  XImage *image = 0;
    XGetWindowAttributes(dpy, window, &xgwa);
  class = visual_class (screen, xgwa.visual);
  if (class == TrueColor)
  
  XGetImage
(xgwa.depth != 8 && xgwa.depth != 12
image->data
image->depth
 * */


bool XHacksLayer::open(char *file) { 
  func("XHacksLayer::open(%s)", file);

      dpy = XOpenDisplay(NIL);
      assert(dpy);

      // Get some colors

      int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
      int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

      // Create the window
//Window XCreateSimpleWindow(display, parent, x, y, width, height, border_width, border, background)
      back_win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 
				    400 , 300, 0, blackColor, blackColor);

      // We want to get MapNotify events
      XSelectInput(dpy, back_win, StructureNotifyMask);

/*
XSetWindowAttributes attrib;
attrib.backing_store = Always;
XChangeWindowAttributes(dpy, back_win, CWBackingStore, &attrib);
*/
      
XMapWindow(dpy, back_win); 
      // Create a "Graphics Context"
      gc = XCreateGC(dpy, back_win, 0, NIL);

/*
Status XGetWindowAttributes(display, w, window_attributes_return)
extern int XChangeWindowAttributes(
    Display*             display ,
    Window               w ,
    unsigned long        valuemask ,
    XSetWindowAttributes*  attributes 
);
*/

/*unsigned long gcm = GCForeground | GCBackground | GCGraphicsExposures;
XGCValues gcv;
    gcv.graphics_exposures = 0;
    gc = XCreateGC(dpy, back_win, gcm, &gcv);
*/


      for(;;) {
	    XEvent e;
	    XNextEvent(dpy, &e);
	    if (e.type == MapNotify)
		  break;
      }
      
     x_pid = fork();
      if (x_pid == 0) { // child
	char args[32];
	sprintf(args, "0x%x", (int)back_win);
	notice("Xlayer::open exec %s", args);
int res =  execl(file, "", "-window-id", args, NULL);
	notice("Xlayer::open exec failed %i because %s", res, strerror(errno));
	exit(0);
      } else {
      }

      /*char args[sizeof file + 42];
      	sprintf(args, "%s -window-id 0x%x &", file, (int)back_win);
	notice("Xlayer::open exec %s", args);
int res =  system(args);
	notice("Xlayer::open exec result %i", res);
*/

  notice("Opened xhacks '%s' with back_win %p",file , back_win);

  set_filename(file);
  return(true);
}
 
void *XHacksLayer::feed() {
//	notice("feed %i, %i",  freej->screen->w, freej->screen->h);
//  XCopyArea(display, wmgen.pixmap, iconwin, NormalGC, 0,0, wmgen.attributes.width, wmgen.attributes.height, 0, 0);	
//  XCopyArea(dpy, back_win, *img, gc, 0,0, freej->screen->w , freej->screen->h, 0, 0);	
  //buffer=&img.data;
//  XCopyArea(dpy, back_win, *img->data, gc, 0,0, geo.w , geo.h, 0, 0);	
//img = XGetImage(dpy, back_win, 0, 0, freej->screen->w, freej->screen->h, 
//XDestroyImage(img);
	
//Pixmap p=XCreatePixmap(dpy, back_win, 400,300,24);
//XGetSubImage(dpy, back_win, 0,0, geo.w, geo.h, ~0L, ZPixmap, img, 0, 0);
//XCopyArea(display, src, dest, gc, src_x, src_y, width, height,  dest_x, dest_y)
//XCopyArea(dpy, back_win, p, gc, 0,0, geo.w , geo.h, 0, 0);	
//XCopyArea(dpy, back_win,  (long unsigned int)img, gc, 0,0, geo.w , geo.h, 0, 0);	
//XSync(dpy, true);
//XFreePixmap(dpy, img);
//img = XGetImage(dpy, back_win, 0, 0, geo.w, geo.h, 32, XYBitmap);
img = XGetImage(dpy, back_win, 0, 0, geo.w, geo.h, ~0L, ZPixmap);
//buffer=img->data;
memcpy(buffer, img->data, geo.size);
//buffer=*(&img->data);
XDestroyImage(img);
//XSync(dpy, true);
  return buffer;
}

void XHacksLayer::close() {
  notice("Closing XHC layer");
  if(buffer) jfree(buffer);
  kill(x_pid, SIGTERM);
}

void XHacksLayer::pause(bool set) {
  paused = set;
  int res;
  if (paused)
	  res=kill(x_pid, SIGSTOP);
  else
	  res=kill(x_pid, SIGCONT);
		  
  notice("xhack to %i pause : %s is %i err %i", x_pid, ((paused) ? "on" : "off"), res,errno );
  show_osd();
}

bool XHacksLayer::keypress(SDL_keysym *keysym) {
  bool res = false;
  switch(keysym->sym) {
	case SDLK_KP0: pause(!paused);
	    res = true; break;
	default: break;
  }
  return res;
}

/*  framepos_t steps = 1;
  switch(keysym->sym) {
  case SDLK_RIGHT:
    if(keysym->mod & KMOD_LCTRL) steps=5000;
    if(keysym->mod & KMOD_RCTRL) steps=1000;
    forward(steps);
    res = true; break;

  case SDLK_LEFT:
    if(keysym->mod & KMOD_LCTRL) steps=5000;
    if(keysym->mod & KMOD_RCTRL) steps=1000;
    rewind(steps);
    res = true; break;

  case SDLK_i:
    if(keysym->mod & KMOD_SHIFT)
    set_mark_in(); break;

  case SDLK_o:
    if(keysym->mod & KMOD_SHIFT)
    set_mark_out(); break;
    
	case SDLK_n: 
		set_play_speed(-1);
    res = true; break;

	case SDLK_m: 
		set_play_speed(+1);
    res = true; break;

	case SDLK_k: 
		set_slow_frame(-1);
    res = true; break;

	case SDLK_l: 
		set_slow_frame(+1);
    res = true; break;
		
  case SDLK_KP0: pause();
    res = true; break;

  default: break;
  }
  return res;
*/

#endif
