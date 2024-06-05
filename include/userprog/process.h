#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd(const char *file_name);
tid_t process_fork(const char *name, struct intr_frame *if_ UNUSED);
int process_exec(void *f_name);
int process_wait(tid_t);
void process_exit(void);
void process_activate(struct thread *next);

typedef struct lazy_load_info_t
{
   struct file *file; // 로드할 파일의 포인터
   off_t offset;      // 파일 내에서 읽기 시작할 위치
   size_t read_bytes; // 파일에서 읽을 바이트 수
   size_t zero_bytes; // 0으로 채울 바이트 수
} lazy_load_info;

struct lock filesys_lock;

#endif /* userprog/process.h */
