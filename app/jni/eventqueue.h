#ifndef _EVENTQUEUE_H
#define _EVENTQUEUE_H

typedef void (* SendAnalog_f)(int down, float x, float y);
typedef void (* SendKey_f)(int down, int keycode, int charcode);
typedef void (* SendMotion_f)(float deltax, float deltay);

#ifdef __cplusplus
extern "C" {
#endif
void Q3E_InitEventManager(SendKey_f sendKey, SendMotion_f sendMotion, SendAnalog_f sendAnalog);
void Q3E_ShutdownEventManager();
int Q3E_EventManagerIsInitialized();
int Q3E_PullEvent(int num);
void Q3E_PushKeyEvent(int down, int keycode, int charcode);
void Q3E_PushMotionEvent(float deltax, float deltay);
void Q3E_PushAnalogEvent(int down, float x, float y);

#ifdef __cplusplus
};
#endif

#endif //_EVENTQUEUE_H
