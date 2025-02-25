/* pd-hidraw.c
 * By Lucas Cordiviola <lucarda27@hotmail.com> 2022-2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "m_pd.h"
#include <hidapi.h>

// Sample using platform-specific headers
#if defined(__APPLE__)
#include <hidapi_darwin.h>
#endif

#if defined(_WIN32)
#include <hidapi_winapi.h>
#endif

#define HIDRAW_MAJOR_VERSION 0
#define HIDRAW_MINOR_VERSION 2
#define HIDRAW_BUGFIX_VERSION 2

#define MAXHIDS 50

typedef struct _hidraw {
    t_object  x_obj;
    struct hid_device_info *devs;
    unsigned short targetPID;
    unsigned short targetVID;
    unsigned char readbuf[256];
    char hidpath[MAXHIDS][256];
    char targetpath[256];
    int readlen;
    char devlistdone;
    int ndevices;
    t_float polltime;
    hid_device *handle;
    t_outlet *bytes_out, *readstatus, *info;
    t_clock *hidclock;
    char infomode;

} t_hidraw;


t_class *hidraw_class;


static void hidraw_outlet_info(struct hid_device_info *cur_dev, t_hidraw *x)
{
    t_atom devs[6];
    char *empty = "empty";

    int n_man, n_prod, n_serial;
    char *manufacturer, *product, *serial;

    n_man = wcslen(cur_dev->manufacturer_string);
    manufacturer = getbytes(n_man);
    wcstombs(manufacturer, cur_dev->manufacturer_string, n_man);

    n_prod = wcslen(cur_dev->product_string);
    product = getbytes(n_prod);
    wcstombs(product,cur_dev->product_string, n_prod);

    n_serial = wcslen(cur_dev->serial_number);
    serial = getbytes(n_serial);
    wcstombs(serial, cur_dev->serial_number, n_serial);

    if (strlen(manufacturer) == 0) SETSYMBOL (devs+0, gensym(empty));
    else SETSYMBOL(devs+0, gensym(manufacturer));
    if (strlen(product) == 0) SETSYMBOL (devs+1, gensym(empty));
    else SETSYMBOL(devs+1, gensym(product));
    if (strlen(serial) == 0) SETSYMBOL (devs+2, gensym(empty));
    else SETSYMBOL(devs+2, gensym(serial));

    SETFLOAT(devs+3, (t_float)cur_dev->vendor_id);
    SETFLOAT(devs+4, (t_float)cur_dev->product_id);

    SETSYMBOL(devs+5, gensym(cur_dev->path));

    outlet_list(x->info, 0, 6, devs);

    freebytes(manufacturer, n_man);
    freebytes(product, n_prod);
    freebytes(serial, n_serial);
}


static void print_device(struct hid_device_info *cur_dev, int devn)
{
    post("-----------");
    post("Pd device enum: %d", devn);
    post("device VID PID (shown in decimal notation): %d %d", cur_dev->vendor_id,
        cur_dev->product_id);
    post("\n\n  type: %04hx %04hx", cur_dev->vendor_id,
        cur_dev->product_id);
    post("  path: %s", cur_dev->path);
    post("  serial_number: %ls", cur_dev->serial_number);
    post("  Manufacturer: %ls", cur_dev->manufacturer_string);
    post("  Product:      %ls", cur_dev->product_string);
    post("  Release:      %hx", cur_dev->release_number);
    post("  Interface:    %d",  cur_dev->interface_number);
    post("  Usage (page): 0x%hx (0x%hx)", cur_dev->usage, cur_dev->usage_page);
    post(" ");
}

static void list_devices(struct hid_device_info *cur_dev, t_hidraw *x)
{
    int i = 1; // start enumeration from 1 to use 0 as closedevice()

    while (cur_dev) {
        //x->hidpath[i] = getbytes(strlen(cur_dev->path));
        strcpy((char *)x->hidpath[i], cur_dev->path);
        x->ndevices = i;
        if (x->infomode != 1) print_device(cur_dev, i);
        if (x->infomode == 1) hidraw_outlet_info(cur_dev, x);
        cur_dev = cur_dev->next;
        i++;
    }
}



static void hidraw_open(t_hidraw *x, char openmode)
{
    if (x->handle){
        hid_close(x->handle);
        x->handle = NULL;
        post("hidraw: closing previously opened device ...");
    }

    if (openmode) {
        // Open the device using the VID, PID,
        // and optionally the Serial number.
        x->handle = hid_open(x->targetVID, x->targetPID, NULL);
    } else {
        // Open the device using the path
        x->handle = hid_open_path(x->targetpath);
    }

    if (!x->handle) {
        post("hidraw: unable to open device: %ls\n", hid_error(x->handle));
        x->handle = NULL;
        return;
    }

    post("hidraw: successfully opened the device");

    // Set the hid_read() function to be non-blocking.
    hid_set_nonblocking(x->handle, 1);

    // Set up buffer.
    memset(x->readbuf,0x00,sizeof(x->readbuf));
}

static void hidraw_closedevice(t_hidraw *x)
{
    if (x->handle){
        hid_close(x->handle);
        x->handle = NULL;
        post("hidraw: device closed");
    }
}

static void hidraw_opendevice(t_hidraw *x, t_float hidn)
{
    int n = (int)hidn;

    if (n == 0) {
        hidraw_closedevice(x);
        return;
    } else if (!x->devlistdone) {
        post("hidraw: devices not listed yet.");
        return;
    } else if (n > x->ndevices) {
        post("hidraw: device out range. current count of devices is: %d", x->ndevices);
        return;
    } else {
        strcpy(x->targetpath,x->hidpath[n]);
        hidraw_open(x, 0);
    }
}

static void hidraw_opendevice_vidpid(t_hidraw *x, t_float vid, t_float pid)
{
    x->targetVID = (unsigned short) vid;
    x->targetPID = (unsigned short) pid;
    hidraw_open(x, 1);
}

static void hidraw_opendevice_path(t_hidraw *x, t_symbol *path)
{
    strcpy(x->targetpath,path->s_name);
    hidraw_open(x, 0);
}

static void hidraw_listvidpid(t_hidraw *x, t_float vid, t_float pid)
{
    x->devs = hid_enumerate((unsigned short)vid, (unsigned short)pid);
    list_devices(x->devs, x);
    hid_free_enumeration(x->devs);
    x->devlistdone = 1;
}

static void hidraw_listhids(t_hidraw *x)
{
    hidraw_listvidpid(x, 0, 0);
}

static void hidraw_poll(t_hidraw *x, t_float f )
{
    x->polltime = f;
    if (f != 0) clock_delay(x->hidclock, 0);
    else clock_unset(x->hidclock);
}

static void hidraw_tick(t_hidraw *x)
{
    t_atom out[256];

    if (!x->handle){
        post("hidraw: no device opened yet");
        return;
    }

    x->readlen = 0;
    x->readlen = hid_read(x->handle, x->readbuf, sizeof(x->readbuf));

    if (x->readlen < 0) {
        post("hidraw: unable to read(): %ls\n", hid_error(x->handle));
        outlet_float(x->readstatus, -1);
        goto polling;
    }

    if (x->readlen == 0) {
        outlet_float(x->readstatus, 1); //waiting...
        goto polling;
    }

    for (int i = 0; i < x->readlen; i++) {
        SETFLOAT(out+i, x->readbuf[i]);
    }
    outlet_float(x->readstatus, 2);
    outlet_list(x->bytes_out, NULL, x->readlen, out);

    polling:
    clock_delay(x->hidclock, x->polltime);
}

static void hidraw_pdversion(void)
{
    post("---");
    post("  hidraw v%d.%d.%d", HIDRAW_MAJOR_VERSION, HIDRAW_MINOR_VERSION, HIDRAW_BUGFIX_VERSION);
    post("  hidapi v%d.%d.%d", HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH);
    post("---");
}

static void hidraw_free(t_hidraw *x)
{
    if (x->handle) hid_close(x->handle);
    clock_free(x->hidclock);
}


// this is commented out because it is incompatible with not so old Pds
/*

static void hidraw_cleanup(t_class *c) {

    //Free static HIDAPI objects. (when Pd shuts down.)
    hid_exit();

}
*/

