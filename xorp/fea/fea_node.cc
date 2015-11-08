// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2011 XORP, Inc and Others
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




//
// FEA (Forwarding Engine Abstraction) node implementation.
//

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libcomm/comm_api.h"

#ifdef XORP_USE_CLICK
#include "fea/data_plane/managers/fea_data_plane_manager_click.hh"
#endif
#ifdef XORP_USE_FEA_DUMMY
#include "fea/data_plane/managers/fea_data_plane_manager_dummy.hh"
#endif
#include "fea/data_plane/managers/fea_data_plane_manager_linux.hh"

#include "fea_io.hh"
#include "fea_node.hh"
#ifndef XORP_DISABLE_PROFILE
#include "profile_vars.hh"
#endif

FeaNode::FeaNode(EventLoop& eventloop, FeaIo& fea_io, bool is_dummy)
    : _eventloop(eventloop),
      _is_running(false),
      _is_dummy(is_dummy),
      _ifconfig(*this),
#ifndef XORP_DISABLE_FIREWALL
      _firewall_manager(*this, ifconfig().merged_config()),
#endif
      _fibconfig(*this, _ifconfig.system_config(), _ifconfig.merged_config()),
      _io_link_manager(*this, ifconfig().merged_config()),
      _io_ip_manager(*this, ifconfig().merged_config()),
      _io_tcpudp_manager(*this, ifconfig().merged_config()),
      _fea_io(fea_io)
{
}

FeaNode::~FeaNode()
{
    shutdown();
}

