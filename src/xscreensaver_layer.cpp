#include <config.h>
#ifdef WITH_XSCREENSAVER

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vroot.h>


#include <context.h>
#include <jutils.h>
#include <xscreensaver_layer.h>


XScreenSaverLayer::XScreenSaverLayer()
  :Layer() {
  set_name("XSS");
}

XScreenSaverLayer::~XScreenSaverLayer() {
  close();
}

bool XScreenSaverLayer::init(Context *ctx, int width, int height) {
  func("%u:%s:%s (%p)",__LINE__,__FILE__,__FUNCTION__, this);
  if(!ctx)
    return false;

  env = ctx;

  _init(width, height);

  buffer = malloc(geo.size);
  // img=XCreatePixmap(dpy, back_win, freej->screen->w, freej->screen->h, 32);
  // img = XGetImage(dpy, back_win, 0, 0, geo.w, geo.h, ~0L, ZPixmap);
  // buffer=img->data;
  // buffer=*(&img->data);

  return true;
}

bool XScreenSaverLayer::init(Context *ctx) {
  if(!ctx)
    return false;
  return init(ctx, ctx->screen->w, ctx->screen->h);
}

// XWindowAttributes xgwa;
// XImage *image = 0;
// XGetWindowAttributes(dpy, window, &xgwa);
// class = visual_class (screen, xgwa.visual);
// if (class == TrueColor)
//
// XGetImage
// (xgwa.depth != 8 && xgwa.depth != 12
// image->data
// image->depth

bool XScreenSaverLayer::open(const char *file) {
  func("XScreenSaverLayer::open(%s)", file);

  dpy = XOpenDisplay(NULL);
  if(!dpy) {
    error("XScreenSaverLayer can't open X display");
    return false;
  }


  // Get some colors

  int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
  int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

  // Create the window
  // Window XCreateSimpleWindow(display, parent, x, y, width, height,
  //                           border_width, border, background)
  back_win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                 400 , 300, 0, blackColor, blackColor);

  // We want to get MapNotify events
  XSelectInput(dpy, back_win, StructureNotifyMask);

  // XSetWindowAttributes attrib;
  // attrib.backing_store = Always;
  // XChangeWindowAttributes(dpy, back_win, CWBackingStore, &attrib);

  XMapWindow(dpy, back_win);
  // Create a "Graphics Context"
  gc = XCreateGC(dpy, back_win, 0, NULL);

  // Status XGetWindowAttributes(display, w, window_attributes_return)
  // extern int XChangeWindowAttributes(
  //    Display*             display ,
  //    Window               w ,
  //    unsigned long        valuemask ,
  //    XSetWindowAttributes*  attributes
  // );

  // unsigned long gcm = GCForeground | GCBackground | GCGraphicsExposures;
  // XGCValues gcv;
  // gcv.graphics_exposures = 0;
  // gc = XCreateGC(dpy, back_win, gcm, &gcv);


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
    notice("%s exec %s", __PRETTY_FUNCTION__, args);
    int res =  execl(file, "", "-window-id", args, NULL);
    notice("%s exec failed %i because %s", __PRETTY_FUNCTION__, res,
           strerror(errno));
    exit(0);
  }

  // char args[sizeof file + 42];
  // sprintf(args, "%s -window-id 0x%x &", file, (int)back_win);
  // notice("Xlayer::open exec %s", args);
  // int res =  system(args);
  // notice("Xlayer::open exec result %i", res);

  notice("Opened XScreenSaver '%s' with back_win %p",file , back_win);

  opened = true;

  set_filename(file);
  return(true);
}

void *XScreenSaverLayer::feed() {
  // notice("feed %i, %i",  freej->screen->w, freej->screen->h);
  // XCopyArea(display, wmgen.pixmap, iconwin, NormalGC, 0,0,
  //           wmgen.attributes.width, wmgen.attributes.height, 0, 0);
  // XCopyArea(dpy, back_win, *img, gc, 0,0, freej->screen->w,
  //           freej->screen->h, 0, 0);
  // buffer=&img.data;
  // XCopyArea(dpy, back_win, *img->data, gc, 0,0, geo.w , geo.h, 0, 0);
  // img = XGetImage(dpy, back_win, 0, 0, freej->screen->w, freej->screen->h,
  // XDestroyImage(img);

  // Pixmap p=XCreatePixmap(dpy, back_win, 400,300,24);
  // XGetSubImage(dpy, back_win, 0,0, geo.w, geo.h, ~0L, ZPixmap, img, 0, 0);
  // XCopyArea(display, src, dest, gc, src_x, src_y, width, height,
  //           dest_x, dest_y)
  // XCopyArea(dpy, back_win, p, gc, 0,0, geo.w , geo.h, 0, 0);
  // XCopyArea(dpy, back_win, (long unsigned int)img, gc, 0,0, geo.w, geo.h,
  //           0, 0);
  // XSync(dpy, true);
  // XFreePixmap(dpy, img);
  // img = XGetImage(dpy, back_win, 0, 0, geo.w, geo.h, 32, XYBitmap);
  img = XGetImage(dpy, back_win, 0, 0, geo.w, geo.h, ~0L, ZPixmap);
  // buffer=img->data;
  memcpy(buffer, img->data, geo.size);
  // buffer=*(&img->data);
  XDestroyImage(img);
  // XSync(dpy, true);
  return buffer;
}

void XScreenSaverLayer::close() {
  notice("Closing XScreenSaver layer");
  if(buffer)
    free(buffer);
  kill(x_pid, SIGTERM);
}

void XScreenSaverLayer::pause(bool set) {
  paused = set;
  int res;
  if (paused)
    res=kill(x_pid, SIGSTOP);
  else
    res=kill(x_pid, SIGCONT);

  notice("XScreenSaver to %i pause : %s is %i err %i", x_pid,
         ((paused) ? "on" : "off"), res,errno );
  show_osd();
}

// bool XScreenSaverLayer::keypress(int key) {
//   bool res = true;
//   switch(key) {
// 	case 'p': pause(!paused);
// 	  break;
//   default: res = false; break;
//   }
//   return res;
// }

#endif