static void *hidraw_new(t_symbol *s, int argc, t_atom *argv)
{

    t_hidraw *x = (t_hidraw *)pd_new(hidraw_class);
    x->hidclock = clock_new(x, (t_method)hidraw_tick);
    x->bytes_out = outlet_new(&x->x_obj, &s_list);
    x->readstatus = outlet_new(&x->x_obj, &s_float);
    x->infomode = 0;
    while (argc && argv->a_type == A_SYMBOL)
    {
        if (!strcmp(argv->a_w.w_symbol->s_name, "-info"))
        {
            x->infomode = 1;
            x->info = outlet_new(&x->x_obj, &s_list);
        }
        argc--;
    }
    x->ndevices = 0;
    x->devlistdone = 0;
    x->handle = NULL;
    return (void *)x;
}



#if defined(_WIN32)
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
void hidraw_setup(void)
{
    hidraw_class = class_new(gensym("hidraw"),
                   (t_newmethod)hidraw_new,
                   (t_method)hidraw_free,
                   sizeof(t_hidraw),
                   CLASS_DEFAULT,
                   A_GIMME,
                   0);


    //class_setfreefn(hidraw_class, hidraw_cleanup); // I prefer to not do this as it is incompatible with not so old Pds.
    class_addmethod(hidraw_class, (t_method)hidraw_listhids, gensym("listdevices"), 0);
    class_addmethod(hidraw_class, (t_method)hidraw_listvidpid, gensym("list-vidpid"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(hidraw_class, (t_method)hidraw_opendevice, gensym("open"), A_FLOAT, 0);
    class_addmethod(hidraw_class, (t_method)hidraw_opendevice_vidpid, gensym("open-vidpid"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(hidraw_class, (t_method)hidraw_opendevice_path, gensym("open-path"), A_SYMBOL, 0);
    class_addmethod(hidraw_class, (t_method)hidraw_poll, gensym("poll"), A_FLOAT, 0);
    class_addmethod(hidraw_class, (t_method)hidraw_closedevice, gensym("close"), 0);

    hidraw_pdversion();
    hid_init();

#if defined(__APPLE__)
    // To work properly needs to be called before hid_open/hid_open_path after hid_init.
    // Best/recommended option - call it right after hid_init.
    hid_darwin_set_open_exclusive(0);
#endif

}
