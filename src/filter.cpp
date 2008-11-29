/*  FreeJ - New Freior based Filter class
 *  (c) Copyright 2007 Denis Rojo <jaromil@dyne.org>
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

$Id: $

 */

#include <config.h>
#include <layer.h>
#include <filter.h>

#include <frei0r_freej.h>
#include <freeframe_freej.h>

#include <jutils.h>


Parameter::Parameter(int param_type)
  : Entry() {
  switch(param_type) {
  case PARAM_BOOL:
    value = calloc(1, sizeof(bool));
    break;
  case PARAM_NUMBER:
    value = calloc(1, sizeof(double));
    break;
  case PARAM_COLOR:
    value = calloc(3, sizeof(double));
    break;
  case PARAM_POSITION:
    value = calloc(2, sizeof(double));
    break;
  case PARAM_STRING:
    value = calloc(512, sizeof(char));
    break;
  default:
    error("parameter initialized with unknown type: %u", param_type);
  }

  type = param_type;

  layer_set_f = NULL;
  layer_get_f = NULL;
  filter_set_f = NULL;
  filter_get_f = NULL;
}

Parameter::~Parameter() {
  free(value);
}

bool Parameter::set(void *val) {
  ////////////////////////////////////////
  if(type == PARAM_NUMBER) {

    func("set_parameter number");
    *(float*)value = *(float*)val;
    
    //////////////////////////////////////
  } else if(type == PARAM_BOOL) {

    func("set_parameter bool");
    *(bool*)value = *(bool*)val;
    
    //    act("filter %s parameter %s set to: %e", name, param->name, (double*)value);
    //////////////////////////////////////
  } else if (type == PARAM_POSITION) {
    
    ((double*)value)[0] = ((double*)val)[0];
    ((double*)value)[1] = ((double*)val)[1];
    
    //////////////////////////////////////
  } else if (type==PARAM_COLOR) {

    ((double*)value)[0] = ((double*)val)[0];
    ((double*)value)[1] = ((double*)val)[1];
    ((double*)value)[2] = ((double*)val)[2];
    
    //////////////////////////////////////
  } else if (type==PARAM_STRING) {
    
    strcpy((char*)value, (char*)val);

  } else {
    error("attempt to set value for a parameter of unknown type: %u", type);
    return false;
  }
  
  return true;
}

// TODO VERIFY ALL TYPES
bool Parameter::parse(char *p) {
  // parse the strings into value


    //////////////////////////////////////
  if(type == PARAM_NUMBER) {

    func("parsing number parameter");
    if( sscanf(p, "%le", (double*)value) < 1 ) {
      error("error parsing value [%s] for parameter %s", p, name);
      return false;
    }
    func("parameter %s parsed to %g",p, *(double*)value);

    
    //////////////////////////////////////
  } else if(type == PARAM_BOOL) {

    func("parsing bool parameter");
    char *pp;
    for( pp=p; (*pp!='1') & (*pp!='0') ; pp++) {
      if(pp-p>128) {
	error("error parsing value [%s] for parameter %s", p, name);
	return false;
      }
    }
    if(*pp=='1') *(bool*)value = true;
    if(*pp=='0') *(bool*)value = false;
    func("parameter %s parsed to %s",p, ( *(bool*)value == true ) ? "true" : "false" );
 

    //////////////////////////////////////    
  } else if(type == PARAM_POSITION) {

    double *val;
    
    val = (double*)value;
    if( sscanf(p, "%le %le", &val[0], &val[1]) < 1 ) {
      error("error parsing position [%s] for parameter %s", p, name);
      return false;
    }
    func("parameter %s parsed to %g %g",p, val[0], val[1]);


    //////////////////////////////////////
  } else if(type == PARAM_COLOR) {
    
    double *val;

    val = (double*)value;
    if( sscanf(p, "%le %le %le", &val[0], &val[1], &val[2]) < 1 ) {
      error("error parsing position [%s] for parameter %s", p, name);
      return false;
    }
    func("parameter %s parsed to %le %le %le",p, val[0], val[1], val[2]);


    //////////////////////////////////////
  } else {
    error("attempt to set value for a parameter of unknown type: %u", type);
    return false;
  }

  return true;

}

