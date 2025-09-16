#define _XOPEN_SOURCE 700
#include "event_loop.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#ifndef EL_MAX_FDS
#define EL_MAX_FDS 64   /* достаточно для наших процессов; при необходимости увеличьте */
#endif

typedef struct {
    int fd;
    el_readable_cb cb;
    void *ud;
} el_fd_entry;

struct event_loop {
    int running;
    int interval_ms;

    /* таймер */
    el_timer_cb on_timer;
    void *timer_ud;
    struct timeval next_tick;

    /* fds */
    el_fd_entry fds[EL_MAX_FDS];
    int fds_count;

    /* wakeup */
    el_wakeup_cb on_wakeup;
    void *wakeup_ud;
};

/* утилиты времени */
static void tv_now(struct timeval *tv) {
    gettimeofday(tv, NULL);
}
static long tv_diff_ms(const struct timeval *a, const struct timeval *b) {
    /* a - b */
    long sec  = (long)a->tv_sec  - (long)b->tv_sec;
    long usec = (long)a->tv_usec - (long)b->tv_usec;
    return sec*1000 + usec/1000;
}
static void tv_add_ms(struct timeval *dst, const struct timeval *src, int ms) {
    dst->tv_sec  = src->tv_sec  + ms/1000;
    dst->tv_usec = src->tv_usec + (ms%1000)*1000;
    if (dst->tv_usec >= 1000000) {
        dst->tv_sec += 1;
        dst->tv_usec -= 1000000;
    }
}

/* SIGINT/SIGTERM — мягкая остановка цикла */
static volatile sig_atomic_t g_should_stop = 0;
static void el_sigint_handler(int sig) { (void)sig; g_should_stop = 1; }

event_loop* el_create(int interval_ms, el_timer_cb on_timer, void *timer_ud) {
    event_loop *el = (event_loop*)calloc(1, sizeof(*el));
    if (!el) return NULL;
    el->interval_ms = (interval_ms > 0) ? interval_ms : 1000;
    el->on_timer = on_timer;
    el->timer_ud = timer_ud;
    tv_now(&el->next_tick);
    tv_add_ms(&el->next_tick, &el->next_tick, el->interval_ms);
    return el;
}

void el_destroy(event_loop *el) {
    if (el) free(el);
}

int el_add_fd(event_loop *el, int fd, el_readable_cb on_readable, void *ud) {
    if (!el || fd < 0 || !on_readable) return -1;
    if (el->fds_count >= EL_MAX_FDS) return -1;
    /* не добавляем дубли */
    for (int i=0;i<el->fds_count;i++) if (el->fds[i].fd == fd) return 0;
    el->fds[el->fds_count++] = (el_fd_entry){ .fd = fd, .cb = on_readable, .ud = ud };
    return 0;
}

int el_remove_fd(event_loop *el, int fd) {
    if (!el || fd < 0) return -1;
    for (int i=0;i<el->fds_count;i++) {
        if (el->fds[i].fd == fd) {
            el->fds[i] = el->fds[--el->fds_count];
            return 0;
        }
    }
    return -1;
}

void el_set_wakeup_cb(event_loop *el, el_wakeup_cb on_wakeup, void *ud) {
    if (!el) return;
    el->on_wakeup = on_wakeup;
    el->wakeup_ud = ud;
}

void el_stop(event_loop *el) {
    if (el) el->running = 0;
}

int el_run(event_loop *el) {
    if (!el) return -1;

    /* Устанавливаем обработчики сигналов для мягкой остановки */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = el_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    el->running = 1;
    while (el->running && !g_should_stop) {
        struct timeval now; tv_now(&now);

        /* вычисляем таймаут до следующего тика */
        long ms_left = tv_diff_ms(&el->next_tick, &now);
        if (ms_left < 0) ms_left = 0;              /* тик просрочен — сработаем сразу */

        struct timeval tv;
        tv.tv_sec  = ms_left / 1000;
        tv.tv_usec = (ms_left % 1000) * 1000;

        /* собираем набор дескрипторов */
        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = -1;
        for (int i=0;i<el->fds_count;i++) {
            FD_SET(el->fds[i].fd, &rfds);
            if (el->fds[i].fd > maxfd) maxfd = el->fds[i].fd;
        }

        int ret = select(maxfd+1, (maxfd>=0 ? &rfds : NULL), NULL, NULL, &tv);

        if (ret < 0) {
            if (errno == EINTR) {
                /* нас разбудил сигнал, даём шанс on_wakeup */
                if (el->on_wakeup) el->on_wakeup(el->wakeup_ud);
                continue;
            }
            return -1; /* критическая ошибка select */
        }

        /* готовы fd? */
        if (ret > 0) {
            for (int i=0;i<el->fds_count;i++) {
                int fd = el->fds[i].fd;
                if (FD_ISSET(fd, &rfds)) {
                    el->fds[i].cb(fd, el->fds[i].ud);
                }
            }
        } else {
            /* ret == 0 → таймаут */
        }

        /* проверяем, пора ли вызывать on_timer (могло пройти несколько периодов) */
        tv_now(&now);
        while (tv_diff_ms(&now, &el->next_tick) >= 0) {
            if (el->on_timer) el->on_timer(el->timer_ud);
            /* следующий дедлайн */
            tv_add_ms(&el->next_tick, &el->next_tick, el->interval_ms);
            /* если мы сильно отстаём, «догоняем» циклом while */
        }
    }
    return 0;
}
