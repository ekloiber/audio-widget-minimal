/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/*
** simple command line program to control SDRWidget features,
** as defined in src/features.h
**
** todo - make sure we don't try to print or set features
** if the feature_major or feature_minor is different from
** the one we were compiled with, since that would have
** very unpredictable effects.
*/
const char usage[] = {
	"usage: sudo ./widget-control [options] [values]\n"
	"options: -d = print the default feature values.\n"
	//	"         -f = factory reset to image default values;\n"
	//	"           doesn't perform a reset, just marks the features\n"
	//	"           to be restored to original values on next reset.\n"
	"         -g = get the feature values from the widget nvram.\n"
	"         -l = list the possible feature values.\n"
	"         -m = get the feature values from the widget ram.\n"
	"         -r = reboot the widget.\n"
	"         -s = set the feature values in the widget nvram.\n"
	"Only -s takes values, in the form printed by -d or -g or -m.\n"
	"The acceptable values for each feature are listed by -l.\n"
	"The major and minor version numbers are optional to -s, but\n"
	"if provided they must match the ones printed by -d, -g, -l, and -m,\n"
	"which must all match each other, or your widget-control is out of\n"
	"sync with your widget.\n"
}; 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "src/features.h"

int verbose = 0;

/*
** features
*/
#define true_feature_major_index 0
#define true_feature_minor_index 1
int true_feature_end_index;
int true_feature_end_values;

// features_t features_default = { FEATURES_DEFAULT };
uint8_t *true_features_default;
//features_t features_nvram;
uint8_t *true_features_nvram;
//features_t features_mem;
uint8_t *true_features_mem;
//char *feature_index_names[] = { FEATURE_INDEX_NAMES };
char **true_feature_index_names;
//char *feature_value_names[] = { FEATURE_VALUE_NAMES };
char **true_feature_value_names;
//int feature_first_value[feature_end_index];
int *feature_first_value;
// int feature_last_value[feature_end_index];
int *feature_last_value;

void feature_first_and_last_init(void) {
	int i, j;
	feature_first_value[true_feature_major_index] = -1;
	feature_first_value[true_feature_minor_index] = -1;
	feature_last_value[true_feature_major_index] = -1;
	feature_last_value[true_feature_minor_index] = -1;
	if (true_feature_minor_index+1 < true_feature_end_index) { 
		feature_first_value[true_feature_minor_index+1] = 0;
		for (i = true_feature_minor_index+1; i < true_feature_end_index; i += 1) {
			for (j = feature_first_value[i]; j < true_feature_end_values && strcmp(true_feature_value_names[j], "end") != 0; j += 1);
			feature_last_value[i] = j-1;
			feature_first_value[i+1] = j+1;
		}
	}
}

int first_value(int index) {
	if (index > true_feature_minor_index && index < true_feature_end_index)
		return feature_first_value[index];
	return -1;
}

int last_value(int index) {
	if (index > true_feature_minor_index && index < true_feature_end_index)
		return feature_last_value[index];
	return -1;
}

int find_feature_value(int index, char *value) {
	int i;
	for (i = first_value(index); i <= last_value(index); i += 1)
		if (strcmp(true_feature_value_names[i], value) == 0)
			return i;
	return -1;
}

/*
** usb
*/
#define REQDIR_HOSTTODEVICE		(0 << 7)
#define REQDIR_DEVICETOHOST		(1 << 7)
#define REQTYPE_STANDARD		(0 << 5)
#define REQTYPE_CLASS			(1 << 5)
#define REQTYPE_VENDOR			(2 << 5)
#define REQREC_DEVICE			(0 << 0)
#define REQREC_INTERFACE		(1 << 0)
#define REQREC_ENDPOINT			(2 << 0)
#define REQREC_OTHER			(3 << 0)

#define WIDGET_RESET			0x0f

#define DG8SAQ_VENDOR_ID	  0x16c0		//!  DG8SAQ device
#define DG8SAQ_PRODUCT_ID     0x05dc
#define AUDIO_VENDOR_ID		  0x16c0		//!  Internal Lab use
#define AUDIO_PRODUCT_ID      0x03e8
#define HPSDR_VENDOR_ID       0xfffe		//! Ozy Device
#define HPSDR_PRODUCT_ID      0x0007