/// frei0r parameter callbacks
static void get_frei0r_parameter(FilterInstance *filt, Parameter *param, int idx) {
  Freior *f = filt->proto->freior;

  switch(f->param_infos[idx-1].type) {

    // idx-1 because frei0r's index starts from 0
  case F0R_PARAM_BOOL:
    (*f->f0r_get_param_value)(filt->core, (f0r_param_t)param->value, idx-1);
    func("bool value is %s",(*(bool*)param->value==true) ? "true" : "false");
    break;

  case F0R_PARAM_DOUBLE:
    (*f->f0r_get_param_value)(filt->core, (f0r_param_t)param->value, idx-1);
    func("number value is %g",*(double*)param->value);
    break;

  case F0R_PARAM_COLOR:
    { f0r_param_color *color = new f0r_param_color;
      (*f->f0r_get_param_value)(filt->core, (f0r_param_t)color, idx-1);
      ((double*)param->value)[0] = (double)color->r;
      ((double*)param->value)[1] = (double)color->g;
      ((double*)param->value)[2] = (double)color->b;
      delete color;
    } break;

  case F0R_PARAM_POSITION:
    { f0r_param_position *position = new f0r_param_position;
      (*f->f0r_get_param_value)(filt->core, (f0r_param_t)position, idx-1);
      ((double*)param->value)[0] = (double)position->x;
      ((double*)param->value)[1] = (double)position->y;
      delete position;
    } break;

  default:

    error("Unrecognized parameter type %u for get_parameter_value",
	  f->param_infos[idx].type);
  }  
}

static void set_frei0r_parameter(FilterInstance *filt, Parameter *param, int idx) {

  func("set_frei0r_param callback on %s for parameter %s at pos %u",
       filt->proto->name, param->name, idx);

  Freior *f = filt->proto->freior;
  double *val = (double*)param->value;

  switch(f->param_infos[idx-1].type) {
    
    // idx-1 because frei0r's index starts from 0
  case F0R_PARAM_BOOL:

    func("bool value is %s",(*(bool*)param->value==true) ? "true" : "false");

    (*f->f0r_set_param_value)
      (filt->core, new f0r_param_bool(*(bool*)param->value), idx-1);

    break;
    
  case F0R_PARAM_DOUBLE:
    func("number value is %g",*(double*)param->value);
    (*f->f0r_set_param_value)(filt->core, new f0r_param_double( *(double*)param->value), idx-1);
    break;

  case F0R_PARAM_COLOR:
    { f0r_param_color *color = new f0r_param_color;
      color->r = val[0];
      color->g = val[1];
      color->b = val[2];
      (*f->f0r_set_param_value)(filt->core, color, idx-1);
      // QUAAA: should we delete the new allocated object? -jrml
    } break;

  case F0R_PARAM_POSITION:
    { f0r_param_position *position = new f0r_param_position;
      position->x = val[0];
      position->y = val[1];
      (*f->f0r_set_param_value)(filt->core, position, idx-1);
    } break;

  default:

    error("Unrecognized parameter type %u for set_parameter_value",
	  f->param_infos[idx].type);

  }

}

Filter::Filter(int type, void *filt) 
  : Entry() {
  int i;

  initialized = false;
  active = false;
  inuse = false;

  freior = NULL;
  freeframe = NULL;

  bytesize = 0;

  // critical errors:
  if(!filt) error("Filter constructor received a NULL object");
  //  if(!filt->opened) error("Filter constructor received a Freior object that is not open");

  switch(type) {

  case FREIOR:
    freior = (Freior*)filt;
    (*freior->f0r_init)();

    // Get the list of params.
    freior->param_infos.resize(freior->info.num_params);
    for (i = 0; i < freior->info.num_params; ++i) {
      
      (*freior->f0r_get_param_info)(&freior->param_infos[i], i);
      
      Parameter *param = new Parameter(freior->param_infos[i].type);
      strncpy(param->name, freior->param_infos[i].name, 255);
      
      param->description = freior->param_infos[i].explanation;
      param->filter_set_f = set_frei0r_parameter;
      param->filter_get_f = get_frei0r_parameter;
      parameters.append(param);
    }

    if(get_debug()>2)
      freior->print_info();
    
    set_name((char*)freior->info.name);
    
    break;

  case FREEFRAME:
    freeframe = (Freeframe*)filt;

    set_name((char*)freeframe->info->pluginName);

    // init freeframe filter
    if(freeframe->main(FF_INITIALISE, NULL, 0).ivalue == FF_FAIL)
      error("cannot initialise freeframe plugin %s",name);
    
    // TODO freeframe parameters
    
    if(get_debug()>2)
      freeframe->print_info();
    
    break;

  default:
    error("filter type %u not supported",type);
    return;
  }

  backend = type;

}

Filter::~Filter() {
  if(freior) delete freior;
  if(freeframe) delete freeframe;
}

