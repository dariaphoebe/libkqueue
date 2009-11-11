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

#include "common.h"

int kqfd;
pid_t child;

static void
add_and_delete(void)
{
    const char *test_id1 = "kevent(EVFILT_PROC, EV_ADD)";
    const char *test_id2 = "kevent(EVFILT_PROC, EV_ADD)";
    struct kevent kev;

    test_begin(test_id1);

    EV_SET(&kev, child, EVFILT_PROC, EV_ADD, 0, 0, NULL);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id1);

    success(test_id1);

    test_begin(test_id2);

    kev.flags = EV_DELETE;
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id2);

    success(test_id2);

}

#if TODO
void
test_kevent_signal_get(void)
{
    const char *test_id = "kevent(EVFILT_SIGNAL, wait)";
    struct kevent kev;

    test_begin(test_id);

    EV_SET(&kev, SIGUSR1, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);    
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    /* Block SIGUSR1, then send it to ourselves */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        err(1, "sigprocmask");
    if (kill(getpid(), SIGUSR1) < 0)
        err(1, "kill");

    kev.flags |= EV_CLEAR;
    kev.data = 1;
    kevent_cmp(&kev, kevent_get(kqfd));

    success(test_id);
}

void
test_kevent_signal_disable(void)
{
    const char *test_id = "kevent(EVFILT_SIGNAL, EV_DISABLE)";
    struct kevent kev;

    test_begin(test_id);

    EV_SET(&kev, SIGUSR1, EVFILT_SIGNAL, EV_DISABLE, 0, 0, NULL);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    /* Block SIGUSR1, then send it to ourselves */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        err(1, "sigprocmask");
    if (kill(getpid(), SIGUSR1) < 0)
        err(1, "kill");

    test_no_kevents();

    success(test_id);
}

void
test_kevent_signal_enable(void)
{
    const char *test_id = "kevent(EVFILT_SIGNAL, EV_ENABLE)";
    struct kevent kev;

    test_begin(test_id);

    EV_SET(&kev, SIGUSR1, EVFILT_SIGNAL, EV_ENABLE, 0, 0, NULL);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    /* Block SIGUSR1, then send it to ourselves */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        err(1, "sigprocmask");
    if (kill(getpid(), SIGUSR1) < 0)
        err(1, "kill");

    kev.flags = EV_ADD | EV_CLEAR;
#if LIBKQUEUE
    kev.data = 1; /* WORKAROUND */
#else
    kev.data = 2; // one extra time from test_kevent_signal_disable()
#endif
    kevent_cmp(&kev, kevent_get(kqfd));

    /* Delete the watch */
    kev.flags = EV_DELETE;
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    success(test_id);
}

void
test_kevent_signal_del(void)
{
    const char *test_id = "kevent(EVFILT_SIGNAL, EV_DELETE)";
    struct kevent kev;

    test_begin(test_id);

    /* Delete the kevent */
    EV_SET(&kev, SIGUSR1, EVFILT_SIGNAL, EV_DELETE, 0, 0, NULL);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    /* Block SIGUSR1, then send it to ourselves */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        err(1, "sigprocmask");
    if (kill(getpid(), SIGUSR1) < 0)
        err(1, "kill");

    test_no_kevents();
    success(test_id);
}

void
test_kevent_signal_oneshot(void)
{
    const char *test_id = "kevent(EVFILT_SIGNAL, EV_ONESHOT)";
    struct kevent kev;

    test_begin(test_id);

    EV_SET(&kev, SIGUSR1, EVFILT_SIGNAL, EV_ADD | EV_ONESHOT, 0, 0, NULL);
    if (kevent(kqfd, &kev, 1, NULL, 0, NULL) < 0)
        err(1, "%s", test_id);

    /* Block SIGUSR1, then send it to ourselves */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        err(1, "sigprocmask");
    if (kill(getpid(), SIGUSR1) < 0)
        err(1, "kill");

    kev.flags |= EV_CLEAR;
    kev.data = 1;
    kevent_cmp(&kev, kevent_get(kqfd));

    /* Send another one and make sure we get no events */
    if (kill(getpid(), SIGUSR1) < 0)
        err(1, "kill");
    test_no_kevents();

    success(test_id);
}
#endif

void
test_evfilt_proc()
{
    kqfd = kqueue();

    /* Create a child that sleeps for a few seconds and then exits */
    child = fork();
    if (child == 0) {
        sleep(3);
        exit(123);
    }

    add_and_delete();
#if TODO
    test_kevent_signal_add();
    test_kevent_signal_del();
    test_kevent_signal_get();
    test_kevent_signal_disable();
    test_kevent_signal_enable();
    test_kevent_signal_oneshot();
#endif
    close(kqfd);
}
