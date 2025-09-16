#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

/* Максимальные размеры */
#define MSG_MAX_SIZE  1024    /* буфер для строки сообщения */
#define ID_MAX_LEN    64      /* длина id */
#define TIME_MAX_LEN  64      /* длина ISO-времени */

/* Тип сообщения */
typedef enum {
    MSG_PLAIN,   /* (msg ...) */
    MSG_ENCRYPT  /* (emsg ...) */
} msg_type_t;

/* Служебные структуры */
typedef struct {
    char id[ID_MAX_LEN];
    char prev_id[ID_MAX_LEN];
    char time[TIME_MAX_LEN];
    char dir[8];          /* "in" или "out" */
    char data[MSG_MAX_SIZE]; /* текст для (msg ...) */
} msg_plain_t;

typedef struct {
    char id[ID_MAX_LEN];
    char dir[8];          /* "in" или "out" */
    char *cipher_b64;     /* base64 шифротекст (malloc/free) */
} msg_encrypted_t;

/* ---- Утилиты ---- */

/* Генерация ID (ULID-подобный). Пишет в out[ID_MAX_LEN]. */
void gen_id(char *out, size_t outlen);

/* Генерация времени в ISO8601 UTC (YYYY-MM-DDTHH-MM-SS.mmmZ). */
void gen_time(char *out, size_t outlen);

/* Сборка имени файла: <time>__<id>__<dir>.
   dst должен быть достаточно большим (>= 256). */
void make_filename(char *dst, size_t dstlen,
                   const char *time, const char *id, const char *dir);

/* Атомарная запись файла: сначала во временный, затем rename().
   Возвращает 0 при успехе. */
int write_file_atomic(const char *dir, const char *filename,
                      const char *data, size_t len);

/* Чтение всего файла в память. malloc/free.
   Возвращает буфер (null-terminated), *out_len (без \0), или NULL при ошибке. */
char *read_file_all(const char *path, size_t *out_len);

/* ---- API функций, вызываемых бинарниками ---- */

/* Общая функция записи нового сообщения (msg или emsg). */
int insert_new(msg_type_t type, const char *sexpr_str);

/* Чтение всех сообщений из каталога.
   dir_filter = "in", "out" или NULL (оба).
   Возвращает массив строк (malloc/free). */
char **get_all(msg_type_t type,
               const char *dir_filter,
               size_t *out_count);

#endif /* COMMON_H */
