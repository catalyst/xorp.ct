// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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



// Access the profiling support in XORP processes.

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "rtrmgr_module.h"

#include <stdio.h>

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/status_codes.h"
#include "libxorp/callback.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#endif

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/profile_xif.hh"

#include "xrl/targets/profiler_base.hh"


class XrlProfilerTarget :  XrlProfilerTargetBase {
 public:
    XrlProfilerTarget(XrlRouter *r)
	:  XrlProfilerTargetBase(r), _xrl_router(r), _done(false)
    {}

    XrlCmdError
    common_0_1_get_target_name(string& name)
    {
	name = "profiler";
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_get_version(string& version)
    {
	version = "0.1";
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_get_status(uint32_t& status, string& /*reason*/)
    {
	status =  PROC_READY;
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_shutdown()
    {
	_done = true;
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_startup() {
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    profile_client_0_1_log(const string& pname,	const uint32_t&	sec,
			   const uint32_t& usec, const string&	comment)
    {
	printf("%s %u %u %s\n", pname.c_str(), XORP_UINT_CAST(sec),
	       XORP_UINT_CAST(usec), comment.c_str());

	return XrlCmdError::OKAY();
    }

    XrlCmdError 
    profile_client_0_1_finished(const string& /*pname*/)
    {
	_done = true;
	return XrlCmdError::OKAY();
    }

    void
    list(const string& target)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_list(target.c_str(),
			  callback(this, &XrlProfilerTarget::list_cb));
    }

    void
    list_cb(const XrlError& error, const string* list)
    {
	_done = true;
	if(XrlError::OKAY() != error) {
	    XLOG_WARNING("callback: %s", error.str().c_str());
	    return;
	}
	printf("%s", list->c_str());
    }

    void
    enable(const string& target, const string& pname)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_enable(target.c_str(),
			    pname,
			    callback(this, &XrlProfilerTarget::cb));
    }

    void
    disable(const string& target, const string& pname)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_disable(target.c_str(),
			     pname,
			     callback(this, &XrlProfilerTarget::cb));
    }

    void
    clear(const string& target, const string& pname)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_clear(target.c_str(),
			   pname,
			   callback(this, &XrlProfilerTarget::cb));
    }

    void
    get(const string& target, const string& pname)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_get_entries(target.c_str(),
				 pname,
				 _xrl_router->instance_name(),
				 callback(this, &XrlProfilerTarget::get_cb));
    }

    void
    get_cb(const XrlError& error)
    {
	// If there is an error then set done to be true.
	if(XrlError::OKAY() != error) {
	    _done = true;
	    XLOG_WARNING("callback: %s", error.str().c_str());
	}
    }

    void
    cb(const XrlError& error)
    {
	_done = true;
	if(XrlError::OKAY() != error) {
	    XLOG_WARNING("callback: %s", error.str().c_str());
	}
    }

    bool done() const { return _done; }

 private:
    XrlRouter *_xrl_router;
    bool _done;
};

int
usage(const char *myname)
{
    fprintf(stderr,
	    "usage: %s -t target {-l | -v profile variable [-e|-d|-c|-g]}\n",
	    myname);
    return 1;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    string target;	// Target name
    string pname;	// Profile variable
    string command;	// Command
    bool requires_pname = false;

    int c;
    while ((c = getopt(argc, argv, "t:lv:edcg")) != -1) {
	switch (c) {
	case 't':
	    target = optarg;
	    break;
	case 'l':
	    command = "list";
	    break;
	case 'v':
	    pname = optarg;
	    break;
	case 'e':
	    command = "enable";
	    requires_pname = true;
	    break;
	case 'd':
	    command = "disable";
	    requires_pname = true;
	    break;
	case 'c':
	    command = "clear";
	    requires_pname = true;
	    break;
	case 'g':
	    command = "get";
	    requires_pname = true;
	    break;
	default:
	    return usage(argv[0]);
	}
    }
    
    if (target.empty())
	return usage(argv[0]);

    if (command.empty())
	return usage(argv[0]);
    
    if (requires_pname && pname.empty())
	return usage(argv[0]);

    try {
	XrlStdRouter xrl_router( "profiler");
	XrlProfilerTarget profiler(&xrl_router);

	xrl_router.finalize();
	wait_until_xrl_router_is_ready( xrl_router);

	if (command == "list")
	    profiler.list(target);
	else if (command == "enable")
	    profiler.enable(target, pname);
	else if (command == "disable")
	    profiler.disable(target, pname);
	else if (command == "clear")
	    profiler.clear(target, pname);
	else if (command == "get")
	    profiler.get(target, pname);
	else
	    XLOG_FATAL("Unknown command");

	while (!profiler.done())
	    EventLoop::instance().run();

	while (xrl_router.pending())
 	    EventLoop::instance().run();

        } catch (...) {
	xorp_catch_standard_exceptions();
    }

    // 
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
