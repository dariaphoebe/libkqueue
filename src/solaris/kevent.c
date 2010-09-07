/*
 * Copyright (c) 2009 Mark Heily <mark@heily.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <port.h>
#include <poll.h>

#include "sys/event.h"
#include "private.h"

/* Dump a poll(2) events bitmask */
static char *
poll_events_dump(short events)
{
    static char __thread buf[512];

#define _PL_DUMP(attrib) \
    if (events == attrib) \
       strcat(&buf[0], " "#attrib);

    snprintf(&buf[0], 512, "events = %hd 0x%o (", events, events);
    _PL_DUMP(POLLIN);
    _PL_DUMP(POLLPRI);
    _PL_DUMP(POLLOUT);
    _PL_DUMP(POLLRDNORM);
    _PL_DUMP(POLLRDBAND);
    _PL_DUMP(POLLWRBAND);
    _PL_DUMP(POLLERR);
    _PL_DUMP(POLLHUP);
    _PL_DUMP(POLLNVAL);
    strcat(&buf[0], ")");

    return (&buf[0]);

#undef _PL_DUMP
}

static char *
port_event_dump(port_event_t *evt)
{
    static char __thread buf[512];

    if (evt == NULL)
        return "(null)";

#define PE_DUMP(attrib) \
    if (evt->portev_source == attrib) \
       strcat(&buf[0], #attrib);

    snprintf(&buf[0], 512,
                " { object = %u, user = %p, %s, source = %d (",
                (unsigned int) evt->portev_object,
                evt->portev_user,
                poll_events_dump(evt->portev_events),
                evt->portev_source);
    PE_DUMP(PORT_SOURCE_AIO);
    PE_DUMP(PORT_SOURCE_FD);
    PE_DUMP(PORT_SOURCE_TIMER);
    PE_DUMP(PORT_SOURCE_USER);
    PE_DUMP(PORT_SOURCE_ALERT);
    strcat(&buf[0], ") }\n");

    return (&buf[0]);
#undef PE_DUMP
}

int
kevent_wait(struct kqueue *kq, const struct timespec *timeout)
{
    port_event_t pe;
    int rv;
    uint_t nget = 1;

retry_port_getn:
    reset_errno();
    dbg_printf("waiting for events (timeout=%p)", timeout);
    rv = port_getn(kq->kq_port, &pe, 0, &nget, (struct timespec *) timeout);
    dbg_printf("rv=%d errno=%d (%s) nget=%d", 
                rv, errno, strerror(errno), nget);
    if (rv < 0) {
        if (errno == ETIME) {
            dbg_puts("no events within the given timeout");
            return (0);
        }
        if (errno == EINTR) {
            dbg_puts("signal caught");
            return (-1);
        }
        dbg_perror("port_get(2)");
        return (-1);
    }

#error FIXME
    // according to manpage, port_getn with max=0 will not block.
    // will have to use max=1 and store the result in a TLS variable

    /* WORKAROUND: Solaris sometimes returns immediately with no events
       even when a NULL timeout has been provided. */
    if (nget == 0 && rv == 0 && timeout == NULL) {
        dbg_puts("WARNING: port_getn() returned no events and no error was indicated");
        goto retry_port_getn;
    }

    return (nget);
}

int
kevent_copyout(struct kqueue *kq, int nready,
        struct kevent *eventlist, int nevents)
{
    port_event_t *pe = &kq->kq_evt;
    struct filter *filt;
    struct timespec timeout;
    int rv;
    uint_t nget = 1;

    /* Retrieve an event */
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
    rv = port_getn(kq->kq_port, pe, 1, &nget, &timeout);
    dbg_printf("rv=%d errno=%d nget=%d", rv, errno, nget);
    if (rv < 0) {
        if (errno == ETIME) {
            dbg_puts("no events within the given timeout");
            return (-1);
        }
        if (errno == EINTR) {
            dbg_puts("signal caught");
            return (-1);
        }
        dbg_perror("port_get(2)");
        return (-1);
    }
    if (nget == 0) {
            dbg_puts("no events returned");
            return (-1);
    }

    dbg_printf("%s", port_event_dump(pe));
    switch (pe->portev_source) {
	case PORT_SOURCE_FD:
        filt = pe->portev_user;
        rv = filt->kf_copyout(filt, eventlist, nevents);
        break;

	case PORT_SOURCE_TIMER:
        filter_lookup(&filt, kq, EVFILT_TIMER);
        rv = filt->kf_copyout(filt, eventlist, nevents);
        break;

	case PORT_SOURCE_USER:
        switch (pe->portev_events) {
            case X_PORT_SOURCE_SIGNAL:
                filter_lookup(&filt, kq, EVFILT_SIGNAL);
                rv = filt->kf_copyout(filt, eventlist, nevents);
                break;
            default:
                dbg_puts("unsupported portev_events");
                abort();
        }
        break;

	default:
		dbg_puts("unsupported source");
    		abort();
    }
    if (rv < 0) {
        dbg_puts("kevent_copyout failed");
	return (-1);
    }

    return (1);
}
