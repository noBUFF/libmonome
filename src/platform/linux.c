/*
 * This file is part of libmonome.
 * libmonome is copyright 2007-2010 will light <visinin@gmail.com>
 *
 * libmonome is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libudev.h>
#include <monome.h>

#include "monome_internal.h"

typedef struct monome_device_mapping monome_device_mapping_t;

struct monome_device_mapping {
	char *serial;
	monome_device_t model;
	char *friendly;
};

static monome_device_mapping_t mapping[] = {
	{"m256-%d", MONOME_DEVICE_256, "monome 256"},
	{"m128-%d", MONOME_DEVICE_128, "monome 128"},
	{"m64-%d",  MONOME_DEVICE_64,  "monome 64"},
	{"m40h%d",  MONOME_DEVICE_40h, "monome 40h"},
	{0, 0, 0}
};

static struct udev *udev;

static int get_monome_information(monome_t *monome, struct udev_device *d) {
	const char *serial, *tty;
	int serialnum;

	monome_device_t model = 0;
	monome_device_mapping_t *c;

	assert(d);

	if( !(tty    = udev_device_get_devnode(d)) ||
		!(serial = udev_device_get_property_value(d, "ID_SERIAL_SHORT")) )
		return 1;

	for( c = mapping; c->serial; c++ ) {
		if( !sscanf(serial, c->serial, &serialnum) )
			continue;

		model = c->model;
		break;
	}

	if( !model )
		return 1; /* unrecognized device, disregard */

	monome->model  = model;
	monome->serial = strdup(serial);
	monome->device = strdup(tty);

	return 0;
}

static int get_monome_information_from_syspath(monome_t *monome, const char *syspath) {
	struct udev_device *d = NULL;
	int ret;

	assert(syspath);
	if( !(d = udev_device_new_from_syspath(udev, syspath)) )
		return 1;

	ret = get_monome_information(monome, d);
	udev_device_unref(d);

	return ret;
}

static int get_monome_information_from_devname(monome_t *monome, const char *name) {
	struct udev_enumerate *ue;
	struct udev_list_entry *c;
	const char *syspath;

	assert(name);

	udev = udev_new();

	ue = udev_enumerate_new(udev);
	udev_enumerate_add_match_property(ue, "DEVNAME", name);

	if( udev_enumerate_scan_devices(ue) )
		goto err;

	c = udev_enumerate_get_list_entry(ue);

	if( !(syspath = udev_list_entry_get_name(c)) )
		goto err; /* should NOT happen ever but whatever... */

	if( get_monome_information_from_syspath(monome, syspath) )
		goto err;

	udev_enumerate_unref(ue);
	udev_unref(udev);
	return 0;

err:
	/* so uh something went wrong
	   we'll just assume we've got a 40h... */

	monome->model = MONOME_DEVICE_40h;

	udev_enumerate_unref(ue);
	udev_unref(udev);
	return 1;
}

#include "posix.inc"
