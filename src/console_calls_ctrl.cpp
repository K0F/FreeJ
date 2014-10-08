/*  FreeJ - S-Lang console
 *
 *  (c) Copyright 2004-2009 Denis Roio <jaromil@dyne.org>
 *
 * This source code  is free software; you can  redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software  Foundation; either version 3 of  the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but  WITHOUT ANY  WARRANTY; without  even the  implied  warranty of
 * MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 *
 * You should  have received  a copy of  the GNU Public  License along
 * with this source code; if  not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <algorithm>

#include <console_calls_ctrl.h>
#include <console_readline_ctrl.h>
#include <console_ctrl.h>
#include <parameter.h>
#include <context.h>
#include <blitter.h>
#include <layer.h>

#include <generator_layer.h>

#include <jutils.h>

int console_param_selection(ContextPtr env, char *cmd) {
    if(!cmd) return 0;
    if(!strlen(cmd)) return 0;

    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    LayerPtr lay = screen->mSelectedLayer;
    if(!lay) {
        ::error("no layer currently selected");
        return 0;
    }
    FilterInstancePtr filt = lay->mSelectedFilter;

    // find the values after the first blank space
    char *p;
    for(p = cmd; *p != '\0'; p++)
        if(*p == '=') {
            *p = '\0';
            if(*(p - 1) == ' ')
                *(p - 1) = '\0';
            p++;
            break;
        }

    while(*p == ' ') p++;  // jump all spaces
    if(*p == '\0') return 0;  // no value was given

    if(filt) { ///////////////////////// parameters for filter
        LockedLinkList<Parameter> list = LockedLinkList<Parameter>(filt->parameters);
        LockedLinkList<Parameter>::iterator it = std::find_if(list.begin(), list.end(), [&] (ParameterPtr &param) {
                                                                  return param->getName() == cmd;

                                                              });

        if(it == list.end()) {
            error("parameter %s not found in filter %s", cmd, filt->proto->getName().c_str());
            return 0;
        } else {
            ParameterPtr param = *it;
            func("parameter %s found in filter %s",
                 param->getName().c_str(), filt->proto->getName().c_str());

            // parse from the string to the value
            param->parse(p);
        }
    } else { /////// parameters for layer
        LockedLinkList<Parameter> list = LockedLinkList<Parameter>(lay->parameters);
        LockedLinkList<Parameter>::iterator it = std::find_if(list.begin(), list.end(), [&] (ParameterPtr &param) {
                                                                  return param->getName() == cmd;

                                                              });

        if(it == list.end()) {
            error("parameter %s not found in layers %s", cmd, lay->getName().c_str());
            return 0;
        } else {
            ParameterPtr param = *it;
            func("parameter %s found in layer %s at position %u",
                 param->getName().c_str(), lay->getName().c_str());

            // parse from the string to the value
            param->parse(p);
        }
    }

    return 1;
}

int console_param_completion(ContextPtr env, char *cmd) {
    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    LayerPtr lay = screen->mSelectedLayer;
    if(!lay) {
        ::error("no layer currently selected");
        return 0;
    }
    FilterInstancePtr filt = lay->mSelectedFilter;

    LinkList<Parameter> *parameters;
    if(filt) parameters = &filt->parameters;
    else parameters = &lay->parameters;

    // Find completions
    ParameterPtr exactParam = NULL;
    std::list<ParameterPtr> retList;
    LockedLinkList<Parameter> list = LockedLinkList<Parameter>(parameters);
    std::string cmdString(cmd);
    std::transform(cmdString.begin(), cmdString.end(), cmdString.begin(), ::tolower);
    std::copy_if(list.begin(), list.end(), retList.begin(), [&] (ParameterPtr param) {
                     std::string name = param->getName();
                     std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                     if(name == cmdString) {
                         exactParam = param;
                     }
                     return name.compare(cmdString) == 0;
                 });

    if(retList.empty()) return 0;

    if(exactParam != NULL) {
        snprintf(cmd, MAX_CMDLINE, "%s = ", exactParam->getName().c_str());
        return 1;
    }

    if(cmdString.empty()) {
        notice("List available parameters");
    } else {
        notice("List available parameters starting with \"%s\"", cmd);
    }

    int c = 0;
    std::for_each(retList.begin(), retList.end(), [&] (ParameterPtr p) {
                      switch(p->type) {
                      case Parameter::BOOL:
                          ::act("(bool) %s = %s ::  %s", p->getName().c_str(),
                                (*(bool*)p->get() == true) ? "true" : "false",
                                p->getDescription().c_str());
                          break;
                      case Parameter::NUMBER:
                          ::act("(number) %s = %.2f :: %s", p->getName().c_str(),
                                *(float*)p->get(),
                                p->getDescription().c_str());
                          break;
                      case Parameter::STRING:
                          ::act("(string) %s = %s :: %s", p->getName().c_str(), (char*)p->get(), p->getDescription().c_str());
                          break;
                      case Parameter::POSITION: {
                          float *val = (float*)p->get();
                          ::act("(position) %s = %.2f x %.2f :: %s", p->getName().c_str(),
                                val[0], val[1],
                                p->getDescription().c_str());
                      }
                      break;
                      case Parameter::COLOR:
                          ::act("%s (color) %s", p->getName().c_str(), p->getDescription().c_str());
                          break;
                      default:
                          ::error("%s (unknown) %s", p->getName().c_str(), p->getDescription().c_str());
                          break;
                      }
                      ++c;
                  });
    return c;
}

// callbacks used by readline to handle input from console
int console_blit_selection(ContextPtr env, char *cmd) {
    if(!cmd) return 0;
    if(!strlen(cmd)) return 0;

    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    LayerPtr lay = screen->mSelectedLayer;
    if(!lay) {
        ::error("no layer currently selected");
        return 0;
    }
    lay->set_blit(cmd); // now this takes a string!
    return 1;
}

int console_blit_completion(ContextPtr env, char *cmd) {
    if(!cmd) return 0;

    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    LayerPtr lay = screen->mSelectedLayer;
    if(!lay) {
        ::error("no layer currently selected");
        return 0;
    }

    // Find completions
    BlitPtr exactBlit;
    std::list<BlitPtr> retList;
    LockedLinkList<Blit> list = LockedLinkList<Blit>(lay->blitter->blitlist);
    std::string cmdString(cmd);
    std::transform(cmdString.begin(), cmdString.end(), cmdString.begin(), ::tolower);
    std::copy_if(list.begin(), list.end(), retList.begin(), [&] (BlitPtr blit) {
                     std::string name = blit->getName();
                     std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                     if(name == cmdString) {
                         exactBlit = blit;
                     }
                     return name.compare(cmdString) == 0;
                 });

    if(retList.empty()) return 0;

    if(exactBlit != NULL) {
        snprintf(cmd, MAX_CMDLINE, "%s = ", exactBlit->getName().c_str());
        return 1;
    }

    if(cmdString.empty()) {
        notice("List available blits");
    } else {
        notice("List available blits starting with \"%s\"", cmd);
    }

    int c = 0;
    char tmp[256];
    std::for_each(retList.begin(), retList.end(), [&] (BlitPtr b) {
                      if(c % 4 == 0) {
                          if(c != 0) {
                              ::act("%s", tmp);
                          }
                          tmp[0] = '\0';
                      }
                      strncat(tmp, "\t", sizeof(tmp) - 1);
                      strncat(tmp, b->getName().c_str(), sizeof(tmp) - 1);
                      ++c;
                  });
    return c;
}

int console_blit_param_selection(ContextPtr env, char *cmd) {
    if(!cmd) return 0;

    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    LayerPtr lay = screen->mSelectedLayer;
    if(!lay) {
        ::error("no layer currently selected");
        return 0;
    }

    BlitPtr b = lay->current_blit;
    if(!b) {
        ::error("no blit selected on layer %s", lay->getName().c_str());
        return 0;
    }
    // find the values after the first blank space
    char *p;
    for(p = cmd; *p != '\0'; p++)
        if(*p == '=') {
            *p = '\0';
            if(*(p - 1) == ' ')
                *(p - 1) = '\0';
            p++;
            break;
        }

    while(*p == ' ') p++;  // jump all spaces
    if(*p == '\0') return 0;  // no value was given

    LockedLinkList<Parameter> list = LockedLinkList<Parameter>(b->parameters);
    LockedLinkList<Parameter>::iterator it = std::find_if(list.begin(), list.end(), [&](ParameterPtr p) {
                                                              return p->getName() == cmd;
                                                          });
    if(it == list.end()) {
        error("parameter %s not found in blit %s", cmd, b->getName().c_str());
        return 0;
    }

    ParameterPtr param = *it;

    func("parameter %s found in blit %s",  param->getName().c_str(), b->getName().c_str());

    param->parse(p);
    return 1;
}

int console_blit_param_completion(ContextPtr env, char *cmd) {
    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    LayerPtr lay = screen->mSelectedLayer;
    if(!lay) {
        ::error("no layer currently selected");
        return 0;
    }

    BlitPtr b = lay->current_blit;
    if(!b) {
        ::error("no blit selected on layer %s", lay->getName().c_str());
        return 0;
    }

    // Find completions
    // Find completions
    ParameterPtr exactParam;
    std::list<ParameterPtr> retList;
    LockedLinkList<Parameter> list = LockedLinkList<Parameter>(b->parameters);
    std::string cmdString(cmd);
    std::transform(cmdString.begin(), cmdString.end(), cmdString.begin(), ::tolower);
    std::copy_if(list.begin(), list.end(), retList.begin(), [&] (ParameterPtr param) {
                     std::string name = param->getName();
                     std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                     if(name == cmdString) {
                         exactParam = param;
                     }
                     return name.compare(cmdString) == 0;
                 });

    if(retList.empty()) return 0;

    if(exactParam != NULL) {
        snprintf(cmd, MAX_CMDLINE, "%s = ", exactParam->getName().c_str());
        return 1;
    }

    if(cmdString.empty()) {
        notice("List available blit parameters");
    } else {
        notice("List available blit parameters starting with \"%s\"", cmd);
    }

    int c = 0;
    std::for_each(retList.begin(), retList.end(), [&] (ParameterPtr p) {
                      switch(p->type) {
                      case Parameter::BOOL:
                          ::act("(bool) %s = %s ::  %s", p->getName().c_str(),
                                (*(bool*)p->get() == true) ? "true" : "false",
                                p->getDescription().c_str());
                          break;
                      case Parameter::NUMBER:
                          ::act("(number) %s = %.2f :: %s", p->getName().c_str(),
                                *(float*)p->get(),
                                p->getDescription().c_str());
                          break;
                      case Parameter::STRING:
                          ::act("(string) %s = %s :: %s", p->getName().c_str(), (char*)p->get(), p->getDescription().c_str());
                          break;
                      case Parameter::POSITION: {
                          float *val = (float*)p->get();
                          ::act("(position) %s = %.2f x %.2f :: %s", p->getName().c_str(),
                                val[0], val[1],
                                p->getDescription().c_str());
                      }
                      break;
                      case Parameter::COLOR:
                          ::act("%s (color) %s", p->getName().c_str(), p->getDescription().c_str());
                          break;
                      default:
                          ::error("%s (unknown) %s", p->getName().c_str(), p->getDescription().c_str());
                          break;
                      }
                      ++c;
                  });
    return c;
}

int console_filter_selection(ContextPtr env, char *cmd) {
    if(!cmd) return 0;

    LockedLinkList<Filter> list = LockedLinkList<Filter>(env->filters);
    LockedLinkList<Filter>::iterator it = std::find_if(list.begin(), list.end(), [&](FilterPtr filter) {
                                                           return filter->getName() == cmd;
                                                       });
    if(it == list.end()) {
        ::error("filter not found: %s", cmd);
        return 0;
    }
    FilterPtr filt = *it;

    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    LayerPtr lay = screen->mSelectedLayer;
    if(!lay) {
        ::error("no layer selected for effect %s", filt->getName().c_str());
        return 0;
    }

    if(!filt->apply(lay)) {
        ::error("error applying filter %s on layer %s", filt->getName().c_str(), lay->getName().c_str());
        return 0;
    }

    // select automatically the new filter
//  lay->filters.sel(0);
//  ff->sel(true);
    return 1;
}

int console_filter_completion(ContextPtr env, char *cmd) {
    if(!cmd) return 0;

    // Find completions
    FilterPtr exactFilter;
    std::list<FilterPtr> retList;
    LockedLinkList<Filter> list = LockedLinkList<Filter>(env->filters);
    std::string cmdString(cmd);
    std::transform(cmdString.begin(), cmdString.end(), cmdString.begin(), ::tolower);
    std::copy_if(list.begin(), list.end(), retList.begin(), [&] (FilterPtr filter) {
                     std::string name = filter->getName();
                     std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                     if(name == cmdString) {
                         exactFilter = filter;
                     }
                     return name.compare(cmdString) == 0;
                 });

    if(retList.empty()) return 0;

    if(exactFilter != NULL) {
        ::notice("%s :: %s", exactFilter->getName().c_str(), exactFilter->getDescription().c_str());
        snprintf(cmd, MAX_CMDLINE, "%s", exactFilter->getName().c_str());
        return 1;
    }

    if(cmdString.empty()) {
        notice("List available filters");
    } else {
        notice("List available filters with \"%s\"", cmd);
    }

    int c = 0;
    char tmp[256];
    std::for_each(retList.begin(), retList.end(), [&] (FilterPtr f) {
                      if(c % 4 == 0) {
                          if(c != 0) {
                              ::act("%s", tmp);
                          }
                          tmp[0] = '\0';
                      }
                      strncat(tmp, "\t", sizeof(tmp) - 1);
                      strncat(tmp, f->getName().c_str(), sizeof(tmp) - 1);
                      ++c;
                  });
    return c;
}

int console_exec_script(ContextPtr env, char *cmd) {
    struct stat filestatus;

    func("exec_script(%s)", cmd);

    // check that is a good file
    if(stat(cmd, &filestatus) < 0) {
        error("invalid file %s: %s", cmd, strerror(errno));
        return 0;
    } else { // is it a directory?
        if(S_ISDIR(filestatus.st_mode)) {
            error("can't open a directory as a script", cmd);
            return 0;
        }
    }

    env->open_script(cmd);

    return 0;
}

int console_exec_script_command(ContextPtr env, char *cmd) {

    act("> %s", cmd);

    // check that is a good file

    env->parse_js_cmd(cmd);

    return 0;
}

int console_open_layer(ContextPtr env, char *cmd) {
    struct stat filestatus;

    func("open_layer(%s)", cmd);

    // check that is a good file
    if(strncasecmp(cmd, "/dev/video", 10) != 0) {
        if(stat(cmd, &filestatus) < 0) {
            error("invalid file %s: %s", cmd, strerror(errno));
            return 0;
        } else { // is it a directory?
            if(S_ISDIR(filestatus.st_mode)) {
                error("can't open a directory as a layer", cmd);
                return 0;
            }
        }
    }

    // ok the path in cmd should be good here

    LayerPtr l = env->open(cmd);
    if(l) {
        /*
           if(!l->init(env)) {
            error("can't initialize layer");
            delete l;
           } else {
         */
        //	  l->set_fps(env->fps_speed);
        l->start();
        env->mSelectedScreen->add_layer(l);
        l->active = true;
        //    l->fps=env->fps_speed;

        ViewPortPtr screen = env->mSelectedScreen;
        if(!screen) {
            ::error("no screen currently selected");
            return 0;
        }
        int len = LockedLinkList<Layer>(screen->layers).size();
        notice("layer successfully created, now you have %i layers", len);
        return len;
    }
    error("layer creation aborted");
    return 0;
}

