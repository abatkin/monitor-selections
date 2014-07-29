#include<stdio.h>
#include<stdlib.h>

#include<X11/Xlib.h>
#include<X11/Xatom.h>
#include<X11/Xos.h>

Bool XNextEventTimed(Display* dsp, XEvent* event_return, struct timeval* tv) {
  // optimization
  if (tv == NULL) {
    XNextEvent(dsp, event_return);
    return True;
  }

  // the real deal
  if (XPending(dsp) == 0) {
    int fd = ConnectionNumber(dsp);
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(fd, &readset);
    if (select(fd+1, &readset, NULL, NULL, tv) == 0) {
      return False;
    } else {
      XNextEvent(dsp, event_return);
      return True;
    }
  } else {
    XNextEvent(dsp, event_return);
    return True;
  }
}

typedef struct {
  int requestSent;
} clipstate;

int sendRequest(Display* display, Window window, Atom selection) {
  Window w = XGetSelectionOwner(display, selection);
  if (w == None) {
    printf("No selection\n");
    return 0;
  } else {
    char* name;
    if (XFetchName(display, w, &name)) {
      printf("Name [%s]\n", name);
      XFree(name);
      XConvertSelection(display, selection, XA_STRING, XA_PRIMARY, window, CurrentTime);
      return 1;
    } else {
      printf("Unable to fetch windown name\n");
      return 0;
    }
  }
}

unsigned long getSelectionSize(Display *display, XSelectionEvent *event) {
  Atom actualType;
  int actualFormat;
  unsigned long actualItems;
  unsigned long bytesRemaining;
  unsigned char* prop;
  XGetWindowProperty(display, event->requestor, event->property, 0, 0, False, AnyPropertyType,
    &actualType,
    &actualFormat,
    &actualItems,
    &bytesRemaining,
    &prop);
  return bytesRemaining;
}

void handleSelection(Display* display, Window w, clipstate *cs, XEvent *report) {
  XSelectionEvent *event = (XSelectionEvent*)report;
  if (event->property == None) {
    printf("None\n");
  } else {
    unsigned long len = getSelectionSize(display, event);
    Atom actualType;
    int actualFormat;
    unsigned long actualItems;
    unsigned long bytesRemaining;
    unsigned char* prop;
    XGetWindowProperty(display, event->requestor, event->property, 0, len, False, AnyPropertyType,
      &actualType,
      &actualFormat,
      &actualItems,
      &bytesRemaining,
      &prop);
    if (bytesRemaining == 0) {
      printf("Got [%s]\n", prop);
      XFree(prop);
    } else {
      printf("len [%ld] remaining [%ld]\n", len, bytesRemaining);
    }
  }
  cs->requestSent = 0;
}

void handleEvents(Display* display, Window w) {
  clipstate cs = {0};

  while (1) {
    XEvent report;
    struct timeval tv = { 1, 0 }; // 1 second
    if (XNextEventTimed(display, &report, &tv)) {
      switch (report.type) {
        case SelectionNotify:
          handleSelection(display, w, &cs, &report);
          break;
        default:
          printf("Unknown event type [%d]\n", report.type);
      }
    } else {
      printf("timer\n");
      if (!cs.requestSent) {
        cs.requestSent = sendRequest(display, w, XA_PRIMARY);
      }
    }
  }
}

int main(int argc, char* argv[]) {
  Display *display = XOpenDisplay(NULL);
  if (display == NULL) {
    printf("Unable to open X11 display");
    exit(-1);
  }
  int screen = DefaultScreen(display);
  Window rootWindow = RootWindow(display, screen);
  Window window = XCreateSimpleWindow(display, rootWindow, 1, 1, 1, 1, 0, 0, 0);
  handleEvents(display, window);
  Atom selection = XA_PRIMARY;
  Window w = XGetSelectionOwner(display, selection);
  if (w == None) {
    printf("No selection\n");
  } else {
    char* name;
    if (XFetchName(display, w, &name)) {
      printf("Name [%s]\n", name);
      XFree(name);
      Atom actualType;
      int actualFormat;
      unsigned long actualItems;
      unsigned long bytesRemaining;
      unsigned char* prop;
      if (XGetWindowProperty(
        display,
        w,
        XA_PRIMARY,
        0, 0,
        0,
        AnyPropertyType,
        &actualType,
        &actualFormat,
        &actualItems,
        &bytesRemaining,
        &prop
      )) {
        printf("Success!\n");
      } else {
        printf("Failed\n");
      }
    }
  }
  return 0;
}
