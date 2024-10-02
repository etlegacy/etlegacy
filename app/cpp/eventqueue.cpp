#include "eventqueue.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <mutex>

#include <android/log.h>

#define LOG_TAG "Q3E::idEventManager"

#define LOGD(fmt, args...) { printf("[" LOG_TAG " debug]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args); }
#define LOGI(fmt, args...) { printf("[" LOG_TAG " info]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args); }
#define LOGW(fmt, args...) { printf("[" LOG_TAG " warning]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_WARN, LOG_TAG, fmt, ##args); }
#define LOGE(fmt, args...) { printf("[" LOG_TAG " error]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args); }

#if 1
#define ASSERT(x)
#else
#define ASSERT assert
#endif

typedef enum EventType_e {
    Event_Key = 1,
    Event_Motion,
    Event_Analog,
} EventType_t;

typedef struct AnalogEvent_s
{
    int type;
    bool down;
    float x;
    float y;
} AnalogEvent_t;

typedef struct KeyEvent_s
{
    int type;
    bool down;
    int keycode;
    int charcode;
} KeyEvent_t;

typedef struct MotionEvent_s
{
    int type;
    float deltax;
    float deltay;
} MotionEvent_t;

typedef union Event_s
{
    int type;
    KeyEvent_t key;
    AnalogEvent_t analog;
    MotionEvent_t motion;
} Event_t;

typedef std::list<Event_t *> EventQueue_t;


#define LOCK() const std::lock_guard<std::mutex> _lock(lockMutex);
#define PUSH(e) queue.push_back(/*(Event_t *)*/e);
#define ABORT() std::abort();

// thread-safe: thread 1: java main thread send; thread 2: native game thread pull
class idEventManager
{
public:
    idEventManager();
    virtual ~idEventManager();
    void PushKeyEvent(bool down, int keycode, int charcode);
    void PushMotionEvent(float deltax, float deltay);
    void PushAnalogEvent(bool down, float x, float y);
    void Clear();
    int Num();
    int PullEvent(int num);
    void SetCallback(SendKey_f sendKey, SendMotion_f sendMotion, SendAnalog_f sendAnalog);

private:
    void SendEvent(const Event_t *event);

private:
    EventQueue_t queue;
    std::mutex lockMutex;
    SendKey_f sendKey = nullptr;
    SendMotion_f sendMotion = nullptr;
    SendAnalog_f sendAnalog = nullptr;
};

idEventManager::idEventManager()
/*        : sendKey(nullptr),
          sendMotion(nullptr),
          sendAnalog(nullptr)*/
{
}

idEventManager::~idEventManager()
{
    while (!queue.empty())
    {
        auto &ev = queue.front();
        free(ev);
        queue.pop_front();
    }
}

inline void idEventManager::PushKeyEvent(bool down, int keycode, int charcode)
{
    LOCK();
    Event_t *event = (Event_t *)malloc(sizeof(Event_t));
    KeyEvent_t *ev = &event->key;
    ev->type = Event_Key;
    ev->down = down;
    ev->keycode = keycode;
    ev->charcode = charcode;
    PUSH(event);
}

inline void idEventManager::PushMotionEvent(float deltax, float deltay)
{
    LOCK();
    Event_t *event = (Event_t *)malloc(sizeof(Event_t));
    MotionEvent_t *ev = &event->motion;
    ev->type = Event_Motion;
    ev->deltax = deltax;
    ev->deltay = deltay;
    PUSH(event);
}

inline void idEventManager::PushAnalogEvent(bool down, float x, float y)
{
    LOCK();
    Event_t *event = (Event_t *)malloc(sizeof(Event_t));
    AnalogEvent_t *ev = &event->analog;
    ev->type = Event_Analog;
    ev->down = down;
    ev->x = x;
    ev->y = y;
    PUSH(event);
}

inline void idEventManager::Clear()
{
    LOCK();
    while (!queue.empty())
    {
        auto &ev = queue.front();
        free(ev);
        queue.pop_front();
    }
}

inline int idEventManager::Num()
{
    LOCK();
    return queue.size();
}

inline void idEventManager::SendEvent(const Event_t *ev)
{
    //LOGI("send event type: %d", ev->type);
    switch (ev->type) {
        case Event_Key: {
            const KeyEvent_t &keyEvent = ev->key;
            sendKey(keyEvent.down, keyEvent.keycode, keyEvent.charcode);
        }
            break;
        case Event_Motion: {
            const MotionEvent_t &motionEvent = ev->motion;
            sendMotion(motionEvent.deltax, motionEvent.deltay);
        }
            break;
        case Event_Analog: {
            const AnalogEvent_t &analogEvent = ev->analog;
            sendAnalog(analogEvent.down, analogEvent.x, analogEvent.y);
        }
            break;
        default:
            LOGE("Unexpected event type: %d\n", ev->type);
            ABORT();
            break;
    }
}

int idEventManager::PullEvent(int num)
{
    LOCK();
    if (num < 0) // pull all
    {
        int i = queue.size();
        while (!queue.empty())
        {
            auto &ev = queue.front();
            SendEvent(ev);
            free(ev);
            queue.pop_front();
        }
        return i;
    }
    else if (num > 0) // pull num
    {
        int i = 0;
        while (!queue.empty())
        {
            auto &ev = queue.front();
            SendEvent(ev);
            free(ev);
            queue.pop_front();
            i++;
            if(i >= num)
                break;
        }
        return i;
    }
    else // clear
    {
        while (!queue.empty())
        {
            auto &ev = queue.front();
            free(ev);
            queue.pop_front();
        }
        return 0;
    }
}

void idEventManager::SetCallback(SendKey_f _sendKey, SendMotion_f _sendMotion, SendAnalog_f _sendAnalog)
{
    this->sendKey = _sendKey;
    this->sendMotion = _sendMotion;
    this->sendAnalog = _sendAnalog;
}

// C interface
static idEventManager *eventManager = nullptr;

void Q3E_InitEventManager(SendKey_f sendKey, SendMotion_f sendMotion, SendAnalog_f sendAnalog)
{
    if(eventManager)
    {
        LOGW("idEventManager has initialized\n");
        return;
    }

    LOGI("idEventManager initialization\n");
    eventManager = new idEventManager;
    eventManager->SetCallback(sendKey, sendMotion, sendAnalog);
    LOGI("-----------------------\n");
}

void Q3E_ShutdownEventManager()
{
    if(!eventManager)
    {
        LOGW("idEventManager not initialized\n");
        return;
    }

    LOGI("idEventManager shutdown\n");
    delete eventManager;
    eventManager = nullptr;
    LOGI("-----------------------\n");
}

int Q3E_PullEvent(int num)
{
    ASSERT(eventManager);

    return eventManager->PullEvent(num);
}

void Q3E_PushKeyEvent(int down, int keycode, int charcode)
{
    ASSERT(eventManager);

    eventManager->PushKeyEvent(down, keycode, charcode);
}

void Q3E_PushMotionEvent(float deltax, float deltay)
{
    ASSERT(eventManager);

    eventManager->PushMotionEvent(deltax, deltay);
}

void Q3E_PushAnalogEvent(int down, float x, float y)
{
    ASSERT(eventManager);

    eventManager->PushAnalogEvent(down, x, y);
}

int Q3E_EventManagerIsInitialized()
{
    return eventManager != nullptr;
}
