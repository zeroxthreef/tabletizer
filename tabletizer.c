#if 0
echo building tabletizer
set -ex
#clang -std=c99 -Weverything -fsanitize=undefined -fsanitize=address -Wno-cast-qual -Wno-string-plus-int -Wno-c23-extensions -Wno-extra-semi-stmt -Wno-format-extra-args -Wno-used-but-marked-unused -Wno-gnu-label-as-value -Wno-unsafe-buffer-usage -Wno-padded -Wno-comment -Wno-missing-prototypes -Wno-disabled-macro-expansion -Wno-unused-variable -Wno-unused-label -Wno-missing-noreturn -Wno-switch-enum -Wno-unused-parameter -Wno-unused-macros -g -o tabletizer pdjson/pdjson.c $0 -lX11 -lXi -lXrandr -ludev
cc -std=c99 -Wall -Wextra -Wno-format-extra-args -Os -s -o tabletizer pdjson/pdjson.c $0 -lX11 -lXi -lXrandr -ludev
echo success
exit 0
#endif
/*
Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include "tabletizer.h"
#include "pdjson/pdjson.h"

#include <poll.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *t_strndup(char *str, size_t len)
{
	char *ret = NULL;
	t_assert(ret = malloc(len + 1), return NULL);
	memcpy(ret, str, len);
	ret[len] = 0x0;
	return ret;
}

int t_append(void **data, size_t *data_amount, void *append, size_t append_size)
{
	t_assert(data && data_amount && append && append_size, return 1);
	t_assert(*data = realloc(*data, (*data_amount + 1) * append_size), return 1);
	memcpy(&((char *)*data)[*data_amount * append_size], append, append_size);
	(*data_amount)++;
	return 0;
}

#define t_match(cstr, str, len) (strlen(cstr) == (len) && !strncmp((cstr), (str), (len)))
int t_configure(t_ctx *t, char *path)
{
	FILE *f;
	json_stream j;
	char *str;
	size_t len;

	json_open_stream(&j, (f = fopen(path, "r")));
	t_assert(f, {t_errln("unable to open config file. Does it exist?"); goto error;});
	if(json_next(&j) != JSON_OBJECT)
	{
		t_errln("expected root json object");
		goto error;
	}
	for(;;)
		switch(json_next(&j))
		{
			/* expect only keys in root object */
			case JSON_STRING:
				t_assert((str = (char*)json_get_string(&j, &len)) && len--, goto error);
				if(t_match("display", str, len))
				{
					t_assert(json_next(&j) == JSON_STRING, goto error);
					t_assert((str = (char*)json_get_string(&j, &len)) && len--, goto error);
					t_assert(t->display_name = t_strndup(str, len), goto error);
					t_errln("tabletizer target display is: '%s'", t->display_name);
				}
				else if(t_match("buttons", str, len))
				{
					size_t i = 0;
					t_assert(json_next(&j) == JSON_ARRAY, goto error);
					for(;;i++)
					{
						if(json_peek(&j) == JSON_ARRAY_END) break;
						t_assert(json_next(&j) == JSON_OBJECT, goto error);

						/* button id key */
						t_assert(json_next(&j) == JSON_STRING, goto error);
						t_assert((str = (char*)json_get_string(&j, &len)) && len--, goto error);
						t_assert(!t_append((void*)&t->button, &t->buttons, &(t_button){.id = (size_t)atoi(str) - 1}, sizeof(t_button)), goto error;);
						/* button action */
						t_assert(json_next(&j) == JSON_STRING, goto error);
						t_assert((str = (char*)json_get_string(&j, &len)) && len--, goto error);
						t_assert(t->button[i].action = t_strndup(str, len), goto error);
						t_errln("button: %lu, action: '%s'", t->button[i].id, t->button[i].action);

						t_assert(json_next(&j) == JSON_OBJECT_END, goto error);
					}
					t_assert(json_next(&j) == JSON_ARRAY_END, goto error);
				}
				else if(t_match("devs", str, len))
				{
					size_t i = 0;
					enum json_type type;
					t_assert(json_next(&j) == JSON_ARRAY, goto error);
					for(;;i++)
					{
						if(json_peek(&j) == JSON_ARRAY_END) break;
						t_assert(json_next(&j) == JSON_OBJECT, goto error);

						/* device key */
						t_assert(json_next(&j) == JSON_STRING, goto error);
						t_assert((str = (char*)json_get_string(&j, &len)) && len--, goto error);
						t_assert(t_match("name", str, len), goto error);
						/* device name */
						t_assert(json_next(&j) == JSON_STRING, goto error);
						t_assert((str = (char*)json_get_string(&j, &len)) && len--, goto error);
						t_assert(!t_append((void**)&t->dev, &t->devs, &(t_device){.name = t_strndup(str, len)}, sizeof(t_device)), goto error);
						/* optional button mapped device */
						if(json_peek(&j) == JSON_STRING)
						{
							t_assert(json_next(&j) == JSON_STRING, goto error);
							t_assert((str = (char*)json_get_string(&j, &len)) && len--, goto error);
							t_assert(t_match("button", str, len), goto error);
							type = json_next(&j);
							t_assert(type == JSON_TRUE || type == JSON_FALSE, goto error);
							t->dev[i].button = type == JSON_TRUE ? 1 : 0;
						}
						t_errln("device: '%s', has button: %s", t->dev[i].name, t->dev[i].button ? "true" : "false");
						t_assert(json_next(&j) == JSON_OBJECT_END, goto error);
					}
					t_assert(json_next(&j) == JSON_ARRAY_END, goto error);
				}
				else
				{
					t_errln("unexpected config field: '%.*s'", (int)len, str);
					goto error;
				}
			break;
			case JSON_OBJECT_END:
				goto leave;
			default:
			case JSON_ERROR:
				t_errln("error in json config:%lu:'%s'", json_get_lineno(&j), json_get_error(&j));
				goto error;
		}
