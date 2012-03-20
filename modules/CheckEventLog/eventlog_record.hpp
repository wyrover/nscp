#pragma once

#include "simple_registry.hpp"
#include <config.h>
#include <boost/tuple/tuple.hpp>

class EventLogRecord {
	const EVENTLOGRECORD *pevlr_;
	__int64 currentTime_;
	std::wstring file_;
public:
	EventLogRecord(std::wstring file, const EVENTLOGRECORD *pevlr, __int64 currentTime) : file_(file), pevlr_(pevlr), currentTime_(currentTime) {
	}
	inline __int64 timeGenerated() const {
		return (currentTime_-pevlr_->TimeGenerated)*1000;
	}
	inline __int64 timeWritten() const {
		return (currentTime_-pevlr_->TimeWritten)*1000;
	}
	inline __int64 generated() const {
		return pevlr_->TimeGenerated;
	}
	inline __int64 written() const {
		return pevlr_->TimeWritten;
	}
	inline WORD category() const {
		return pevlr_->EventCategory;
	}
	inline std::wstring get_source() const {
		return reinterpret_cast<const WCHAR*>(reinterpret_cast<const BYTE*>(pevlr_) + sizeof(EVENTLOGRECORD));
	}
	inline std::wstring get_computer() const {
		size_t len = wcslen(reinterpret_cast<const WCHAR*>(reinterpret_cast<const BYTE*>(pevlr_) + sizeof(EVENTLOGRECORD)));
		return reinterpret_cast<const WCHAR*>(reinterpret_cast<const BYTE*>(pevlr_) + sizeof(EVENTLOGRECORD) + (len+1)*sizeof(wchar_t));
	}
	inline DWORD eventID() const {
		return (pevlr_->EventID&0xffff);
	}
	inline DWORD severity() const {
		return (pevlr_->EventID>>30) & 0x3;
	}
	inline DWORD facility() const {
		return (pevlr_->EventID>>16) & 0xfff;
	}
	inline WORD customer() const {
		return (pevlr_->EventID>>29) & 0x1;
	}
	inline DWORD raw_id() const {
		return pevlr_->EventID;
	}

	inline DWORD eventType() const {
		return pevlr_->EventType;
	}

	std::wstring userSID() const {
		if (pevlr_->UserSidOffset == 0)
			return _T("");
		PSID p = NULL; // = reinterpret_cast<const void*>(reinterpret_cast<const BYTE*>(pevlr_) + + pevlr_->UserSidOffset);
		DWORD userLen = 0;
		DWORD domainLen = 0;
		SID_NAME_USE sidName;

		LookupAccountSid(NULL, p, NULL, &userLen, NULL, &domainLen, &sidName);
		LPTSTR user = new TCHAR[userLen+10];
		LPTSTR domain = new TCHAR[domainLen+10];

		LookupAccountSid(NULL, p, user, &userLen, domain, &domainLen, &sidName);
		user[userLen] = 0;
		domain[domainLen] = 0;
		std::wstring ustr = user;
		std::wstring dstr = domain;
		delete [] user;
		delete [] domain;
		if (!dstr.empty())
			dstr = dstr + _T("\\");
		if (ustr.empty() && dstr.empty())
			return _T("missing");

		return dstr + ustr;
	}

	std::wstring enumStrings() const {
		std::wstring ret;
		const TCHAR* p = reinterpret_cast<const TCHAR*>(reinterpret_cast<const BYTE*>(pevlr_) + pevlr_->StringOffset);
		for (unsigned int i =0;i<pevlr_->NumStrings;i++) {
			std::wstring s = p;
			if (!s.empty())
				s += _T(", ");
			ret += s;
			p = &p[wcslen(p) + 1];
		}
		return ret;
	}

