#ifndef DOBBY_H
#define DOBBY_H
#ifdef __cplusplus
extern "C" {
#endif
int DobbyHook(void *address, void *replace_call, void **origin_call);
int DobbyDestroy(void *address);
#ifdef __cplusplus
}
#endif
#endif
