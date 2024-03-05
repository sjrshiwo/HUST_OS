/*
 * header file to be used by applications.
 */

int printu(const char *s, ...);
int exit(int code);
void* naive_malloc();
void naive_free(void* va);
int fork();
int sem_new(int a1);
int sem_P(int a1);
int sem_V(int a1);
void yield();
