#include <stdio.h>
#include <stdlib.h>     /* NULL */
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "event_loop.h"
#include "common.h"

/* --- простое экранирование для строки внутри S-expr --- */
static void escape_for_sexpr(const char *in, char *out, size_t out_cap) {
    /* экранируем \, " и перевод строки как \n */
    size_t o = 0;
    for (size_t i = 0; in[i] && o + 2 < out_cap; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '\\' || c == '\"') {
            out[o++] = '\\';
            out[o++] = (char)c;
        } else if (c == '\n' || c == '\r') {
            if (o + 2 < out_cap) {
                out[o++] = '\\'; out[o++] = 'n';
            }
        } else {
            out[o++] = (char)c;
        }
    }
    out[o] = '\0';
}

/* --- проверка лимита RSA-4096 OAEP (446 байт) --- */
static int fits_rsa_plaintext_limit(const char *sexpr) {
    size_t len = strlen(sexpr);
    return (len <= 446);
}

/* --- аккумулируем куски stdin до \n --- */
typedef struct {
    char buf[4096];
    size_t len;
} linebuf_t;

static void process_line(const char *line) {
    /* отбросим \r\n и пустые строки */
    size_t n = strlen(line);
    while (n > 0 && (line[n-1] == '\n' || line[n-1] == '\r')) n--;
    if (n == 0) return;

    /* сформируем поля */
    char id[ID_MAX_LEN];     gen_id(id, sizeof id);
    char tim[TIME_MAX_LEN];  gen_time(tim, sizeof tim);

    /* экранируем содержимое */
    char esc[MSG_MAX_SIZE];
    escape_for_sexpr(line, esc, sizeof esc);

    /* соберём S-expr */
    char sexpr[2048];
    int written = snprintf(sexpr, sizeof sexpr,
                           "(msg :id \"%s\" :time \"%s\" :dir \"out\" :data \"%s\")",
                           id, tim, esc);

    if (written <= 0 || (size_t)written >= sizeof sexpr) {
        fprintf(stderr, "[ui_in_msg] error: message too long after escaping\n");
        return;
    }

    if (!fits_rsa_plaintext_limit(sexpr)) {
        fprintf(stderr, "[ui_in_msg] error: plaintext exceeds 446 bytes, skipped\n");
        return;
    }

    /* запись в msgs/ */
    if (insert_new(MSG_PLAIN, sexpr) != 0) {
        fprintf(stderr, "[ui_in_msg] error: failed to write message\n");
    } else {
        /* опционально подтверждаем */
        fprintf(stdout, "[ui_in_msg] queued: %s\n", id);
        fflush(stdout);
    }
}

static void on_stdin(int fd, void *ud) {
    (void)fd;
    linebuf_t *lb = (linebuf_t*)ud;

    char tmp[1024];
    ssize_t r = read(STDIN_FILENO, tmp, sizeof tmp);
    if (r <= 0) return;

    size_t off = 0;
    while (off < (size_t)r) {
        /* копим до \n */
        while (off < (size_t)r && lb->len < sizeof(lb->buf)-1) {
            char c = tmp[off++];
            lb->buf[lb->len++] = c;
            if (c == '\n') break;
        }
        lb->buf[lb->len] = '\0';

        /* есть ли полная строка */
        char *nl = strchr(lb->buf, '\n');
        if (nl) {
            /* выделим первую строку */
            *nl = '\0';
            process_line(lb->buf);

            /* сдвиг хвоста (если есть) в начало буфера */
            size_t rest = lb->len - (size_t)(nl - lb->buf + 1);
            memmove(lb->buf, nl + 1, rest);
            lb->len = rest;
            lb->buf[lb->len] = '\0';
        }
    }
}

static void on_timer(void *ud) {
    (void)ud;
    /* для ui_in_msg таймер ничего не делает; оставлен для симметрии */
}

int main(void) {
    linebuf_t lb; lb.len = 0; lb.buf[0] = '\0';

    event_loop *el = el_create(1000, on_timer, NULL);
    if (!el) {
        fprintf(stderr, "failed to init event loop\n");
        return 1;
    }
    /* регистрируем STDIN */
    el_add_fd(el, STDIN_FILENO, on_stdin, &lb);

    int rc = el_run(el);
    el_destroy(el);
    return rc;
}
