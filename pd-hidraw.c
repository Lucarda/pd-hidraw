

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "m_pd.h"


typedef struct _hidraw {
  t_object  x_obj;
  t_canvas  *x_canvas;
  t_outlet *x_outlet1;
   
  } t_hidraw;


t_class *hidraw_class;



void hidraw_main(t_hidraw *x, t_symbol *s, int argc, t_atom *argv) {
	
	
}

static void hidraw_pdversion(t_hidraw *x) {
	
	t_atom version[5];
	
	SETSYMBOL(&version[0], gensym("Pd"));
	SETFLOAT(&version[1], (t_float)PD_MAJOR_VERSION);
	SETFLOAT(&version[2], (t_float)PD_MINOR_VERSION);
	SETFLOAT(&version[3], (t_float)PD_BUGFIX_VERSION);
	SETSYMBOL(&version[4], gensym(PD_TEST_VERSION));
	
	outlet_list(x->x_outlet1, &s_list, 5, version);
	
}

static void hidraw_free(t_hidraw *x) {
	
}



static void *hidraw_new(void)
{
  t_hidraw *x = (t_hidraw *)pd_new(hidraw_class);

  x->x_canvas = canvas_getcurrent();
  
  x->x_outlet1 = outlet_new(&x->x_obj, 0);
 
  return (void *)x;
}




void hidraw_setup(void) {

  hidraw_class = class_new(gensym("hidraw"),      
			       (t_newmethod)hidraw_new,
			       (t_method)hidraw_free,                          
			       sizeof(t_hidraw),       
			       CLASS_DEFAULT,				   
			       0);                        

    
  class_addmethod(hidraw_class, (t_method)hidraw_main, gensym("test"), A_GIMME, 0);
  class_addmethod(hidraw_class, (t_method)hidraw_pdversion, gensym("version"), 0);

}