char *usb_serial_id = NULL;
libusb_device_handle *usb_handle;
char *usb_device = "none";
char usb_data[1024];
unsigned int usb_timeout = 2000;


int device_to_host(unsigned char request, unsigned short value, unsigned short index, unsigned short length) {
	return libusb_control_transfer(usb_handle, (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQTYPE_STANDARD), request, value, index, usb_data, length, usb_timeout);
}
	

// this needs modification to handle the -u usb_serial_id option
// must get the total list of devices and query matches for serial id
libusb_device_handle *find_device() {
	usb_handle = libusb_open_device_with_vid_pid(NULL, DG8SAQ_VENDOR_ID, DG8SAQ_PRODUCT_ID);
	if (usb_handle != NULL) {
		usb_device = "dg8saq";
		return usb_handle;
	}
	usb_handle = libusb_open_device_with_vid_pid(NULL, AUDIO_VENDOR_ID, AUDIO_PRODUCT_ID);
	if (usb_handle != NULL) {
		usb_device = "audio";
		return usb_handle;
	}
	usb_handle = libusb_open_device_with_vid_pid(NULL, HPSDR_VENDOR_ID, HPSDR_PRODUCT_ID);
	if (usb_handle != NULL) {
		usb_device = "hpsdr";
		return usb_handle;
	}
	return usb_handle;
}

char *error_string(int err) {
	switch (err) {
	case LIBUSB_SUCCESS: return "Success (no error).";
	case LIBUSB_ERROR_IO: return "Input/output error.";
	case LIBUSB_ERROR_INVALID_PARAM: return "Invalid parameter.";
	case LIBUSB_ERROR_ACCESS: return "Access denied (insufficient permissions).";
	case LIBUSB_ERROR_NO_DEVICE: return "No such device (it may have been disconnected).";
	case LIBUSB_ERROR_NOT_FOUND: return "Entity not found.";
	case LIBUSB_ERROR_BUSY: return "Resource busy.";
	case LIBUSB_ERROR_TIMEOUT: return "Operation timed out.";
	case LIBUSB_ERROR_OVERFLOW: return "Overflow.";
	case LIBUSB_ERROR_PIPE: return "Pipe error.";
	case LIBUSB_ERROR_INTERRUPTED: return "System call interrupted (perhaps due to signal).";
	case LIBUSB_ERROR_NO_MEM: return "Insufficient memory.";
	case LIBUSB_ERROR_NOT_SUPPORTED: return "Operation not supported or unimplemented on this platform.";
	case LIBUSB_ERROR_OTHER: return "Other error.";
	default: {
		static char buff[256];
		sprintf(buff, "undefined error %d", err);
		return buff;
	}
	}
}

