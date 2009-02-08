/*  FreeJ
 *  (c) Copyright 2001-2002 Denis Rojo aka jaromil <jaromil@dyne.org>
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
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <frei0r.h>

#include <config.h>


#include <plugger.h>
#include <context.h>
#include <jutils.h>
#include <frei0r_freej.h>
#include <freeframe_freej.h>


Plugger::Plugger() {
  char temp[256];

  _searchpath = NULL;

  addsearchdir(PACKAGE_LIB_DIR);

  sprintf(temp,"%s/.freej/plugins",getenv("HOME"));
  addsearchdir(temp);

  addsearchdir("/usr/lib/FreeFrame");
  addsearchdir("/usr/local/lib/FreeFrame");
  addsearchdir("/usr/lib/frei0r-1");
  addsearchdir("/usr/local/lib/frei0r-1");

//   addsearchdir("/usr/lib/freej");
//   addsearchdir("/usr/local/lib/freej");
//   addsearchdir("/opt/video/lib/freej");


}

Plugger::~Plugger() {

  func("Plugger::~Plugger()");
  if(_searchpath) free(_searchpath);

}

int selector(struct dirent *dir) {
  if(strstr(dir->d_name,".so")) return(1);
  return(0);
}


int Plugger::refresh(Context *env) {

  char *dir;
  struct dirent **filelist;
  int found;
  char *path = _getsearchpath();

  
  notice("serching available plugins");
 
  if(!path) { warning("can't find any valid plugger directory"); return(-1); }

  dir = strtok(path,":");

  // scan for all available effects
  do {
    func("scanning %s",dir);

      found = scandir(dir,&filelist,selector,alphasort);
      if(found<0) { error("Plugger::scandir"); return(-1); };
      /* .so files found, check if they are plugins */
      
      
      while(found--) {
	
	char temp[256];
	
	snprintf(temp,255,"%s/%s",dir,filelist[found]->d_name);
	free(filelist[found]);

	{
	  Freior *fr = new Freior();
	  
	  if( ! fr->open(temp) ) {
	    delete fr;
	  } else { // freior effect found
	    // check what kind of plugin is and place it
	    if(fr->info.plugin_type == F0R_PLUGIN_TYPE_FILTER) {
	      
	      Filter *filt = new Filter(Filter::FREIOR,fr);
	      env->filters.append(filt);
	      
	      func("found frei0r filter: %s (%p)", filt->name, fr);
	      continue;

	    } else if(fr->info.plugin_type == F0R_PLUGIN_TYPE_SOURCE) {
	      
	      Filter *filt = new Filter(Filter::FREIOR,fr);
	      env->generators.append(filt);
	      
	      func("found frei0r generator: %s (%p)", filt->name, fr);
	      continue;

	    } else {
	      func("frei0r plugin of type %i not supported (yet)",
		   fr->info.plugin_type);
	    }
	  }
	}

	{
	  Freeframe *fr = new Freeframe();
	  if( ! fr->open(temp) ) {
	    delete fr;
	  } else { // freeframe effect found
	    // check what kind of plugin is and place it
	    if(fr->info->pluginType == FF_EFFECT) {
	      
	      Filter *filt = new Filter(Filter::FREEFRAME, fr);
	      env->filters.append(filt);
	      
	      func("found freeframe filter: %s (%p)",
		   fr->info->pluginName, fr);
	      continue;

	    } else if(fr->info->pluginType == FF_SOURCE) {
	      
	      Filter *filt = new Filter(Filter::FREEFRAME, fr);
	      env->generators.append(filt);
	      
	      func("found freeframe generator: %s (%p)",
		   fr->info->pluginName, fr);
	      continue;

	    }
	  }
	}

	if(found<0) break;
      }
      
  } while((dir = strtok(NULL,":")));

  free(filelist);

  act("filters found: %u", env->filters.len());
  act("generators found: %u", env->generators.len());

  return 0;
}


void Plugger::addsearchdir(const char *dir) {
  char temp[1024];
  if(!dircheck(dir)) return;
  if(_searchpath) {
    snprintf(temp,1024,"%s:%s",_searchpath,dir);
    jfree(_searchpath);
    _searchpath = strdup(temp);
  } else _searchpath = strdup(dir);
}
