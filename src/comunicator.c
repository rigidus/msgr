#include "event_loop.h"
#include <sys/socket.h>
/* ...создаёте сокет, подключаетесь/слушаете... */

static void on_timer(void *ud) {
    /* периодически: send() — забрать emsg:out и попытаться отправить */
}

static void on_sock(int fd, void *ud) {
    /* пришли данные: receive() — преобразовать в (emsg ...) и insert_new("emsg") */
}

int main(void) {
    int sock = /* ... */;
    event_loop *el = el_create(1000, on_timer, NULL);
    el_add_fd(el, sock, on_sock, NULL);
    int rc = el_run(el);
    el_destroy(el);
    return rc;
}
