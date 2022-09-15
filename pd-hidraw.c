/* pd-hidraw.c
 * By Lucas Cordiviola <lucarda27@hotmail.com> 2022
*/

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


// Sample using platform-specific headers
#if defined(__APPLE__) 
#include <hidapi_darwin.h>
#endif

#if defined(_WIN32) 
#include <hidapi_winapi.h>
#endif

#define HIDRAW_MAJOR_VERSION 0
#define HIDRAW_MINOR_VERSION 1
#define HIDRAW_BUGFIX_VERSION 0

#define MAXHIDS 50

typedef struct _hidraw {
    t_object  x_obj;
    struct hid_device_info *devs;
    unsigned short foundPID[MAXHIDS];
    unsigned short foundVID[MAXHIDS];
    unsigned short targetPID;
    unsigned short targetVID;
    unsigned char readbuf[256];
    hid_device *handle;
    t_canvas  *x_canvas;
    t_outlet *bytes_out, *readstatus;
   
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
            post("device VID PID (shown in decimal notation): %d %d", cur_dev->vendor_id, 
                cur_dev->product_id);
            x->foundVID[i] = cur_dev->vendor_id;
            x->foundPID[i] = cur_dev->product_id;
            i++;
        }
        print_device(cur_dev);
        cur_dev = cur_dev->next;
    }
}

static void hidraw_open(t_hidraw *x) {
    
    if (x->handle){
        hid_close(x->handle);
        x->handle = NULL;
        post("hidraw: closing previously opened device ...");
    } 
    
    // Open the device using the VID, PID,
    // and optionally the Serial number.
    x->handle = hid_open(x->targetVID, x->targetPID, NULL);
    
    if (!x->handle) {
        post("hidraw: unable to open device: %ls\n", hid_error(x->handle));
        x->handle = NULL;
        return;
    }
    
    post("hidraw: successfully opened the device");
    
    // Set the hid_read() function to be non-blocking.
    hid_set_nonblocking(x->handle, 1);
    
    // Set up buffers.
    memset(x->readbuf,0x00,sizeof(x->readbuf));
    
}

static void hidraw_opendevice(t_hidraw *x, t_float hidn) {
    
    unsigned short pdenum = (unsigned short) hidn;
    x->targetVID = x->foundVID[pdenum];
    x->targetPID = x->foundPID[pdenum];
    hidraw_open(x); 
}

static void hidraw_opendevice_vidpid(t_hidraw *x, t_float vid, t_float pid) {

    x->targetVID = (unsigned short) vid;
    x->targetPID = (unsigned short) pid;
    hidraw_open(x); 
}

static void hidraw_closedevice(t_hidraw *x) {

    if (x->handle){
        hid_close(x->handle);
        x->handle = NULL;
        post("hidraw: device closed");
    }   
}

static void hidraw_listhids(t_hidraw *x) {
    
    x->devs = hid_enumerate(0x0, 0x0);
    print_devices(x->devs, x);
    hid_free_enumeration(x->devs);
}


static void hidraw_poll(t_hidraw *x) {

    int res;
    t_atom out[256];

    if (!x->handle){
        post("hidraw: no device opened yet");
        return;
    }

    // Read requested state. hid_read() has been set to be
    // non-blocking by the call to hid_set_nonblocking() above.
    // This loop demonstrates the non-blocking nature of hid_read().
    res = 0;
    
    res = hid_read(x->handle, x->readbuf, sizeof(x->readbuf));
    
    if (res < 0) {
        post("hidraw: unable to read(): %ls\n", hid_error(x->handle));
        outlet_float(x->readstatus, -1);
        return;
    }
    
    if (res == 0) {
        outlet_float(x->readstatus, 1); //waiting...
        return;
    }

    for (int i = 0; i < res; i++) {
        SETFLOAT(out+i, x->readbuf[i]);
    }
    outlet_float(x->readstatus, 2);
    outlet_list(x->bytes_out, &s_list, res, out);
    
}

static void hidraw_pdversion(void) {
    
    post("---");
    post("  hidraw v%d.%d.%d", HIDRAW_MAJOR_VERSION, HIDRAW_MINOR_VERSION, HIDRAW_BUGFIX_VERSION);
    post("  hidapi v%d.%d.%d", HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH);
    post("---");
}

static void hidraw_free(t_hidraw *x) {

    if (x->handle){
        hid_close(x->handle);
    }  
}

static void hidraw_cleanup(t_class *c) {
    
    /* Free static HIDAPI objects. */
    hid_exit();
    
}



static void *hidraw_new(void)
{
    t_hidraw *x = (t_hidraw *)pd_new(hidraw_class);

    x->x_canvas = canvas_getcurrent();
  
    x->bytes_out = outlet_new(&x->x_obj, &s_list);
    x->readstatus = outlet_new(&x->x_obj, &s_float);
  
    x->foundVID[0] = 0;
    x->foundPID[0] = 0;
    x->handle = NULL;

    return (void *)x;
}




void hidraw_setup(void) {

    hidraw_class = class_new(gensym("hidraw"),
                   (t_newmethod)hidraw_new,
                   (t_method)hidraw_free,
                   sizeof(t_hidraw),
                   CLASS_DEFAULT,
                   0);

    
    class_setfreefn(hidraw_class, hidraw_cleanup);
    class_addmethod(hidraw_class, (t_method)hidraw_listhids, gensym("listdevices"), 0);
    class_addmethod(hidraw_class, (t_method)hidraw_opendevice, gensym("openhid"), A_FLOAT, 0);
    class_addmethod(hidraw_class, (t_method)hidraw_opendevice_vidpid, gensym("openhid-vidpid"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(hidraw_class, (t_method)hidraw_closedevice, gensym("closehid"), 0);
    class_addbang(hidraw_class, hidraw_poll);
    
    hidraw_pdversion();        
    hid_init();

#if defined(__APPLE__) 
    // To work properly needs to be called before hid_open/hid_open_path after hid_init.
    // Best/recommended option - call it right after hid_init.
    hid_darwin_set_open_exclusive(0);
#endif

}