leave:
	json_close(&j);
	return 0;
error:
	json_close(&j);
	return 1;
}

/* skip through space chars and through alnum in end */
int t_strskip(char *str, size_t *start, size_t *end)
{
	t_assert(str && start && end, return 1);
	t_assert(*start < strlen(str), return 1);
	/* skip space at start */
	for(; *start < strlen(str) && isspace(str[*start]); (*start)++);
	/* find end of alnum */
	for(*end = *start; *end < strlen(str) && (isalnum(str[*end]) || str[*end] == '_'); (*end)++);
	return 0;
}

int t_map_buttons(t_ctx *t, XDeviceInfo *info)
{
	XDevice *dev = NULL;
	size_t i, start, end, total;
	Atom action_atom, type_atom, *button_data = NULL;
	char *action = NULL;
	int ret_format;
	unsigned long num_items_return, bytes_after_return, *actual_action = NULL;
	t_assert(dev = XOpenDevice(t->d, info->id), goto error);

	t_assert(action_atom = XInternAtom(t->d, "Wacom Button Actions", True), goto error;);
	t_assert(Success == XGetDeviceProperty(t->d, dev, action_atom, 0, 100, False, AnyPropertyType,
		&type_atom, &ret_format, &num_items_return, &bytes_after_return, (unsigned char**)&button_data), goto error);

	t_assert(ret_format == 32 && type_atom == XA_ATOM, goto error);
	t_errln("device has %lu buttons, and %d classes", num_items_return, info->num_classes);

	for(i = 0; i < t->buttons; i++)
	{
		t_assert(action = calloc(1, strlen(t->button[i].action) + 1), goto error);
		for(total = start = end = 0; start < strlen(t->button[i].action); start = end)
		{
			t_assert(!t_strskip(t->button[i].action, &start, &end), goto error);
			if(end > 0 && start == end)
				break;

			memcpy(action, &t->button[i].action[start], end - start);
			action[end - start] = 0x0;
			t_assert(!t_append((void**)&actual_action, &total, &(unsigned long[]){
				AC_KEY | AC_KEYBTNPRESS | XKeysymToKeycode(t->d, XStringToKeysym(action))}, sizeof(unsigned long)), goto error);
		}
		t_assert(t->button[i].id < num_items_return, goto error);
		XChangeDeviceProperty(t->d, dev, button_data[t->button[i].id], XA_INTEGER, 32, PropModeReplace, (unsigned char *)actual_action, (int)total);
		free(action); /* terrible, but its a guarantee */
		free(actual_action);
		actual_action = NULL;
	}
	XFree(button_data);
	XCloseDevice(t->d, dev);
	return 0;
error:
	if(actual_action) free(actual_action);
	if(button_data) XFree(button_data);
	if(dev) XCloseDevice(t->d, dev);
	if(action) free(action);
	return 1;
}