#if defined WITH_TEXTLAYER
#include <text_layer.h>
int console_print_text_layer(ContextPtr env, char *cmd) {
    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    LayerPtr lay = screen->mSelectedLayer;
    if(!lay) {
        ::error("no layer currently selected");
        return 0;
    }
    DynamicPointerCast<TextLayer>(lay)->write(cmd);
    return LockedLinkList<Layer>(screen->layers).size();
}

int console_open_text_layer(ContextPtr env, char *cmd) {
    TextLayerPtr txt = MakeShared<TextLayer>();
    if(!txt->init()) {
        error("can't initialize text layer");
        return 0;
    }


    txt->write(cmd);
    txt->start();
    //  txt->set_fps(0);
    env->mSelectedScreen->add_layer(txt);
    txt->active = true;

    notice("layer successfully created with text: %s", cmd);
    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    return LockedLinkList<Layer>(screen->layers).size();
}

#endif

#if defined (HAVE_DARWIN) || defined (HAVE_FREEBSD)
int filebrowse_completion_selector(struct dirent *dir)
#else
int filebrowse_completion_selector(const struct dirent *dir)
#endif
{
    if(dir->d_name[0] == '.')
        if(dir->d_name[1] != '.')
            return(0);  // skip hidden files
    return(1);
}