	static DWORD appendType(DWORD dwType, std::wstring sType) {
		return dwType | translateType(sType);
	}
	static DWORD subtractType(DWORD dwType, std::wstring sType) {
		return dwType & (!translateType(sType));
	}
	static DWORD translateType(std::wstring sType) {
		if (sType.empty())
			return EVENTLOG_ERROR_TYPE;
		if (sType == _T("error"))
			return EVENTLOG_ERROR_TYPE;
		if (sType == _T("warning"))
			return EVENTLOG_WARNING_TYPE;
		if (sType == _T("success"))
			return EVENTLOG_SUCCESS;
		if (sType == _T("info"))
			return EVENTLOG_INFORMATION_TYPE;
		if (sType == _T("auditSuccess"))
			return EVENTLOG_AUDIT_SUCCESS;
		if (sType == _T("auditFailure"))
			return EVENTLOG_AUDIT_FAILURE;
		return strEx::stoi(sType);
	}
	static std::wstring translateType(DWORD dwType) {
		if (dwType == EVENTLOG_ERROR_TYPE)
			return _T("error");
		if (dwType == EVENTLOG_WARNING_TYPE)
			return _T("warning");
		if (dwType == EVENTLOG_SUCCESS)
			return _T("success");
		if (dwType == EVENTLOG_INFORMATION_TYPE)
			return _T("info");
		if (dwType == EVENTLOG_AUDIT_SUCCESS)
			return _T("auditSuccess");
		if (dwType == EVENTLOG_AUDIT_FAILURE)
			return _T("auditFailure");
		return strEx::itos(dwType);
	}
	static DWORD translateSeverity(std::wstring sType) {
		if (sType.empty())
			return 0;
		if (sType == _T("success"))
			return 0;
		if (sType == _T("informational"))
			return 1;
		if (sType == _T("warning"))
			return 2;
		if (sType == _T("error"))
			return 3;
		return strEx::stoi(sType);
	}
	static std::wstring translateSeverity(DWORD dwType) {
		if (dwType == 0)
			return _T("success");
		if (dwType == 1)
			return _T("informational");
		if (dwType == 2)
			return _T("warning");
		if (dwType == 3)
			return _T("error");
		return strEx::itos(dwType);
	}
	std::wstring get_dll() const {
		try {
			return simple_registry::registry_key::get_string(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\") + file_ + (std::wstring)_T("\\") + get_source(), _T("EventMessageFile"));
		} catch (simple_registry::registry_exception &e) {
			NSC_LOG_ERROR_STD(_T("Could not extract DLL for eventsource: ") + get_source() + _T(": ") + e.what());
			return _T("");
		}
	}

	struct tchar_array {
		TCHAR **buffer;
		unsigned int size;
		tchar_array(unsigned int size) : buffer(NULL), size(size) {
			buffer = new TCHAR*[size];
			for (int i=0;i<size;i++) 
				buffer[i] = NULL;
		}
		~tchar_array() {
			for (int i=0;i<size;i++) 
				delete [] buffer[i];
			delete [] buffer;
		}
		unsigned int set(int i, const TCHAR* str) {
			unsigned int len = wcslen(str);
			buffer[i] = new TCHAR[len+2];
			wcsncpy(buffer[i], str, len+1);
			return len;
		}
		TCHAR** get_buffer_unsafe() { return buffer; }

	};

	boost::tuple<DWORD,std::wstring> safe_format(HMODULE hDLL, DWORD dwLang) const {
		LPVOID lpMsgBuf;
		unsigned long dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ARGUMENT_ARRAY|FORMAT_MESSAGE_IGNORE_INSERTS,hDLL,
			pevlr_->EventID,dwLang,(LPTSTR)&lpMsgBuf,0,NULL);
		if (dwRet == 0) {
			return boost::tuple<DWORD,std::wstring>(GetLastError(), _T(""));
		}
		std::wstring msg = reinterpret_cast<wchar_t*>(lpMsgBuf);
		LocalFree(lpMsgBuf);
		const TCHAR* p = reinterpret_cast<const TCHAR*>(reinterpret_cast<const BYTE*>(pevlr_) + pevlr_->StringOffset);
		for (unsigned int i = 0;i<pevlr_->NumStrings;i++) {
			strEx::replace(msg, _T("%")+strEx::itos(i+1), std::wstring(p));
			unsigned int len = wcslen(p);
			p = &(p[len+1]);
		}
		return boost::make_tuple(0, msg);
	}
	std::wstring render_message(DWORD dwLang = 0) const {
		std::vector<std::wstring> args;
		std::wstring ret;
		strEx::splitList dlls = strEx::splitEx(get_dll(), _T(";"));
		for (strEx::splitList::const_iterator cit = dlls.begin(); cit != dlls.end(); ++cit) {
			//std::wstring msg = error::format::message::from_module((*cit), eventID(), _sz);
			std::wstring msg;
			try {
				HMODULE hDLL = LoadLibraryEx((*cit).c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
				if (hDLL == NULL) {
					msg = _T("failed to load: ") + (*cit) + _T(", reason: ") + strEx::itos(GetLastError());
					continue;
				}
				if (dwLang == 0)
					dwLang = MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT);
				boost::tuple<DWORD,std::wstring> formated_data = safe_format(hDLL, dwLang);
				if (formated_data.get<0>() != 0) {
					FreeLibrary(hDLL);
					if (formated_data.get<0>() == 15100) {
						// Invalid MUI file (wrong language)
						msg = _T("");
						continue;
					}
					if (formated_data.get<0>() == 317) {
						// Missing message
						msg = _T("");
						continue;
					}
					msg = _T("failed to lookup error code: ") + strEx::itos(eventID()) + _T(" from DLL: ") + (*cit) + _T("( reason: ") + strEx::itos(formated_data.get<0>()) + _T(")");
					continue;
				}
				FreeLibrary(hDLL);
				msg = formated_data.get<1>();
			} catch (...) {
				msg = _T("Unknown exception getting message");
			}
			strEx::replace(msg, _T("\n"), _T(" "));
			strEx::replace(msg, _T("\t"), _T(" "));
			std::string::size_type pos = msg.find_last_not_of(_T("\n\t "));
			if (pos != std::string::npos) {
				msg = msg.substr(0,pos);
			}
			if (!msg.empty()) {
				if (!ret.empty())
					ret += _T(", ");
				ret += msg;
			}
		}
		return ret;
	}
	SYSTEMTIME get_time(DWORD time) const {
		FILETIME FileTime, LocalFileTime;
		SYSTEMTIME SysTime;
		__int64 lgTemp;
		__int64 SecsTo1970 = 116444736000000000;

		lgTemp = Int32x32To64(time,10000000) + SecsTo1970;

		FileTime.dwLowDateTime = (DWORD) lgTemp;
		FileTime.dwHighDateTime = (DWORD)(lgTemp >> 32);

		FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
		FileTimeToSystemTime(&LocalFileTime, &SysTime);
		return SysTime;
	}

