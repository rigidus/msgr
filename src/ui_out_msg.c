#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "event_loop.h"
#include "common.h"

/* Таймер: каждую секунду проверяем новые входящие сообщения */
static void on_timer(void *ud) {
    (void)ud;

    size_t count = 0;
    char **msgs = get_all(MSG_PLAIN, "in", &count);
    if (!msgs || count == 0) {
        free(msgs);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        printf("%s\n\n", msgs[i]);
        free(msgs[i]);
    }
    free(msgs);

    fflush(stdout);
}

int main(void) {
    event_loop *el = el_create(/*interval_ms=*/1000, on_timer, NULL);
    if (!el) {
        fprintf(stderr, "failed to init event loop\n");
        return 1;
    }
    int rc = el_run(el);
    el_destroy(el);
    return rc;
}