FilterInstance *Filter::apply(Layer *lay) {

  FilterInstance *instance;
  instance = new FilterInstance(this);

  if(freior) {
    instance->core = (void*)(*freior->f0r_construct)(lay->geo.w, lay->geo.h);
  }

  if(freeframe) {
    VideoInfoStruct vidinfo;
    vidinfo.frameWidth = lay->geo.w;
    vidinfo.frameHeight = lay->geo.h;
    vidinfo.orientation = 1;
    vidinfo.bitDepth = FF_CAP_32BITVIDEO;
    instance->intcore = freeframe->main(FF_INSTANTIATE, &vidinfo, 0).ivalue;
    if(instance->intcore == FF_FAIL) {
      error("Filter %s cannot be instantiated", name);
      delete instance;
      return NULL;
    }
  }

  
  errno=0;
  instance->outframe = (uint32_t*) calloc(lay->geo.size, 1);
  if(errno != 0) {
    error("calloc outframe failed (%i) applying filter %s",errno, name);
    error("Filter %s cannot be instantiated", name);
    delete instance;
    return NULL;
  }
  
  bytesize = lay->geo.size;

  lay->filters.append(instance);

  act("initialized filter %s on layer %s", name, lay->name);

  // here maybe keep a list of layers that possess instantiantions of this filter?

  return instance;

}

const char *Filter::description() {
  const char *ret;
  if(backend==FREIOR) {
    ret = freior->info.explanation;
  } else if(backend==FREEFRAME) {
    // TODO freeframe has no extentedinfostruct returned!?
    ret = "freeframe VFX";
  }
  return ret;

}

int Filter::get_parameter_type(int i) {
  int ret;
  if(backend==FREIOR) {
    ret =  freior->param_infos[i].type;
  } else if(backend==FREEFRAME) {
    // TODO freeframe
  }
  return ret;

}

char *Filter::get_parameter_description(int i) {
  char *ret;
  if(backend==FREIOR) {
    ret = (char*)freior->param_infos[i].explanation;
  } else if(backend==FREEFRAME) {
    // TODO freeframe
  }
  return ret;
}

void Filter::destruct(FilterInstance *inst) {

  if(backend==FREIOR) {

    if(inst->core) {
      (*freior->f0r_destruct)((f0r_instance_t*)inst->core);
      inst->core = NULL;
    }

  } else if(backend==FREEFRAME) {

    freeframe->main(FF_DEINSTANTIATE, NULL, inst->intcore);

  }
}

void Filter::update(FilterInstance *inst, double time, uint32_t *inframe, uint32_t *outframe) {

  if(backend==FREIOR) {

    (*freior->f0r_update)((f0r_instance_t*)inst->core, time, inframe, outframe);

  } else if(backend==FREEFRAME) {

    jmemcpy(outframe,inframe,bytesize);

    freeframe->main(FF_PROCESSFRAME, (void*)outframe, inst->intcore);

  }

}


FilterInstance::FilterInstance(Filter *fr)
  : Entry() {
  func("creating instance for filter %s",fr->name);

  proto = fr;
  
  core = NULL;
  intcore = 0;
  outframe = NULL;
  
  active = true;

  set_name(proto->name);
}

FilterInstance::~FilterInstance() {
  func("~FilterInstance");

  if(proto)
    proto->destruct(this);

  if(outframe) free(outframe);

}

uint32_t *FilterInstance::process(float fps, uint32_t *inframe) {
  if(!proto) {
    error("void filter instance was called for process: %p", this);
    return inframe;
  }

  proto->update(this, fps, inframe, outframe);
  return outframe;
}

bool FilterInstance::set_parameter(int idx) {
  Parameter *param;
  param = (Parameter*)proto->parameters[idx];

  if( ! param) {
    error("parameter %s not found in filter %s", param->name, proto->name );
    return false;
  } else 
    func("parameter %s found in filter %s at position %u",
	 param->name, proto->name, idx);

  if(!param->filter_set_f) {
    error("no filter callback function registered in this parameter");
    return false;
  }

  (*param->filter_set_f)(this, param, idx);

  return true;
}

bool FilterInstance::get_parameter(int idx) {
  Parameter *param;
  param = (Parameter*)proto->parameters[idx];

  if( ! param) {
    error("parameter %s not found in filter %s", param->name, proto->name );
    return false;
  } else 
    func("parameter %s found in filter %s at position %u",
	 param->name, proto->name, idx);

  if(!param->filter_get_f) {
    error("no filter callback function registered in this parameter");
    return false;
  }

  (*param->filter_get_f)(this, param, idx);

  return true;
}

    
    
