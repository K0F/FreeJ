#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xmalloc.h"
#include "playlist.h"


/**
 * Escape 'string' according to RFC3986 and
 * http://oauth.net/core/1.0/#encoding_parameters.
 *
 * @param string The data to be encoded
 * @return encoded string otherwise NULL
 * The caller must free the returned string.
 */
char *my_url_escape(const char *string) {
  size_t alloc, newlen;
  char *ns = NULL, *testing_ptr = NULL;
  unsigned char in;
  size_t strindex=0;
  size_t length;

  if (!string) return strdup("");

  alloc = strlen(string)+1;
  newlen = alloc;

  ns = (char*) xmalloc(alloc);

  length = alloc-1;
  while(length--) {
    in = *string;

    switch(in){
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': case '~': case '.': case '-':
      ns[strindex++]=in;
      break;
    default:
      newlen += 2; /* this'll become a %XX */
      if(newlen > alloc) {
        alloc *= 2;
        testing_ptr = (char*) xrealloc(ns, alloc);
        ns = testing_ptr;
      }
      snprintf(&ns[strindex], 4, "%%%02X", in);
      strindex+=3;
      break;
    }
    string++;
  }
  ns[strindex]=0;
  return ns;
}

struct ttargs {
  int del;
  char *me; 
  char *src;
};

#define THREADED

#ifdef THREADED
#include <pthread.h>
pthread_t thread_id_tt;
int       thread_status_tt = 0;
#endif

void *tac_tell_thread(void *arg){
  struct ttargs *a = (struct ttargs*) arg;
  char *d, *see, *mee;
  char uri[BUFSIZ];
#ifdef THREADED
  thread_status_tt = 1;
#endif
  see=my_url_escape(a->src); if (!see) return(0);
  mee=my_url_escape(a->me); if (!mee) { free(see); return(0);}
  snprintf(uri, BUFSIZ, "http://theartcollider.net/yp/tell/php?name=%s&srcurl=%s%s",mee, see, a->del?"&mode=del":"");
  free(see); free(mee);
  fprintf(stderr,"TAC-TELL URL; %s\n", uri);
  d=curl_get(uri);
  if (d) { fprintf(stderr, "TAC-TELL reply: '%s'\n",d); free(d); }
#ifdef THREADED
  free(a);
  thread_status_tt = 0;
#endif
  return(0);
}

#ifndef THREADED
void tac_tell(int del, char *me, char *src){
  struct ttargs args;
  args.del= del;
  args.me = me;
  args.src= src;
  tac_tell_thread(&args);
}
#else



void tac_tell(int del, char *me, char *src){
  if (thread_status_tt) {
  // TODO: check if thread is still/already running
    fprintf(stderr,"TAC-tell process is already active. skipped request\n");
    return;
  }
  struct ttargs *args = xmalloc(sizeof(struct ttargs));
  args->del=del;
  args->me =me;
  args->src=src;
  pthread_create(&thread_id_tt, NULL, tac_tell_thread, args);
}
#endif

// ************************
//      tac_tell(0, "mixx", movieurl); // add
//      tac_tell(1, "mixx", movieurl); // del
