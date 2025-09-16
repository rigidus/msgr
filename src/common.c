#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

/* ---- ID и время ---- */
void gen_id(char *out, size_t outlen) {
    /* Простая генерация: время+rand. TODO: заменить на ULID/UUIDv7 */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned long r = (unsigned long)random();
    snprintf(out, outlen, "%08lx%08lx%08lx",
             (unsigned long)ts.tv_sec,
             (unsigned long)ts.tv_nsec,
             r);
}

void gen_time(char *out, size_t outlen) {
    struct timespec ts;
    struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    gmtime_r(&ts.tv_sec, &tm);
    snprintf(out, outlen, "%04d-%02d-%02dT%02d-%02d-%02d.%03ldZ",
             tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec,
             ts.tv_nsec/1000000);
}

void make_filename(char *dst, size_t dstlen,
                   const char *time, const char *id, const char *dir)
{
    snprintf(dst, dstlen, "%s__%s__%s", time, id, dir);
}

/* ---- Работа с файлами ---- */
int write_file_atomic(const char *dir, const char *filename,
                      const char *data, size_t len)
{
    char tmp_path[512], final_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s/.tmpXXXXXX", dir);
    snprintf(final_path, sizeof(final_path), "%s/%s", dir, filename);

    int fd = mkstemp(tmp_path);
    if (fd < 0) return -1;

    FILE *f = fdopen(fd, "w");
    if (!f) {
        close(fd);
        unlink(tmp_path);
        return -1;
    }

    if (fwrite(data, 1, len, f) != len) {
        fclose(f);
        unlink(tmp_path);
        return -1;
    }
    fflush(f);
    fsync(fd);
    fclose(f);

    if (rename(tmp_path, final_path) != 0) {
        unlink(tmp_path);
        return -1;
    }
    return 0;
}

char *read_file_all(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(sz+1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    fclose(f);

    buf[sz] = '\0';
    if (out_len) *out_len = sz;
    return buf;
}

/* ---- Основные API ---- */
int insert_new(msg_type_t type, const char *sexpr_str) {
    char time[TIME_MAX_LEN];
    char id[ID_MAX_LEN];
    char filename[256];
    const char *dir;

    gen_time(time, sizeof(time));
    gen_id(id, sizeof(id));

    /* Вытаскиваем :dir из sexpr (очень грубо: ищем ":dir") */
    const char *dirpos = strstr(sexpr_str, ":dir");
    const char *val = (dirpos ? strstr(dirpos, "\"") : NULL);
    char dirflag[8] = "out";
    if (val) {
        sscanf(val+1, "%7[^\"]", dirflag);
    }

    make_filename(filename, sizeof(filename), time, id, dirflag);

    if (type == MSG_PLAIN)
        dir = "msgs";
    else
        dir = "encrypted_msgs";

    return write_file_atomic(dir, filename, sexpr_str, strlen(sexpr_str));
}

static int has_dir_suffix(const char *name, const char *dir_sfx) {
    size_t nlen = strlen(name), slen = strlen(dir_sfx);
    if (slen == 0) return 1;                 /* нет фильтра */
    if (nlen < slen) return 0;
    return (strcmp(name + (nlen - slen), dir_sfx) == 0);
}

/* Возвращает новые строки (malloc). Вызывает scandir, сортирует по имени (alphasort).
 * dir_filter: "in", "out" или NULL (оба).
 */
char **get_all(msg_type_t type, const char *dir_filter, size_t *out_count)
{
    const char *dirpath = (type == MSG_PLAIN) ? "msgs" : "encrypted_msgs";
    struct dirent **namelist = NULL;
    int n = scandir(dirpath, &namelist, NULL, alphasort);
    if (n < 0) { *out_count = 0; return NULL; }

    const char *sfx = NULL;
    char sfx_buf[8] = {0};
    if (dir_filter && (strcmp(dir_filter, "in") == 0 || strcmp(dir_filter, "out") == 0)) {
        snprintf(sfx_buf, sizeof(sfx_buf), "__%s", dir_filter);
        sfx = sfx_buf;
    }

    char **out = NULL;
    size_t cap = 0, cnt = 0;

    for (int i = 0; i < n; i++) {
        struct dirent *de = namelist[i];
        if (de->d_name[0] == '.') { free(de); continue; }  /* . .. .tmp... */
        if (sfx && !has_dir_suffix(de->d_name, sfx)) { free(de); continue; }

        /* Полный путь */
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirpath, de->d_name);

        /* Читаем файл целиком */
        size_t len = 0;
        char *content = read_file_all(path, &len);
        free(de);
        if (!content) continue;

        /* Расширяем массив результов при необходимости */
        if (cnt == cap) {
            size_t newcap = (cap == 0) ? 8 : (cap * 2);
            char **tmp = realloc(out, newcap * sizeof(char*));
            if (!tmp) { free(content); break; }
            out = tmp; cap = newcap;
        }

        out[cnt++] = content;
    }
    free(namelist);

    *out_count = cnt;
    return out;
}
