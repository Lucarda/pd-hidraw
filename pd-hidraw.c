

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "m_pd.h"
#include <hidapi.h>


// Headers needed for sleeping.
#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif


// Fallback/example
#ifndef HID_API_MAKE_VERSION
#define HID_API_MAKE_VERSION(mj, mn, p) (((mj) << 24) | ((mn) << 8) | (p))
#endif
#ifndef HID_API_VERSION
#define HID_API_VERSION HID_API_MAKE_VERSION(HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH)
#endif


// Sample using platform-specific headers
#if defined(__APPLE__) 
#include <hidapi_darwin.h>
#endif

#if defined(_WIN32) 
#include <hidapi_winapi.h>
#endif

#define	HIDRAW_MAJOR_VERSION 0
#define	HIDRAW_MINOR_VERSION 0
#define	HIDRAW_BUGFIX_VERSION 0

#define MAXHIDS 50

typedef struct _hidraw {
  t_object  x_obj;
  struct hid_device_info *devs;
  unsigned short foundPID[MAXHIDS];
  unsigned short foundVID[MAXHIDS];
  char currentpdhid;
  char isadeviceopen;
  hid_device *handle;
  t_canvas  *x_canvas;
  t_outlet *x_outlet1;
   
  } t_hidraw;


t_class *hidraw_class;

static void print_device(struct hid_device_info *cur_dev) {
	post("\n\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
	post("  Manufacturer: %ls", cur_dev->manufacturer_string);
	post("  Product:      %ls", cur_dev->product_string);
	post("  Release:      %hx", cur_dev->release_number);
	post("  Interface:    %d",  cur_dev->interface_number);
	post("  Usage (page): 0x%hx (0x%hx)", cur_dev->usage, cur_dev->usage_page);
	post(" ");
}

static void print_devices(struct hid_device_info *cur_dev, t_hidraw *x) {
	
	int i = 0;
	
	while (cur_dev) {
		
		if (x->foundVID[i-1] != cur_dev->vendor_id && x->foundPID[i-1] != cur_dev->product_id) {
		    post("-----------\nPd device enum: %d", i);
		    x->foundVID[i] = cur_dev->vendor_id;
		    x->foundPID[i] = cur_dev->product_id;
		    i++;
		}
		print_device(cur_dev);
		cur_dev = cur_dev->next;
	}
}

static void hidraw_opendevice(t_hidraw *x, t_float hidn) {
	
	
	unsigned short pdenum = (unsigned short) hidn;
	
	// Open the device using the VID, PID,
	// and optionally the Serial number.
	////handle = hid_open(0x4d8, 0x3f, L"12345");
	x->handle = hid_open(x->foundVID[pdenum], x->foundPID[pdenum], NULL);
	////////////we cant open mouse or keyboard on Windows. Security stuff.
	
	//x->handle = hid_open_path("\\?\HID#VID_13BA&PID_0017&MI_01&Col02#8&22796146&0&0001#{4d1e55b2-f16f-11cf-88cb-001111000030}");
	
	if (!x->handle) {
		post("unable to open device %d", (int)hidn);
 		return;
	}
	
    x->isadeviceopen = 1;
	
	
	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(x->handle, 1);
	
}

static void hidraw_closedevice(t_hidraw *x) {

    hid_close(x->handle);
	x->isadeviceopen = 0;
	
}

static void hidraw_listhids(t_hidraw *x) {
	
    x->devs = hid_enumerate(0x0, 0x0);
	print_devices(x->devs, x);
	hid_free_enumeration(x->devs);
}

static void hidraw_main(t_hidraw *x) {

	int res;
	unsigned char buf[256];
	#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	
	int i;

	if (!x->isadeviceopen){
		post("no device opened yet");
	    return;
	}
	
///////////////////////////////	
	
	// Set up the command buffer.
	memset(buf,0x00,sizeof(buf));
	buf[0] = 0x01;
	buf[1] = 0x81;


	
	
	
	
	// Request state (cmd 0x81). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x81;
	res = hid_write(x->handle, buf, 3);
	printf("res write: %d\n", res);
	if (res < 0) {
		printf("Unable to write()/2: %ls\n", hid_error(x->handle));
	}
	
	

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	i = 0;
	while (res == 0) {
		res = hid_read(x->handle, buf, sizeof(buf));
		
		if (res == 0) {
			printf("waiting...\n");
		}
		if (res < 0) {
			printf("Unable to read(): %ls\n", hid_error(x->handle));
			break;
		}

		i++;
		
		
		if (i >= 10) { // 10 tries by 500 ms - 5 seconds of waiting
			printf("read() timeout\n");
			break;
		}
        
		
	if (res > 0) {
		printf("Data read:\n   ");
		// Print out the returned buffer.
		for (i = 0; i < res; i++)
			printf("%02x ", (unsigned int) buf[i]);
		printf("\n");
	}
  }


	
	
	
	
	
	

	
	
	


	
}

static void hidraw_pdversion(t_hidraw *x) {
	
	t_atom version[8];
	
	
	SETSYMBOL(&version[0], gensym("hidraw"));
	SETFLOAT(&version[1], (t_float)HIDRAW_MAJOR_VERSION);
	SETFLOAT(&version[2], (t_float)HIDRAW_MINOR_VERSION);
	SETFLOAT(&version[3], (t_float)HIDRAW_BUGFIX_VERSION);
	SETSYMBOL(&version[4], gensym("hidapi"));
	SETFLOAT(&version[5], (t_float)HID_API_VERSION_MAJOR);
	SETFLOAT(&version[6], (t_float)HID_API_VERSION_MINOR);
	SETFLOAT(&version[7], (t_float)HID_API_VERSION_PATCH);
	
	outlet_list(x->x_outlet1, &s_list, 8, version);
	
	
	int i;
	for (i=0; i < MAXHIDS; i++) {
		printf("%d %d\n",	 x->foundVID[i], x->foundPID[i]);
	}
	
}

static void hidraw_free(t_hidraw *x) {

	
	hid_close(x->handle);
	/* Free static HIDAPI objects. */
	hid_exit();
	
}



static void *hidraw_new(void)
{
    t_hidraw *x = (t_hidraw *)pd_new(hidraw_class);

    x->x_canvas = canvas_getcurrent();
  
    x->x_outlet1 = outlet_new(&x->x_obj, 0);
  
    x->foundVID[0] = 0;
    x->foundPID[0] = 0;
	x->currentpdhid = 0;
	x->isadeviceopen = 0;
  
    hid_init();

#if defined(__APPLE__) 
	// To work properly needs to be called before hid_open/hid_open_path after hid_init.
	// Best/recommended option - call it right after hid_init.
	hid_darwin_set_open_exclusive(0);
#endif
 
  return (void *)x;
}




void hidraw_setup(void) {

  hidraw_class = class_new(gensym("hidraw"),      
			       (t_newmethod)hidraw_new,
			       (t_method)hidraw_free,                          
			       sizeof(t_hidraw),       
			       CLASS_DEFAULT,				   
			       0);                        

    
  class_addmethod(hidraw_class, (t_method)hidraw_main, gensym("test"), 0);
  class_addmethod(hidraw_class, (t_method)hidraw_listhids, gensym("listdevices"), 0);
  class_addmethod(hidraw_class, (t_method)hidraw_pdversion, gensym("version"), 0);
  class_addmethod(hidraw_class, (t_method)hidraw_opendevice, gensym("openhid"), A_FLOAT, 0);
  class_addmethod(hidraw_class, (t_method)hidraw_closedevice, gensym("closehid"), 0);

}
