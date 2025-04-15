#ifndef TABLETIZER_H__
#define TABLETIZER_H__
#include <stddef.h>
#include <libudev.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <xorg/Xwacom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>

#define t_errln(msg, ...) (fprintf(stderr, msg "\n", __VA_ARGS__ + 0))
#define t_assert(x, on_not) do{if(!(x)){t_errln(__FILE__ ":%d:assertion failed: " #x, __LINE__); {on_not;};}}while(0)

typedef struct t_ctx t_ctx;


typedef struct
{
	char *name;
	int button;
} t_device;

typedef struct
{
	size_t id;
	char *action;
} t_button;


struct t_ctx
{
	Display *d;
	struct udev *u;
	struct udev_monitor *m;
	int monitor_fd;
	char *display_name; /* devices mapped to this */
	t_device *dev;
	size_t devs;
	t_button *button;
	size_t buttons;
};

#endif /* TABLETIZER_H__ */