int console_filebrowse_completion(ContextPtr env, char *cmd) {
    std::list<EntryPtr> files;

    struct stat filestatus;
#if defined (HAVE_DARWIN) || defined (HAVE_FREEBSD)
    struct dirent **filelist;
#else
    struct dirent **filelist;
#endif
    char path[MAX_CMDLINE];
    char needle[MAX_CMDLINE];
    bool incomplete = false;

    if(cmd[0] != '/') // path is relative: prefix our location
        snprintf(path, MAX_CMDLINE, "%s/%s", getenv("PWD"), cmd);
    else // path is absolute
        strncpy(path, cmd, MAX_CMDLINE);

    if(stat(path, &filestatus) < 0) {  // no file there?
        int c = 0;
        // parse backwards to the first '/' and zero it,
        // store the word of the right part in needle
        for(c = strlen(path); path[c] != '/' && c > 0; c--) ;
        strncpy(needle, &path[c + 1], MAX_CMDLINE);
        path[c + 1] = '\0';
        incomplete = true;

        if(stat(path, &filestatus) < 0) { // yet no valid file?
            error("error on file completion path %s: %s", path, strerror(errno));
            return 0;
        }

    } else { // we have a file!

        if(S_ISREG(filestatus.st_mode))
            return 1;  // is a regular file!

        // is it a directory? then append the trailing slash
        if(S_ISDIR(filestatus.st_mode)) {
            int c = strlen(path);
            if(path[c - 1] != '/') {
                path[c] = '/';
                path[c + 1] = '\0';
            }
        }

        strncpy(cmd, path, MAX_CMDLINE);

    }
    func("file completion: %s", cmd);
    // at this point in path there should be something valid
    int found = scandir
                    (path, &filelist,
                    filebrowse_completion_selector, alphasort);

    if(found < 0) {
        error("filebrowse_completion: scandir: %s", strerror(errno));
        return 0;
    }

    for(int c = found - 1; c > 0; c--) { // insert each entry found in a linklist
        EntryPtr e = MakeShared<Entry>();
        e->setName(filelist[c]->d_name);
        files.push_back(e);
    }

    int c = 0; // counter for entries found

    if(incomplete) {
        // list all files in directory *path starting with *needle
        // Find completions
        EntryPtr exactEntry;
        std::list<EntryPtr> retList;
        std::string cmdString(needle);
        std::transform(cmdString.begin(), cmdString.end(), cmdString.begin(), ::tolower);
        std::copy_if(files.begin(), files.end(), retList.begin(), [&] (EntryPtr entry) {
                         std::string name = entry->getName();
                         std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                         if(name == cmdString) {
                             exactEntry = entry;
                         }
                         return name.compare(cmdString) == 0;
                     });

        c = retList.size();
        if(exactEntry != NULL) {
            snprintf(cmd, MAX_CMDLINE, "%s%s", path, exactEntry->getName().c_str());
        } else {
            notice("list of %s* files in %s:", needle, path);
            std::for_each(retList.begin(), retList.end(), [&] (EntryPtr entry) {
                              ::act(" %s", entry->getName().c_str());
                          });
        }
    } else {
        // list all entries
        notice("list of all files in %s:", path);
        std::for_each(files.begin(), files.end(), [&] (EntryPtr e) {
                          ::act("%s", e->getName().c_str());
                      });
    }

    return(c);
}

