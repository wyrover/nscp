#pragma once

#include <string>
#include <list>
#include <NSCAPI.h>

namespace NSCHelper
{
	int wrapReturnString(char *buffer, unsigned int bufLen, std::string str);
	std::list<std::string> makelist(const unsigned int argLen, char **argument);
	std::string translateMessageType(NSCAPI::messageTypes msgType);
};

namespace NSCModuleHelper
{
	// Types for the Callbacks into the main program
	typedef int (*lpNSAPIGetBasePath)(char*,unsigned int);
	typedef int (*lpNSAPIGetApplicationName)(char*,unsigned int);
	typedef int (*lpNSAPIGetApplicationVersionStr)(char*,unsigned int);
	typedef int (*lpNSAPIGetSettingsString)(const char*,const char*,const char*,char*,unsigned int);
	typedef int (*lpNSAPIGetSettingsInt)(const char*, const char*, int);
	typedef void (*lpNSAPIMessage)(int, const char*, const int, const char*);
	typedef int (*lpNSAPIStopServer)(void);
	typedef int (*lpNSAPIInject)(const char*,char*,unsigned int);
	typedef LPVOID (*lpNSAPILoader)(char*);

	// Helper functions for calling into the core
	std::string getApplicationName(void);
	std::string getApplicationVersionString(void);
	std::string getSettingsString(std::string section, std::string key, std::string defaultValue);
	int getSettingsInt(std::string section, std::string key, int defaultValue);
	void Message(int msgType, std::string file, int line, std::string message);
	std::string InjectCommand(std::string command);
	void StopService(void);
	std::string getBasePath();
};

namespace NSCModuleWrapper {
	struct module_version {
		int major;
		int minor;
		int revision;
	};

	BOOL wrapDllMain(HANDLE hModule, DWORD ul_reason_for_call);
	HINSTANCE getModule();

	int wrapModuleHelperInit(NSCModuleHelper::lpNSAPILoader f);;
	int wrapGetModuleName(char* buf, unsigned int buflen, std::string str);
	int wrapLoadModule(bool success);
	int wrapGetModuleVersion(int *major, int *minor, int *revision, module_version version);
	int wrapHasCommandHandler(bool has);
	int wrapHasMessageHandler(bool has);
	int wrapUnloadModule(bool success);
	int wrapHandleCommand(const std::string retStr, char *returnBuffer, unsigned int returnBufferLen);
}

//////////////////////////////////////////////////////////////////////////
// Module wrappers (definitions)
#define NSC_WRAPPERS_MAIN() \
	extern "C" int NSModuleHelperInit(NSCModuleHelper::lpNSAPILoader f); \
	extern int NSLoadModule(); \
	extern int NSGetModuleName(char* buf, int buflen); \
	extern int NSGetModuleVersion(int *major, int *minor, int *revision); \
	extern int NSHasCommandHandler(); \
	extern int NSHasMessageHandler(); \
	extern void NSHandleMessage(int msgType, char* file, int line, char* message); \
	extern int NSHandleCommand(const char* command, const unsigned int argLen, char **argument, char *returnBuffer, unsigned int returnBufferLen); \
	extern int NSUnloadModule();



#define NSC_LOG_ERROR_STD(msg) NSC_LOG_ERROR(((std::string)msg).c_str())
#define NSC_LOG_ERROR(msg) \
	NSCModuleHelper::Message(NSCAPI::error, __FILE__, __LINE__, msg)

#define NSC_LOG_CRITICAL_STD(msg) NSC_LOG_CRITICAL(((std::string)msg).c_str())
#define NSC_LOG_CRITICAL(msg) \
	NSCModuleHelper::Message(NSCAPI::critical, __FILE__, __LINE__, msg)

#define NSC_LOG_MESSAGE_STD(msg) NSC_LOG_MESSAGE(((std::string)msg).c_str())
#define NSC_LOG_MESSAGE(msg) \
	NSCModuleHelper::Message(NSCAPI::log, __FILE__, __LINE__, msg)

#ifdef _DEBUG
#define NSC_DEBUG_MSG_STD(msg) NSC_DEBUG_MSG(((std::string)msg).c_str())
#define NSC_DEBUG_MSG(msg) \
	NSCModuleHelper::Message(NSCAPI::debug, __FILE__, __LINE__, msg)
#else
#define NSC_DEBUG_MSG_STD(msg)
#define NSC_DEBUG_MSG(msg)
#endif

//////////////////////////////////////////////////////////////////////////
// Message wrappers below this point

#define NSC_WRAPPERS_MAIN_DEF(toObject) \
	extern int NSModuleHelperInit(NSCModuleHelper::lpNSAPILoader f) { \
	return NSCModuleWrapper::wrapModuleHelperInit(f); \
	} \
	extern int NSLoadModule() { \
	return NSCModuleWrapper::wrapLoadModule(toObject.loadModule()); \
	} \
	extern int NSGetModuleName(char* buf, int buflen) { \
	return NSCModuleWrapper::wrapGetModuleName(buf, buflen, toObject.getModuleName()); \
	} \
	extern int NSGetModuleVersion(int *major, int *minor, int *revision) { \
	return NSCModuleWrapper::wrapGetModuleVersion(major, minor, revision, toObject.getModuleVersion()); \
	} \
	extern int NSHasCommandHandler() { \
	return NSCModuleWrapper::wrapHasCommandHandler(toObject.hasCommandHandler()); \
	} \
	extern int NSHasMessageHandler() { \
	return NSCModuleWrapper::wrapHasMessageHandler(toObject.hasMessageHandler()); \
	} \
	extern int NSUnloadModule() { \
	return NSCModuleWrapper::wrapUnloadModule(toObject.unloadModule()); \
	}
#define NSC_WRAPPERS_HANDLE_MSG_DEF(toObject) \
	extern void NSHandleMessage(int msgType, char* file, int line, char* message) { \
	toObject.handleMessage(msgType, file, line, message); \
	}
#define NSC_WRAPPERS_IGNORE_MSG_DEF() \
	extern void NSHandleMessage(int msgType, char* file, int line, char* message) { \
	}
#define NSC_WRAPPERS_HANDLE_CMD_DEF(toObject) \
	extern int NSHandleCommand(const char* command, const unsigned int argLen, char **argument, char *returnBuffer, unsigned int returnBufferLen) { \
		return NSCModuleWrapper::wrapHandleCommand(toObject.handleCommand(command, argLen, argument), returnBuffer, returnBufferLen); \
	}
#define NSC_WRAPPERS_IGNORE_CMD_DEF() \
	extern int NSHandleCommand(const char* command, const unsigned int argLen, char **argument, char *returnBuffer, unsigned int returnBufferLen) { \
	return NSCAPI::failed; \
	}
#define NSC_LOG_DEBUG(str) \
	NSCHelper::DebugMessage(__FILE__, __LINE__, str);