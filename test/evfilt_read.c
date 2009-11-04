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

#include <sys/event.h>

#include "common.h"

int sockfd[2];

static void
kevent_socket_drain(void)
{
    char buf[1];

    /* Drain the read buffer, then make sure there are no more events. */
    puts("draining the read buffer");
    if (read(sockfd[0], &buf[0], 1) < 1)
        err(1, "read(2)");
}

static void
kevent_socket_fill(void)
{
  puts("filling the read buffer");
    if (write(sockfd[1], ".", 1) < 1)
        err(1, "write(2)");
}


void
test_kevent_socket_add(void)
{
    const char *test_id = "kevent(EVFILT_READ, EV_ADD)";
    struct kevent kev;

    test_begin(test_id);
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_ADD, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    success(test_id);
}

void
test_kevent_socket_get(void)
{
    const char *test_id = "kevent(EVFILT_READ) wait";
    struct kevent kev;
    int nfds;

    test_begin(test_id);

    kevent_socket_fill();

    nfds = kevent(kqfd, NULL, 0, &kev, 1, NULL);
    if (nfds < 1)
        err(1, "%s: nfds=%d", test_id, nfds);
    KEV_CMP(kev, sockfd[0], EVFILT_READ, EV_ADD);
    if ((int)kev.data != 1)
        err(1, "incorrect data value %d", (int) kev.data); // FIXME: make part of KEV_CMP

    kevent_socket_drain();
    test_no_kevents();

    success(test_id);
}

void
test_kevent_socket_clear(void)
{
    const char *test_id = "kevent(EVFILT_READ, EV_CLEAR)";
    struct kevent kev;
    int nfds;

    test_begin(test_id);

    test_no_kevents();

    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    kevent_socket_fill();
    kevent_socket_fill();

    nfds = kevent(kqfd, NULL, 0, &kev, 1, NULL);
    if (nfds < 1)
        err(1, "%s", test_id);
    KEV_CMP(kev, sockfd[0], EVFILT_READ, 0);
    if ((int)kev.data != 2)
        err(1, "incorrect data value %d", (int) kev.data); // FIXME: make part of KEV_CMP

    /* We filled twice, but drain once. Edge-triggered would not generate
       additional events.
     */
    kevent_socket_drain();
    test_no_kevents();

    kevent_socket_drain();
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_DELETE, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    success(test_id);
}

void
test_kevent_socket_disable(void)
{
    const char *test_id = "kevent(EVFILT_READ, EV_DISABLE)";
    struct kevent kev;

    test_begin(test_id);

    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_DISABLE, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    kevent_socket_fill();
    test_no_kevents();
    kevent_socket_drain();

    success(test_id);
}

void
test_kevent_socket_enable(void)
{
    const char *test_id = "kevent(EVFILT_READ, EV_ENABLE)";
    struct kevent kev, ret;
    int nfds;

    test_begin(test_id);

    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_ENABLE, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    kevent_socket_fill();
    nfds = kevent(kqfd, NULL, 0, &ret, 1, NULL);
    if (nfds < 1)
        err(1, "%s", test_id);
    kevent_cmp(&kev, &ret);
    kevent_socket_drain();

    success(test_id);
}

void
test_kevent_socket_del(void)
{
    const char *test_id = "kevent(EVFILT_READ, EV_DELETE)";
    struct kevent kev;

    test_begin(test_id);

    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_DELETE, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    kevent_socket_fill();
    test_no_kevents();
    kevent_socket_drain();

    success(test_id);
}

void
test_kevent_socket_oneshot(void)
{
    const char *test_id = "kevent(EVFILT_READ, EV_ONESHOT)";
    struct kevent kev, ret;

    test_begin(test_id);

    /* Re-add the watch and make sure no events are pending */
    puts("-- re-adding knote");
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);
    test_no_kevents();

    puts("-- getting one event");
    kevent_socket_fill();
    if (kevent(kqfd, NULL, 0, &ret, 1, NULL) != 1)
        err(1, "%s", test_id);
    kevent_cmp(&kev, &ret);

    puts("-- checking knote disabled");
    test_no_kevents();

    /* Try to delete the knote, it should already be deleted */
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_DELETE, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) == 0)
        err(1, "%s", test_id);

    kevent_socket_drain();

    success(test_id);
}


void
test_kevent_socket_dispatch(void)
{
    const char *test_id = "kevent(EVFILT_READ, EV_DISPATCH)";

    test_begin(test_id);

#if HAVE_EV_DISPATCH
    struct kevent kev;

    /* Re-add the watch and make sure no events are pending */
    puts("-- re-adding knote");
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_ADD | EV_DISPATCH, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);
    test_no_kevents();

    /* Since the knote is disabled, there are no events. */
    puts("-- checking if knote is disabled..");
    kevent_socket_fill();
    if (kevent(kqfd, NULL, 0, &kev, 1, NULL) != 1)
        err(1, "%s", test_id);
    KEV_CMP(kev, sockfd[0], EVFILT_READ, 0);
    test_no_kevents();

    /* Since the knote is disabled, the EV_DELETE operation succeeds. */
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_DELETE, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    kevent_socket_drain();

#endif  /* HAVE_EV_DISPATCH */

    success(test_id);
}

#if BROKEN
void
test_kevent_socket_lowat(void)
{
    const char *test_id = "kevent(EVFILT_READ, NOTE_LOWAT)";
    struct kevent kev;

    test_begin(test_id);

    /* Re-add the watch and make sure no events are pending */
    puts("-- re-adding knote, setting low watermark to 2 bytes");
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_ADD | EV_ONESHOT, NOTE_LOWAT, 2, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);
    test_no_kevents();

    puts("-- checking that one byte does not trigger an event..");
    kevent_socket_fill();
    test_no_kevents();

    puts("-- checking that two bytes triggers an event..");
    kevent_socket_fill();
    if (kevent(kqfd, NULL, 0, &kev, 1, NULL) != 1)
        err(1, "%s", test_id);
    KEV_CMP(kev, sockfd[0], EVFILT_READ, 0);
    test_no_kevents();

    kevent_socket_drain();
    kevent_socket_drain();

    success(test_id);
}
#endif

void
test_kevent_socket_eof(void)
{
    const char *test_id = "kevent(EVFILT_READ, EV_EOF)";
    struct kevent kev;

    test_begin(test_id);

    /* Re-add the watch and make sure no events are pending */
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_ADD, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);
    test_no_kevents();

    if (close(sockfd[1]) < 0)
        err(1, "close(2)");

    if (kevent(kqfd, NULL, 0, &kev, 1, NULL) < 1)
        err(1, "%s", test_id);
    KEV_CMP(kev, sockfd[0], EVFILT_READ, EV_EOF);

    /* Delete the watch */
    EV_SET(&kev, sockfd[0], EVFILT_READ, EV_DELETE, 0, 0, &sockfd[0]);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    success(test_id);
}

void
test_evfilt_read()
{
    /* Create a connected pair of full-duplex sockets for testing socket events */
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd) < 0) 
        abort();

    test_kevent_socket_add();
    test_kevent_socket_get();
    test_kevent_socket_disable();
    test_kevent_socket_enable();
    test_kevent_socket_del();
    test_kevent_socket_oneshot();
    test_kevent_socket_clear();
    test_kevent_socket_dispatch();
    test_kevent_socket_eof();
}