void setup() {
	libusb_init(NULL);
	usb_handle = find_device();
	if (usb_handle == NULL) {
		fprintf(stderr, "widget-control: failed to find device\n");
		libusb_exit(NULL);
		exit(1);
	}
	if ( verbose )
		fprintf(stdout, "widget-control: opened %s device\n", usb_device);
	if (libusb_claim_interface(usb_handle, 0) != 0) {
		fprintf(stderr, "widget-control: failed to claim interface 0\n");
		exit(finish(1));
	}
	{
		// read out the widget's major and minor version numbers
		int i, j, res = device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_NVRAM, true_feature_major_index, 8);
		if (res == 1)
			true_feature_end_index = usb_data[0];
		else {
			fprintf(stderr, "unable to read true feature_end_index\n");
			exit(finish(1));
		}
		res = device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_NVRAM, true_feature_minor_index, 8);
		if (res == 1)
			true_feature_end_values = usb_data[0];
		else {
			fprintf(stderr, "unable to read true feature_end_values\n");
			exit(finish(1));
		}
		// allocate arrays for feature values and names
		true_features_default = (uint8_t *)calloc(true_feature_end_index, sizeof(uint8_t));
		true_features_nvram = (uint8_t *)calloc(true_feature_end_index, sizeof(uint8_t));
		true_features_mem = (uint8_t *)calloc(true_feature_end_index, sizeof(uint8_t));
		true_feature_index_names = (char **)calloc(true_feature_end_index, sizeof(char *));
		true_feature_value_names = (char **)calloc(true_feature_end_values, sizeof(char *));
		feature_first_value = (int *)calloc(true_feature_end_index, sizeof(int));
		feature_last_value = (int *)calloc(true_feature_end_index, sizeof(int));
		if (true_features_default == NULL ||
			true_features_nvram == NULL ||
			true_features_mem == NULL ||
			true_feature_index_names == NULL ||
			true_feature_value_names == NULL ||
			feature_first_value == NULL ||
			feature_last_value == NULL) {
			fprintf(stderr, "unable to allocate memory for feature data\n");
			exit(finish(1));
		}
		// read out the widget's index and value names
		for (i = true_feature_major_index; i < true_feature_end_index; i += 1) {
			res = device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_INDEX_NAME, i, sizeof(usb_data));
			true_feature_index_names[i] = calloc(res+1, sizeof(char));
			if (true_feature_index_names[i] == NULL) {
				fprintf(stderr, "unable to allocate memory for feature data\n");
				exit(finish(1));
			}
			for (j = 0; j < res; j += 1) true_feature_index_names[i][res-j-1] = usb_data[j];
			// strncpy(true_feature_index_names[i], usb_data, res);
			true_feature_index_names[i][res] = 0;
			// fprintf(stderr, "retrieved '%s' for index name of %d\n", true_feature_index_names[i], i);
		}
		for (i = 0; i < true_feature_end_values; i += 1) {
			res = device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_VALUE_NAME, i, sizeof(usb_data));
			true_feature_value_names[i] = calloc(res+1, sizeof(char));
			if (true_feature_value_names[i] == NULL) {
				fprintf(stderr, "unable to allocate memory for feature data\n");
				exit(finish(1));
			}
			for (j = 0; j < res; j += 1) true_feature_value_names[i][res-j-1] = usb_data[j];
			// strncpy(true_feature_value_names[i], usb_data, res);
			true_feature_value_names[i][res] = 0;
			// fprintf(stderr, "retrieved '%s' for value name of %d\n", true_feature_value_names[i], i);
		}
		// set up index to value mapping
		feature_first_and_last_init();
		// fetch default values from image
		for (i = 0; i < true_feature_end_index; i += 1) {
			res = device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_DEFAULT, i, 8);
			if (res == 1)
				true_features_default[i] = usb_data[0];
			else {
				fprintf(stderr, "unable to read true features_default %d\n", i);
				exit(finish(1));
			}
		}
	}
}

int finish(int return_value) {
	if (usb_handle != NULL) {
		libusb_release_interface(usb_handle, 0);
		libusb_close(usb_handle);
	}
	libusb_exit(NULL);
	return return_value;
}

/*
** functions
*/
void print_all_features(features_t fp) {
	int j;
	fprintf(stdout, "%d %d", fp[true_feature_major_index], fp[true_feature_minor_index]);
	for (j = true_feature_minor_index+1; j < true_feature_end_index; j += 1) {
		fprintf(stdout, " %s", true_feature_value_names[fp[j]]);
	}
	fprintf(stdout, "\n");
}

void list_all_features() {
	int j, k;
	setup();
	for (j = 0; j < true_feature_end_index; j += 1) {
		switch (j) {
		case true_feature_major_index:
		case true_feature_minor_index:
			fprintf(stdout, "%s %d\n", true_feature_index_names[j], true_features_default[j]);
			continue;
		default:
			fprintf(stdout, "%s = ", true_feature_index_names[j]);
			for (k = first_value(j); k <= last_value(j); k += 1)
				fprintf(stdout, " %s", true_feature_value_names[k]);
			fprintf(stdout, " \n");
		}
	}
}

int get_nvram() {
	int i;
	setup();
	for (i = true_feature_major_index; i < true_feature_end_index; i += 1) {
		int res = device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_NVRAM, i, 8);
		if (res == 1)
			true_features_nvram[i] = usb_data[0];
		else {
			fprintf(stderr, "widget-control: device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_NVRAM, %d, 0) returned %s?\n", i, error_string(res));
			exit(finish(1));
		}
	}
	print_all_features(true_features_nvram);
	return finish(0);
}