int
FeaNode::startup()
{
    string error_msg;

    _is_running = false;

    comm_init();

#ifndef XORP_DISABLE_PROFILE
    initialize_profiling_variables(_profile);
#endif

    if (load_data_plane_managers(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot load the data plane manager(s): %s",
		   error_msg.c_str());
    }

    //
    // Startup managers
    //
    if (_ifconfig.start(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot start IfConfig: %s", error_msg.c_str());
    }
    if (_fibconfig.start(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot start FibConfig: %s", error_msg.c_str());
    }

    _is_running = true;

    return (XORP_OK);
}

int
FeaNode::shutdown()
{
    string error_msg;

    //
    // Gracefully stop the FEA
    //
    // TODO: this may not work if we depend on reading asynchronously
    // data from sockets. To fix this, we need to run the eventloop
    // until we get all the data we need. Tricky...
    //
    if (_fibconfig.stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop FibConfig: %s", error_msg.c_str());
    }
#ifndef XORP_DISABLE_FIREWALL
    if (_firewall_manager.stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop FirewallManager: %s", error_msg.c_str());
    }
#endif
    if (_ifconfig.stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop IfConfig: %s", error_msg.c_str());
    }

    if (unload_data_plane_managers(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot unload the data plane manager(s): %s",
		   error_msg.c_str());
    }

    comm_exit();

    _is_running = false;

    return (XORP_OK);
}

bool
FeaNode::is_running() const
{
    return (_is_running);
}

bool
FeaNode::have_ipv4() const
{
    if (_fea_data_plane_managers.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first data plane manager.
    // In the future we need to rething this and be more flexible.
    //
    return (_fea_data_plane_managers.front()->have_ipv4());
}

bool
FeaNode::have_ipv6() const
{
    if (_fea_data_plane_managers.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first data plane manager.
    // In the future we need to rething this and be more flexible.
    //
    return (_fea_data_plane_managers.front()->have_ipv6());
}

int
FeaNode::register_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager,
				     bool is_exclusive)
{
    string dummy_error_msg;

    if (is_exclusive) {
	// Unload and delete the previous data plane managers
	unload_data_plane_managers(dummy_error_msg);
    }

    if ((fea_data_plane_manager != NULL)
	&& (find(_fea_data_plane_managers.begin(),
		 _fea_data_plane_managers.end(),
		 fea_data_plane_manager)
	    == _fea_data_plane_managers.end())) {
	_fea_data_plane_managers.push_back(fea_data_plane_manager);
    }

    return (XORP_OK);
}

int
FeaNode::unregister_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager)
{
    string dummy_error_msg;

    if (fea_data_plane_manager == NULL)
	return (XORP_ERROR);

    list<FeaDataPlaneManager*>::iterator iter;
    iter = find(_fea_data_plane_managers.begin(),
		_fea_data_plane_managers.end(),
		fea_data_plane_manager);
    if (iter == _fea_data_plane_managers.end())
	return (XORP_ERROR);

    io_link_manager().unregister_data_plane_manager(fea_data_plane_manager);
    io_ip_manager().unregister_data_plane_manager(fea_data_plane_manager);
    io_tcpudp_manager().unregister_data_plane_manager(fea_data_plane_manager);

    fea_data_plane_manager->stop_manager(dummy_error_msg);
    _fea_data_plane_managers.erase(iter);
    delete fea_data_plane_manager;

    return (XORP_OK);
}

int
FeaNode::load_data_plane_managers(string& error_msg)
{
    string dummy_error_msg;

    FeaDataPlaneManager* fea_data_plane_manager = NULL;

    unload_data_plane_managers(dummy_error_msg);

    if (is_dummy()) {
#ifdef XORP_USE_FEA_DUMMY
	fea_data_plane_manager = new FeaDataPlaneManagerDummy(*this);
#endif
    } else {
	fea_data_plane_manager = new FeaDataPlaneManagerLinux(*this);
    }

    if (register_data_plane_manager(fea_data_plane_manager, true)
	!= XORP_OK) {
	error_msg = c_format("Failed to register the %s data plane manager",
			     fea_data_plane_manager->manager_name().c_str());
	delete fea_data_plane_manager;
	return (XORP_ERROR);
    }

    if (fea_data_plane_manager->start_manager(error_msg) != XORP_OK) {
	error_msg = c_format("Failed to start the %s data plane manager: %s",
			     fea_data_plane_manager->manager_name().c_str(),
			     error_msg.c_str());
	unload_data_plane_managers(dummy_error_msg);
	return (XORP_ERROR);
    }

    if (fea_data_plane_manager->register_plugins(error_msg) != XORP_OK) {
	error_msg = c_format("Failed to register the %s data plane "
			     "manager plugins: %s",
			     fea_data_plane_manager->manager_name().c_str(),
			     error_msg.c_str());
	unload_data_plane_managers(dummy_error_msg);
	return (XORP_ERROR);
    }

    if (io_link_manager().register_data_plane_manager(fea_data_plane_manager,
						      true)
	!= XORP_OK) {
	error_msg = c_format("Failed to register the %s data plane "
			     "manager with the I/O Link manager",
			     fea_data_plane_manager->manager_name().c_str());
	unload_data_plane_managers(dummy_error_msg);
	return (XORP_ERROR);
    }

    if (io_ip_manager().register_data_plane_manager(fea_data_plane_manager,
						    true)
	!= XORP_OK) {
	error_msg = c_format("Failed to register the %s data plane "
			     "manager with the I/O IP manager",
			     fea_data_plane_manager->manager_name().c_str());
	unload_data_plane_managers(dummy_error_msg);
	return (XORP_ERROR);
    }

    if (io_tcpudp_manager().register_data_plane_manager(fea_data_plane_manager,
							true)
	!= XORP_OK) {
	error_msg = c_format("Failed to register the %s data plane "
			     "manager with the I/O TCP/UDP manager",
			     fea_data_plane_manager->manager_name().c_str());
	unload_data_plane_managers(dummy_error_msg);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FeaNode::unload_data_plane_managers(string& error_msg)
{
    string dummy_error_msg;

    UNUSED(error_msg);

    //
    // Stop and delete the data plane managers
    //
    while (! _fea_data_plane_managers.empty()) {
	unregister_data_plane_manager(_fea_data_plane_managers.front());
    }

    return (XORP_OK);
}