	SYSTEMTIME get_time_generated() const {
		return get_time(pevlr_->TimeGenerated);
	}
	SYSTEMTIME get_time_written() const {
		return get_time(pevlr_->TimeWritten);
	}
	inline std::wstring get_log() {
		return file_;
	}

	std::wstring render(bool propper, std::wstring syntax, std::wstring date_format = DATE_FORMAT, DWORD langId = 0) const {
		if (propper) {
			// To obtain the appropriate message string from the message file, load the message file with the LoadLibrary function and use the FormatMessage function
			strEx::replace(syntax, _T("%message%"), render_message(langId));
		} else {
			strEx::replace(syntax, _T("%message%"), _T("%message% needs the descriptions flag set!"));
		}

		strEx::replace(syntax, _T("%source%"), get_source());
		strEx::replace(syntax, _T("%computer%"), get_computer());
		strEx::replace(syntax, _T("%generated%"), strEx::format_date(get_time_generated(), date_format));
		strEx::replace(syntax, _T("%written%"), strEx::format_date(get_time_written(), date_format));
		strEx::replace(syntax, _T("%generated-raw%"), strEx::itos(pevlr_->TimeGenerated));
		strEx::replace(syntax, _T("%written-raw%"), strEx::itos(pevlr_->TimeWritten));
		strEx::replace(syntax, _T("%type%"), translateType(eventType()));
		strEx::replace(syntax, _T("%category%"), strEx::itos(pevlr_->EventCategory));
		strEx::replace(syntax, _T("%facility%"), strEx::itos(facility()));
		strEx::replace(syntax, _T("%qualifier%"), strEx::itos(facility()));
		strEx::replace(syntax, _T("%customer%"), strEx::itos(customer()));
		strEx::replace(syntax, _T("%rawid%"), strEx::itos(raw_id()));
		strEx::replace(syntax, _T("%severity%"), translateSeverity(severity()));
		strEx::replace(syntax, _T("%strings%"), enumStrings());
		strEx::replace(syntax, _T("%log%"), file_);
		strEx::replace(syntax, _T("%file%"), file_);
		strEx::replace(syntax, _T("%id%"), strEx::itos(eventID()));
		strEx::replace(syntax, _T("%user%"), userSID());
		return syntax;
	}
};