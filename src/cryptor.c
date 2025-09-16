#include <stddef.h>
#include <stdio.h>

#include "event_loop.h"

static void on_timer(void *ud) {
    (void)ud;
    /* encrypt pass:
       - what_should_be_sent("msg")
       - шифровать (RSA-OAEP)
       - insert_new("emsg")
       - удалить исходный msg
    */
    /* decrypt pass:
       - what_received("emsg")
       - дешифровать
       - insert_new("msg")
       - удалить исходный emsg
    */
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