// static int set_blit_value(ContextPtr env, char *cmd) {
//   int val;
//   int c;
//   if(!sscanf(cmd,"%u",&val)) {
//     error("error parsing input: %s",cmd);
//     return 0;
//   }
//   func("value parsed: %s in %d",cmd,val);
//   Layer *lay = (Layer*)env->screens.selected()->layers.begin();
//   if(!lay) return 0;
//   /* set value in all blits selected
//      (supports multiple selection) */
//   for(c=0 ; lay ; lay = (Layer*)lay->next) {
//     if(!lay->select) continue;
//     //    lay->blitter.fade_value(1,val);
//   }

//   return 1;
// }

int console_generator_completion(ContextPtr env, char *cmd) {
    if(!cmd) return 0;

    LockedLinkList<Filter> list =  LockedLinkList<Filter>(env->generators);
    FilterPtr exactGenerator;
    std::list<FilterPtr> retList;
    std::string cmdString(cmd);
    std::transform(cmdString.begin(), cmdString.end(), cmdString.begin(), ::tolower);
    std::copy_if(list.begin(), list.end(), retList.begin(), [&] (FilterPtr generator) {
                     std::string name = generator->getName();
                     std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                     if(name == cmdString) {
                         exactGenerator = generator;
                     }
                     return name.compare(cmdString) == 0;
                 });

    if(retList.empty()) return 0;

    if(exactGenerator != NULL) {
        ::notice("%s :: %s", exactGenerator->getName().c_str(), exactGenerator->getDescription().c_str());
        snprintf(cmd, MAX_CMDLINE, "%s", exactGenerator->getName().c_str());
        return 1;
    }

    int c = 0;
    char tmp[256];
    std::for_each(retList.begin(), retList.end(), [&] (FilterPtr f) {
                      if(c % 4 == 0) {
                          if(c != 0) {
                              ::act("%s", tmp);
                          }
                          tmp[0] = '\0';
                      }
                      strncat(tmp, "\t", sizeof(tmp) - 1);
                      strncat(tmp, f->getName().c_str(), sizeof(tmp) - 1);
                      ++c;
                  });
    return c;
}

int console_generator_selection(ContextPtr env, char *cmd) {
    GeneratorLayerPtr tmp = MakeShared<GeneratorLayer>();
    if(!tmp) return 0;

    ViewPortPtr screen = env->mSelectedScreen;
    if(!screen) {
        ::error("no screen currently selected");
        return 0;
    }
    if(!tmp->init(env->mSelectedScreen->geo.w,
                  env->mSelectedScreen->geo.h,
                  env->mSelectedScreen->geo.bpp)) {
        error("can't initialize generator layer");
        return 0;
    }
    // this is something specific to the generator layer
    // it needs this from the environment..
    tmp->register_generators(&env->generators);

    if(!tmp->open(cmd)) {
        error("generator %s is not found", cmd);
        return 0;
    }

    tmp->start();
    //  tmp->set_fps(env->fps_speed);
    env->mSelectedScreen->add_layer(tmp);
    tmp->active = true;

    notice("generator %s successfully created", tmp->getName().c_str());
    return 1;
}

