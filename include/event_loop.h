#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Колбэки */
    typedef void (*el_timer_cb)(void *ud);                   /* срабатывает раз в interval_ms */
    typedef void (*el_readable_cb)(int fd, void *ud);        /* дескриптор готов к чтению */
    typedef void (*el_wakeup_cb)(void *ud);                  /* редкий колбэк: возврат после select() без fd (EINTR/др.) */

/* Хэндл цикла */
    typedef struct event_loop event_loop;

/* Создать/уничтожить цикл.
 * interval_ms — период таймера (например, 1000 для 1 секунды).
 * on_timer — колбэк таймера (может быть NULL).
 */
    event_loop* el_create(int interval_ms, el_timer_cb on_timer, void *timer_ud);
    void        el_destroy(event_loop *el);

/* Управление: запустить/остановить цикл */
    int  el_run(event_loop *el);    /* блокирует поток до el_stop() или ошибки */
    void el_stop(event_loop *el);

/* Регистрация дескрипторов (stdin, сокеты, пайпы, FIFO) на "чтение".
 * Возвращает 0 при успехе.
 */
    int  el_add_fd(event_loop *el, int fd, el_readable_cb on_readable, void *ud);
    int  el_remove_fd(event_loop *el, int fd);

/* Необязательный колбэк для «просыпания» без готовых fd (например, сигнал прервал select()) */
    void el_set_wakeup_cb(event_loop *el, el_wakeup_cb on_wakeup, void *ud);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_LOOP_H */
