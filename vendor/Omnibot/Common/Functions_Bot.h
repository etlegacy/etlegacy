////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_FUNCTIONS_BOT_H
#define INCLUDE_FUNCTIONS_BOT_H

#include "Omni-Bot.h"
#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"
#include "MessageHelper.h"
#include "IEngineInterface.h"

// Title: Functions Bot

// typedef: Bot_EngineFuncs_t
//		This struct defines all the function pointers that the bot will fill in
//		and give to the interface so that the interface can request the entire
//		suite of functions at once from the bot.
typedef struct
{
	omnibot_error (*pfnInitialize)(IEngineInterface *_pEngineFuncs, int _version);
	void (*pfnUpdate)();
	void (*pfnShutdown)();
	void (*pfnConsoleCommand)(const Arguments &_args);

	void (*pfnSendTrigger)(const TriggerInfo &_triggerInfo);
	void (*pfnAddBlackboardRecord)(BlackBoard_Key _type, int _posterID, int _targetID, obUserData *_data);

	// events
	void (*pfnSendEvent)(int _dest, const MessageHelper &_message);
	void (*pfnSendGlobalEvent)(const MessageHelper &_message);

	// goals
	void (*pfnAddGoal)(const MapGoalDef &goaldef);
	void (*pfnDeleteGoal)(const char *goalname);
	void (*pfnUpdateEntity)(GameEntity oldent, GameEntity newent);
} Bot_EngineFuncs_t;

typedef struct
{
	omnibot_error (*pfnInitialize)(IEngineInterface71 *_pEngineFuncs, int _version);
	void (*pfnUpdate)();
	void (*pfnShutdown)();
	void (*pfnConsoleCommand)(const Arguments &_args);
	void (*pfnAddGoal)(const MapGoalDef71 &goaldef);
	void (*pfnSendTrigger)(const TriggerInfo &_triggerInfo);
	void (*pfnAddBlackboardRecord)(BlackBoard_Key _type, int _posterID, int _targetID, obUserData *_data);
	void (*pfnBotEntityAdded)(GameEntity _ent, EntityInfo *_info);
	void (*pfnSendEvent)(int _dest, const MessageHelper &_message);
	void (*pfnSendGlobalEvent)(const MessageHelper &_message);
	void (*pfnUpdateEntity)(GameEntity oldent, GameEntity newent);
	void (*pfnDeleteGoal)(const char *goalname);
} Bot_EngineFuncs71_t;


//#define DllExport __declspec( dllexport )
//
//class DllExport IBotEngineInterface
//{
//	omnibot_error Initialize(IEngineInterface *_pEngineFuncs, int _version) = 0;
//	void Update() = 0;
//	void Shutdown() = 0;
//	void ConsoleCommand(const Arguments &_args) = 0;
//
//	void SendTrigger(const TriggerInfo &_triggerInfo) = 0;
//	void AddBlackboardRecord(BlackBoard_Key _type, int _posterID, int _targetID, obUserData *_data) = 0;
//
//	void AddGoal(const MapGoalDef &goaldef) = 0;
//	void DeleteGoal(const char *goalname) = 0;
//
//	// New message stuff.
//	void SendEvent(int _dest, const MessageHelper &_message) = 0;
//	void SendGlobalEvent(const MessageHelper &_message) = 0;
//	void UpdateEntity(GameEntity oldent,GameEntity newent) = 0;
//
//};

#endif
