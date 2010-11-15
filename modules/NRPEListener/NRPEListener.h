/**************************************************************************
*   Copyright (C) 2004-2007 by Michael Medin <michael@medin.name>         *
*                                                                         *
*   This code is part of NSClient++ - http://trac.nakednuns.org/nscp      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

NSC_WRAPPERS_MAIN();
#include <Socket.h>
#ifdef USE_SSL
#include <SSLSocket.h>
#endif
#include <map>
#include <nrpe/NRPEPacket.hpp>
#include <execute_process.hpp>

class NRPEListener : public simpleSocket::ListenerHandler {
private:
	typedef enum {
		inject, script, script_dir,
	} command_type;
	struct command_data {
		command_data() : type(inject) {}
		command_data(command_type type_, std::wstring arguments_) : type(type_), arguments(arguments_) {}
		command_type type;
		std::wstring arguments;
	};

#ifdef USE_SSL
	bool bUseSSL_;
	simpleSSL::Listener socket_ssl_;
#endif
	simpleSocket::Listener<> socket_;
	typedef std::map<strEx::blindstr, command_data> command_list;
	command_list commands;
	unsigned int timeout;
	unsigned int socketTimeout_;
	socketHelpers::allowedHosts allowedHosts;
	bool noPerfData_;
	std::wstring scriptDirectory_;
	unsigned int buffer_length_;
	std::wstring root_;
	unsigned int max_packet_count_;

public:
	NRPEListener();
	virtual ~NRPEListener();
	// Module calls
	bool loadModule();
	bool unloadModule();


	std::wstring getModuleName() {
#ifdef USE_SSL
		return _T("NRPE server (w/ SSL)");
#else
		return _T("NRPE server");
#endif
	}
	NSCModuleWrapper::module_version getModuleVersion() {
		NSCModuleWrapper::module_version version = {0, 0, 1 };
		return version;
	}
	std::wstring getModuleDescription() {
		return _T("A simple server that listens for incoming NRPE connection and handles them.\nNRPE is preferred over NSClient as it is more flexible. You can of cource use both NSClient and NRPE.");
	}

	bool hasCommandHandler();
	bool hasMessageHandler();
	NSCAPI::nagiosReturn handleCommand(const strEx::blindstr command, const unsigned int argLen, TCHAR **char_args, std::wstring &message, std::wstring &perf);
	std::wstring getConfigurationMeta();

private:
	bool copyOutPaket(int index, NRPEData &data, simpleSocket::DataBuffer &block, bool last);
	bool write_all(simpleSocket::Socket *client, simpleSocket::DataBuffer &block, int timeout);
	bool read_all(simpleSocket::Socket *client, simpleSocket::DataBuffer &block, unsigned int wanted_bytes, int timeout);


private:
	void onAccept(simpleSocket::Socket *client);
	void onClose();


	NRPEData handlePacket(NRPEData p);
	int executeNRPECommand(std::wstring command, std::wstring &msg, std::wstring &perf);
	void addAllScriptsFrom(std::wstring path);
	void addCommand(command_type type, strEx::blindstr key, std::wstring args = _T("")) {
		addCommand(key, command_data(type, args));
	}
	void addCommand(strEx::blindstr key, command_data args) {
		commands[key] = args;
	}

};

