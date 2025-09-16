#include "event_loop.h"
#include <stdio.h>
#include <unistd.h>

static void on_timer(void *ud) {
    (void)ud;
    /* тут: what_received("msg") для "in", вывод новых и т.п. */
    /* или — для ui_in_msg — ничего не делаем */
}

static void on_stdin(int fd, void *ud) {
    (void)ud;
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = '\0';
        /* здесь: собрать (msg :dir "out" ...) и insert_new("msg") */
        /* … */
    } else if (n == 0) {
        /* EOF → останавливаем цикл */
        event_loop *el = (event_loop*)ud; /* если хотите передать el */
        (void)el;
        /* el_stop(el); */
    }
}

int main(void) {
    event_loop *el = el_create(1000, on_timer, NULL);
    el_add_fd(el, STDIN_FILENO, on_stdin, NULL);
    int rc = el_run(el);
    el_destroy(el);
    return rc;
}