int t_map(t_ctx *t)
{
	size_t i;
	int ndev, ret_format;
	unsigned long num_items_return, bytes_after_return;
	float mat[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, *temp, display_width, display_height;
	Atom matrix_atom, float_atom, ret_atom;
	XDeviceInfo *devs = NULL;
	XRROutputInfo *info = NULL;
	XRRScreenResources *res = NULL;
	XRRCrtcInfo *cinfo = NULL;

	t_assert(res = XRRGetScreenResources(t->d, DefaultRootWindow(t->d)), goto error);
	for(i = 0; i < (size_t)res->noutput; i++)
	{
		t_assert(info = XRRGetOutputInfo(t->d, res, res->outputs[i]), goto error);
		if(!strcmp(info->name, t->display_name)) break;
		XRRFreeOutputInfo(info);
		info = NULL;
	}
	if(!info) return 0;
	t_assert(cinfo = XRRGetCrtcInfo(t->d, res, info->crtc), goto error);

	/* calculate matrix */
	t_assert(matrix_atom = XInternAtom(t->d, "Coordinate Transformation Matrix", True), goto error);
	t_assert(float_atom = XInternAtom(t->d, "FLOAT", True), goto error);
	display_width = (float)DisplayWidth(t->d, DefaultScreen(t->d));
	display_height = (float)DisplayHeight(t->d, DefaultScreen(t->d));
	mat[0][0] = 1.f * (float)cinfo->width / display_width;
	mat[0][2] = 1.f * (float)cinfo->x / display_width;
	mat[1][1] = 1.f * (float)cinfo->height / display_height;
	mat[1][2] = 1.f * (float)cinfo->y / display_height;
	
	t_assert(devs = XListInputDevices(t->d, &ndev), goto error);
	for(i = 0 ; i < (size_t)ndev; i++)
	{
		size_t j;
		for(j = 0; j < t->devs; j++)
			if(!strcmp(devs[i].name, t->dev[j].name))
			{
				t_errln("matched device '%s', activating and mapping to '%s'", devs[i].name, t->display_name);
				t_assert(Success == XIGetProperty(t->d, (int)devs[i].id, matrix_atom, 0, 9, False, float_atom,
					&ret_atom, &ret_format, &num_items_return, &bytes_after_return, (unsigned char**)&temp), goto error);

				t_assert(ret_atom == float_atom && ret_format == 32 && num_items_return == 9 && !bytes_after_return, goto error);
				XIChangeProperty(t->d, (int)devs[i].id, matrix_atom, float_atom, ret_format, PropModeReplace, (unsigned char*)&mat, (int)num_items_return);
				if(t->dev[j].button) t_assert(!t_map_buttons(t, &devs[i]), goto error);
				XFree(temp);
			}
	}
	XFree(devs);
	XRRFreeCrtcInfo(cinfo);
	XRRFreeOutputInfo(info);
	XRRFreeScreenResources(res);
	return 0;
error:
	if(devs) XFree(devs);
	if(cinfo) XRRFreeCrtcInfo(cinfo);
	if(info) XRRFreeOutputInfo(info);
	if(res) XRRFreeScreenResources(res);
	return 1;
}

int t_monitor(t_ctx *t)
{
	struct udev_device *d = NULL;
	struct pollfd pfd = {.fd = t->monitor_fd, .events = POLLIN | POLLPRI, .revents = 0};

	while(poll(&pfd, 1, -1))
	{
		/* unfortunately, udev does not remember the name of devices when theyre removed
		and, for some reason, does not list some input devices like the eraser on my pen.
		Because of this, a slower XDeviceInfo list is required */
		t_assert(d = udev_monitor_receive_device(t->m), goto error);
		if(!strcmp(udev_device_get_action(d), "add"))
			t_assert(!t_map(t), goto error);
		udev_device_unref(d);
	}
	return 0;
error:
	if(d) udev_device_unref(d);
	return 1;
}

int t_release(t_ctx *t)
{
	size_t i;
	free(t->display_name);
	for(i = 0; i < t->devs; i++) free(t->dev[i].name);
	free(t->dev);
	for(i = 0; i < t->buttons; i++) free(t->button[i].action);
	free(t->button);
	return 0;
}

int main(int argc, char **argv)
{
	t_ctx t;
	int opcode, event, error;

	memset(&t, 0, sizeof(t));
	t_errln("tabletizer starting");
		if(argc < 2)
		{
			t_errln("require path to config.json to function");
			return 1;
		}
		t_assert(t.u = udev_new(), goto error);
		t_assert(t.m = udev_monitor_new_from_netlink(t.u, "udev"), goto error);
		t_assert(!udev_monitor_filter_add_match_subsystem_devtype(t.m, "input", NULL), goto error);
		t_assert(udev_monitor_enable_receiving(t.m) >= 0, goto error);
		t_assert((t.monitor_fd = udev_monitor_get_fd(t.m)) >= 0, goto error);
		t_assert(t.d = XOpenDisplay(NULL), goto error);
		XSynchronize(t.d, True); /* surprisingly, everything blows up without this :( */
		t_assert(XQueryExtension(t.d, "XInputExtension", &opcode, &event, &error), goto error);
		t_assert(XQueryExtension(t.d, "RANDR", &opcode, &event, &error), goto error);
		t_assert(!t_configure(&t, argv[1]), goto error);
		t_assert(!t_map(&t), goto error);
	t_errln("tabletizer running");
		t_assert(!t_monitor(&t), goto error);
	t_errln("tabletizer stopping");
		t_release(&t);
		udev_monitor_unref(t.m);
		udev_unref(t.u);
		XCloseDisplay(t.d);
		return 0;
error:
	t_errln("tabletizer aborting");
	t_release(&t);
	if(t.m) udev_monitor_unref(t.m);
	if(t.u) udev_unref(t.u);
	if(t.d) XCloseDisplay(t.d);
	return 1;
}

