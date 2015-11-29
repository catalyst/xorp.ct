// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



// #define DEBUG_LOGGING

#include "rip_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/xorp.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "xrl_config.hh"
#include "xrl_process_spy.hh"

XrlProcessSpy::XrlProcessSpy(XrlRouter& rtr)
	: ServiceBase("FEA/RIB Process Watcher"), _rtr(rtr)
{
	_cname[FEA_IDX] = xrl_fea_name();
	_cname[RIB_IDX] = xrl_rib_name();
}

XrlProcessSpy::~XrlProcessSpy()
{
}

bool
XrlProcessSpy::fea_present() const
{
	if (status() == SERVICE_RUNNING)
		return _iname[FEA_IDX].empty() == false;
	debug_msg("XrlProcessSpy::fea_present() called when not "
			"SERVICE_RUNNING.\n");
	return false;
}

bool
XrlProcessSpy::rib_present() const
{
	if (status() == SERVICE_RUNNING)
		return _iname[RIB_IDX].empty() == false;
	debug_msg("XrlProcessSpy::rib_present() called when not "
			"SERVICE_RUNNING.\n");
	return false;
}


	void
XrlProcessSpy::birth_event(const string& class_name,
		const string& instance_name)
{
	debug_msg("Birth event: class %s instance %s\n",
			class_name.c_str(), instance_name.c_str());

	for (uint32_t i = 0; i < END_IDX; i++) 
	{
		if (class_name != _cname[i]) 
		{
			continue;
		}
		if (_iname[i].empty() == false) 
		{
			XLOG_WARNING("Got ");
		}
		_iname[i] = instance_name;
	}
}

	void
XrlProcessSpy::death_event(const string& class_name,
		const string& instance_name)
{
	debug_msg("Death event: class %s instance %s\n",
			class_name.c_str(), instance_name.c_str());

	for (uint32_t i = 0; i < END_IDX; i++) 
	{
		if (class_name != _cname[i]) 
		{
			continue;
		}
		if (_iname[i] == instance_name) 
		{
			_iname[i].erase();
			return;
		}
	}
}


	int
XrlProcessSpy::startup()
{
	if (status() == SERVICE_READY || status() == SERVICE_SHUTDOWN) 
	{
		send_register(0);
		set_status(SERVICE_STARTING);
	}

	return (XORP_OK);
}

	void
XrlProcessSpy::schedule_register_retry(uint32_t idx)
{
	_retry = EventLoop::instance().new_oneoff_after_ms(100,
			callback(this,
				&XrlProcessSpy::send_register,
				idx));
}

	void
XrlProcessSpy::send_register(uint32_t idx)
{
	XrlFinderEventNotifierV0p1Client x(&_rtr);
	if (x.send_register_class_event_interest("finder", _rtr.instance_name(),
				_cname[idx], callback(this, &XrlProcessSpy::register_cb, idx))
			== false) 
	{
		XLOG_ERROR("Failed to send interest registration for \"%s\"\n",
				_cname[idx].c_str());
		schedule_register_retry(idx);
	}
}

	void
XrlProcessSpy::register_cb(const XrlError& xe, uint32_t idx)
{
	if (XrlError::OKAY() != xe) 
	{
		XLOG_ERROR("Failed to register interest in \"%s\": %s\n",
				_cname[idx].c_str(), xe.str().c_str());
		schedule_register_retry(idx);
		return;
	}
	debug_msg("Registered interest in %s\n", _cname[idx].c_str());
	idx++;
	if (idx < END_IDX) 
	{
		send_register(idx);
	} else 
	{
		set_status(SERVICE_RUNNING);
	}
}


	int
XrlProcessSpy::shutdown()
{
	if (status() == SERVICE_RUNNING) 
	{
		send_deregister(0);
		set_status(SERVICE_SHUTTING_DOWN);
	}

	return (XORP_OK);
}

	void
XrlProcessSpy::schedule_deregister_retry(uint32_t idx)
{
	_retry = EventLoop::instance().new_oneoff_after_ms(100,
			callback(this,
				&XrlProcessSpy::send_deregister,
				idx));
}

	void
XrlProcessSpy::send_deregister(uint32_t idx)
{
	XrlFinderEventNotifierV0p1Client x(&_rtr);
	if (x.send_deregister_class_event_interest(
				"finder", _rtr.instance_name(), _cname[idx],
				callback(this, &XrlProcessSpy::deregister_cb, idx))
			== false) 
	{
		XLOG_ERROR("Failed to send interest registration for \"%s\"\n",
				_cname[idx].c_str());
		schedule_deregister_retry(idx);
	}
}

	void
XrlProcessSpy::deregister_cb(const XrlError& xe, uint32_t idx)
{
	if (XrlError::OKAY() != xe) 
	{
		XLOG_ERROR("Failed to deregister interest in \"%s\": %s\n",
				_cname[idx].c_str(), xe.str().c_str());
		schedule_deregister_retry(idx);
		return;
	}
	debug_msg("Deregistered interest in %s\n", _cname[idx].c_str());
	idx++;
	if (idx < END_IDX) 
	{
		send_deregister(idx);
	} else 
	{
		set_status(SERVICE_SHUTDOWN);
	}
}
