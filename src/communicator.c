#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "event_loop.h"
#include "common.h"

/* --- Отправка (заглушка) --- */
static void send_outgoing(void) {
    size_t count = 0;
    char **emsgs = get_all(MSG_ENCRYPT, "out", &count);
    if (!emsgs || count == 0) {
        free(emsgs);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        printf("[communicator] sending: %s\n", emsgs[i]);
        free(emsgs[i]);
    }
    free(emsgs);

    fflush(stdout);
}

/* --- Приём (заглушка) --- */
static void receive_incoming(void) {
    /* В реальности здесь будет код работы с сетью (IMAP/IRC).
       Пока просто создадим фиктивное входящее сообщение. */

    const char *dummy =
        "(emsg :id \"DUMMY123\" :dir \"in\" :cipher \"QkFTRTY0REFUQQ==\")";

    insert_new(MSG_ENCRYPT, dummy);
    printf("[communicator] received dummy emsg\n");
}

/* --- Таймер: раз в секунду --- */
static void on_timer(void *ud) {
    (void)ud;
    send_outgoing();
    receive_incoming();
}

int main(void) {
    event_loop *el = el_create(1000, on_timer, NULL);
    if (!el) {
        fprintf(stderr, "failed to init event loop\n");
        return 1;
    }
    int rc = el_run(el);
    el_destroy(el);
    return rc;
}