int get_mem() {
	int i;
	setup();
	for (i = true_feature_major_index; i < true_feature_end_index; i += 1) {
		int res = device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_RAM, i, 8);
		if (res == 1)
			true_features_mem[i] = usb_data[0];
		else {
			fprintf(stderr, "widget-control: device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_GET_RAM, %d, 0) returned %s?\n", i, error_string(res));
			exit(finish(1));
		}
	}
	print_all_features(true_features_mem);
	return finish(0);
}

int get_default() {
	int i;
	setup();
	print_all_features(true_features_default);
	return finish(0);
}

int set_nvram(int argc, char *argv[]) {
	int i, j;
	features_t features;
	if (argc == true_feature_end_index - 2) {
		// major and minor are implicit
		argv -= 2;
		argc += 2;
	} else if (argc == true_feature_end_index) {
		// verify the major and minor
		if (atoi(argv[true_feature_major_index]) != FEATURE_MAJOR_DEFAULT) {
			fprintf(stderr, "widget-control: invalid major version number %s, should be %d\n", argv[true_feature_major_index], FEATURE_MAJOR_DEFAULT);
			exit(1);
		}
		if (atoi(argv[true_feature_minor_index]) != FEATURE_MINOR_DEFAULT) {
			fprintf(stderr, "widget-control: invalid minor version number %s, should be %d\n", argv[true_feature_minor_index], FEATURE_MINOR_DEFAULT);
			exit(1);
		}
	} else {
		// invalid number of features
		fprintf(stderr, "widget-control: wrong number (%d) of features specified, should be %d features to set\n", argc, FEATURE_MAJOR_DEFAULT);
		exit(1);
	}
	for (i = true_feature_minor_index+1; i < true_feature_end_index; i += 1) {
		j = find_feature_value(i, argv[i]);
		if (j >= first_value(i) && j <= last_value(i)) {
			features[i] = j;
		} else {
			fprintf(stderr, "widget-control: %s is invalid value for %s\n", argv[i], true_feature_index_names[i]);
			exit(1);
		}
	}
	setup();
	for (i = true_feature_minor_index+1; i < true_feature_end_index; i += 1) {
		int res = device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_SET_NVRAM, i | (features[i] << 8), 8);
		if (res != 1) {
			fprintf(stderr, "widget-control: device_to_host(FEATURE_DG8SAQ_COMMAND, FEATURE_DG8SAQ_SET_NVRAM, %d | (%d << 8), 8) returned %s?\n",
					i, features[i], error_string(res));
			exit(finish(1));
		}
	}
	return finish(0);
}

int reset_widget() {
	setup();
	int res = device_to_host(WIDGET_RESET, 0, 0, 8);
	if (res != 1) {
		fprintf(stderr, "widget-control: device_to_host(WIDGET_RESET, 0, 0, 8) returned %s?\n", error_string(res));
		exit(finish(1));
	}
    return finish(0);
}

int main(int argc, char *argv[]) {
	int i;
	for (i = 1; i < argc; i += 1) {
		if (strcmp(argv[i], "-u") == 0) { // specify usb serial id
			if (i + 1 >= argc) {
				fprintf(stderr, "widget-control: usb serial id for -u option\n");
				exit(finish(1));
			} else {
				usb_serial_id = argv[++i];
				continue;
			}
		}
		if (strcmp(argv[i], "-v") == 0) { // be verbose
			verbose = 1;
			continue;
		}
		if (strcmp(argv[i], "-d") == 0) { // list default values
			exit(get_default());
		}
		if (strcmp(argv[i], "-l") == 0) { // list available features
			list_all_features();
			exit(0);
		}
		if (strcmp(argv[i], "-g") == 0) { // get feature(s)
			exit(get_nvram());
		}
		if (strcmp(argv[i], "-s") == 0) { // set feature(s)
			exit(set_nvram(argc-i-1, argv+i+1));
		}
		if (strcmp(argv[i], "-m") == 0) { // get feature(s) from memory
			exit(get_mem());
		}
		if (strcmp(argv[i], "-r") == 0) { // reboot widget
			exit(reset_widget());
		}
	}
	fprintf(stderr, usage);
	exit(1);
}
