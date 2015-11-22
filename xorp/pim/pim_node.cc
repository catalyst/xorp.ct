// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
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
// Protocol Independent Multicast (both PIM-SM and PIM-DM)
// node implementation (common part).
// PIM-SMv2 (draft-ietf-pim-sm-new-*), PIM-DM (new draft pending).
//


#include "pim_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "fea/mfea_kernel_messages.hh"		// TODO: XXX: yuck!
#include "pim_mre.hh"
#include "pim_node.hh"
#include "pim_vif.hh"


const char* str(VarE v) {
    switch (v) {
    case PROTO_VERSION: return "PROTO_VERSION";
    case HELLO_TRIGGERED_DELAY: return "HELLO_TRIGGERED_DELAY";
    case HELLO_PERIOD: return "HELLO_PERIOD";
    case HELLO_HOLDTIME: return "HELLO_HOLDTIME";
    case DR_PRIORITY: return "DR_PRIORITY";
    case PROPAGATION_DELAY: return "PROPAGATION_DELAY";
    case OVERRIDE_INTERVAL: return "OVERRIDE_INTERVAL";
    case TRACKING_DISABLED: return "TRACKING_DISABLED";
    case ACCEPT_NOHELLO: return "ACCEPT_NOHELLO";
    case JOIN_PRUNE_PERIOD: return "JOIN_PRUNE_PERIOD";
    default:
	return "UNKNOWN/DEFAULT/BUG";
    }
}

/**
 * PimNode::PimNode:
 * @family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @module_id: The module ID (must be either %XORP_MODULE_PIMSM
 * or %XORP_MODULE_PIMDM).
 * TODO: XXX: XORP_MODULE_PIMDM is not implemented yet.
 * 
 * PIM node constructor.
 **/
PimNode::PimNode(int family, xorp_module_id module_id)
    : ProtoNode<PimVif>(family, module_id),
      _pim_mrt(this),
      _pim_mrib_table(*this),
      _rp_table(*this),
      _pim_scope_zone_table(*this),
      _pim_bsr(*this),
      _is_switch_to_spt_enabled(false),	// XXX: disabled by defailt
      _switch_to_spt_threshold_interval_sec(0),
      _switch_to_spt_threshold_bytes(0),
      _is_log_trace(false)
{
    // TODO: XXX: PIMDM not implemented yet
    XLOG_ASSERT(module_id == XORP_MODULE_PIMSM);
    XLOG_ASSERT((module_id == XORP_MODULE_PIMSM)
		|| (module_id == XORP_MODULE_PIMDM));
    if ((module_id != XORP_MODULE_PIMSM) && (module_id != XORP_MODULE_PIMDM)) {
	XLOG_FATAL("Invalid module ID = %d (must be 'XORP_MODULE_PIMSM' = %d "
		   "or 'XORP_MODULE_PIMDM' = %d",
		   module_id, XORP_MODULE_PIMSM, XORP_MODULE_PIMDM);
    }
    
    _pim_register_vif_index = Vif::VIF_INDEX_INVALID;
    
    _buffer_recv = BUFFER_MALLOC(BUF_SIZE_DEFAULT);

    //
    // Set the node status
    //
    ProtoNode<PimVif>::set_node_status(PROC_STARTUP);

    //
    // Set myself as an observer when the node status changes
    //
    set_observer(this);
}

void PimNode::destruct_me() {
    //
    // Unset myself as an observer when the node status changes
    //
    unset_observer(this);

    stop();

    //
    // XXX: Explicitly clear the PimBsr and RpTable now to avoid
    // cross-referencing of lists that may be deleted prematurely
    // at the end of the PimNode destructor.
    //
    _pim_bsr.clear();
    _rp_table.clear();

    //
    // XXX: explicitly clear the PimMrt table now, because PimMrt may utilize
    // some lists in the PimNode class (e.g., _processing_pim_nbr_list) that
    // may be deleted prematurely at the end of the PimNode destructor
    // (depending on the declaration ordering).
    //
    _pim_mrt.clear();
    
    ProtoNode<PimVif>::set_node_status(PROC_NULL);

    delete_all_vifs();
    
    if (_buffer_recv)
	BUFFER_FREE(_buffer_recv);
}

/**
 * PimNode::~PimNode:
 * @: 
 * 
 * PIM node destructor.
 * 
 **/
PimNode::~PimNode()
{
    destruct_me();
}

/**
 * PimNode::start:
 * @: 
 * 
 * Start the PIM protocol.
 * TODO: This function should not start the protocol operation on the
 * interfaces. The interfaces must be activated separately.
 * After the startup operations are completed,
 * PimNode::final_start() is called to complete the job.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
PimNode::start()
{
    if (! is_enabled())
	return (XORP_OK);

    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_STARTING)
	|| (ServiceBase::status() == SERVICE_RUNNING)) {
	return (XORP_OK);
    }
    if (ServiceBase::status() != SERVICE_READY) {
	return (XORP_ERROR);
    }

    if (ProtoNode<PimVif>::pending_start() != XORP_OK)
	return (XORP_ERROR);

    //
    // Register with the FEA and MFEA
    //
    fea_register_startup();
    mfea_register_startup();

    //
    // Register with the RIB
    //
    rib_register_startup();

    //
    // Set the node status
    //
    ProtoNode<PimVif>::set_node_status(PROC_STARTUP);

    //
    // Update the node status
    //
    update_status();

    return (XORP_OK);
}

/**
 * PimNode::final_start:
 * @: 
 * 
 * Complete the start-up of the PIM protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::final_start()
{

    if (ProtoNode<PimVif>::start() != XORP_OK) {
	ProtoNode<PimVif>::stop();
	return (XORP_ERROR);
    }

    // Start the pim_vifs
    start_all_vifs();
    
    // Start the BSR module
    if (_pim_bsr.start() != XORP_OK)
	return (XORP_ERROR);

    XLOG_INFO("Protocol started");

    return (XORP_OK);
}

/**
 * PimNode::stop:
 * @: 
 * 
 * Gracefully stop the PIM protocol.
 * XXX: The graceful stop will attempt to send Join/Prune, Assert, etc.
 * messages for all multicast routing entries to gracefully clean-up
 * state with neighbors.
 * XXX: After the multicast routing entries cleanup is completed,
 * PimNode::final_stop() is called to complete the job.
 * XXX: This function, unlike start(), will stop the protocol
 * operation on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::stop()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_SHUTDOWN)
	|| (ServiceBase::status() == SERVICE_SHUTTING_DOWN)
	|| (ServiceBase::status() == SERVICE_FAILED)) {
	return (XORP_OK);
    }
    if ((ServiceBase::status() != SERVICE_RUNNING)
	&& (ServiceBase::status() != SERVICE_STARTING)
	&& (ServiceBase::status() != SERVICE_PAUSING)
	&& (ServiceBase::status() != SERVICE_PAUSED)
	&& (ServiceBase::status() != SERVICE_RESUMING)) {
	return (XORP_ERROR);
    }

    if (ProtoNode<PimVif>::pending_stop() != XORP_OK)
	return (XORP_ERROR);

    //
    // Perform misc. PIM-specific stop operations
    //
    _pim_bsr.stop();
    
    // Stop the vifs
    stop_all_vifs();
    
    //
    // Set the node status
    //
    ProtoNode<PimVif>::set_node_status(PROC_SHUTDOWN);

    //
    // Update the node status
    //
    update_status();

    return (XORP_OK);
}

/**
 * PimNode::final_stop:
 * @: 
 * 
 * Completely stop the PIM protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::final_stop()
{
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);

    if (ProtoNode<PimVif>::stop() != XORP_OK)
	return (XORP_ERROR);

    XLOG_INFO("Protocol stopped");

    return (XORP_OK);
}

/**
 * Enable the node operation.
 * 
 * If an unit is not enabled, it cannot be start, or pending-start.
 */
void
PimNode::enable()
{
    ProtoUnit::enable();

    XLOG_INFO("Protocol enabled");
}

/**
 * Disable the node operation.
 * 
 * If an unit is disabled, it cannot be start or pending-start.
 * If the unit was runnning, it will be stop first.
 */
void
PimNode::disable()
{
    stop();
    ProtoUnit::disable();

    XLOG_INFO("Protocol disabled");
}

/**
 * Get the IP protocol number.
 *
 * @return the IP protocol number.
 */
uint8_t
PimNode::ip_protocol_number() const
{
    return (IPPROTO_PIM);
}

void
PimNode::status_change(ServiceBase*  service,
		       ServiceStatus old_status,
		       ServiceStatus new_status)
{
    if (service == this) {
	if ((old_status == SERVICE_STARTING)
	    && (new_status == SERVICE_RUNNING)) {
	    // The startup process has completed
	    if (final_start() != XORP_OK) {
		XLOG_ERROR("Cannot complete the startup process; "
			   "current state is %s",
			   ProtoNode<PimVif>::state_str().c_str());
		return;
	    }
	    ProtoNode<PimVif>::set_node_status(PROC_READY);
	    return;
	}

	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    // The shutdown process has completed
	    final_stop();
	    // Set the node status
	    ProtoNode<PimVif>::set_node_status(PROC_DONE);
	    return;
	}
	//
	// TODO: check if there was an error
	//
	return;
    }

    if (service == ifmgr_mirror_service_base()) {
	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    decr_shutdown_requests_n();		// XXX: for the ifmgr
	}
    }
}

void
PimNode::tree_complete()
{
    decr_startup_requests_n();			// XXX: for the ifmgr

    //
    // XXX: we use same actions when the tree is completed or updates are made
    //
    updates_made();
}

void
PimNode::updates_made()
{
    map<string, Vif>::iterator pim_vif_iter;
    string error_msg;

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();

    //
    // Add new vifs and update existing ones
    //
    IfMgrIfTree::IfMap::const_iterator ifmgr_iface_iter;
    for (ifmgr_iface_iter = _iftree.interfaces().begin();
	 ifmgr_iface_iter != _iftree.interfaces().end();
	 ++ifmgr_iface_iter) {
	const IfMgrIfAtom& ifmgr_iface = ifmgr_iface_iter->second;

	IfMgrIfAtom::VifMap::const_iterator ifmgr_vif_iter;
	for (ifmgr_vif_iter = ifmgr_iface.vifs().begin();
	     ifmgr_vif_iter != ifmgr_iface.vifs().end();
	     ++ifmgr_vif_iter) {
	    const IfMgrVifAtom& ifmgr_vif = ifmgr_vif_iter->second;
	    const string& ifmgr_vif_name = ifmgr_vif.name();
	    Vif* node_vif = NULL;
	
	    pim_vif_iter = configured_vifs().find(ifmgr_vif_name);
	    if (pim_vif_iter != configured_vifs().end()) {
		node_vif = &(pim_vif_iter->second);
	    }

	    //
	    // Add a new vif
	    //
	    if (node_vif == NULL) {
		uint32_t vif_index = ifmgr_vif.vif_index();
		XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
		if (add_config_vif(ifmgr_vif_name, vif_index, error_msg)
		    != XORP_OK) {
		    XLOG_ERROR("Cannot add vif %s to the set of configured "
			       "vifs: %s",
			       ifmgr_vif_name.c_str(), error_msg.c_str());
		    continue;
		}
		pim_vif_iter = configured_vifs().find(ifmgr_vif_name);
		XLOG_ASSERT(pim_vif_iter != configured_vifs().end());
		node_vif = &(pim_vif_iter->second);
		// FALLTHROUGH
	    }

	    //
	    // Update the pif_index
	    //
	    set_config_pif_index(ifmgr_vif_name,
				 ifmgr_vif.pif_index(),
				 error_msg);

	    if (node_vif->vif_index() != ifmgr_vif.vif_index()) {
		XLOG_INFO("Vif-index changed, node-vif: %s  ifmgr-vif: %s",
			  node_vif->str().c_str(), ifmgr_vif.toString().c_str());
		// It went real..fix up underlying logic.
		PimVif* fake_vif = vif_find_by_vif_index(ifmgr_vif.vif_index());
		if (fake_vif) {
		    // Move any fake with our real ifindex out of the way.
		    adjust_fake_vif(fake_vif, ifmgr_vif.vif_index());
		}

		ProtoNode<PimVif>::delete_vif((PimVif*)(node_vif));
		XLOG_INFO("Setting formerly fake node_vif: %s  to real ifindex: %i",
			  node_vif->name().c_str(), ifmgr_vif.vif_index());
		node_vif->set_vif_index(ifmgr_vif.vif_index());
		node_vif->set_is_fake(false);
		ProtoNode<PimVif>::add_vif((PimVif*)(node_vif));
	    }
	
	    //
	    // Update the vif flags
	    //
	    bool is_up = ifmgr_iface.enabled();
	    is_up &= ifmgr_vif.enabled();
	    set_config_vif_flags(ifmgr_vif_name,
				 ifmgr_vif.pim_register(),
				 ifmgr_vif.p2p_capable(),
				 ifmgr_vif.loopback(),
				 ifmgr_vif.multicast_capable(),
				 ifmgr_vif.broadcast_capable(),
				 is_up,
				 ifmgr_iface.mtu(),
				 error_msg);
	
	}
    }

    //
    // Add new vif addresses, update existing ones, and remove old addresses
    //
    for (ifmgr_iface_iter = _iftree.interfaces().begin();
	 ifmgr_iface_iter != _iftree.interfaces().end();
	 ++ifmgr_iface_iter) {
	const IfMgrIfAtom& ifmgr_iface = ifmgr_iface_iter->second;
	const string& ifmgr_iface_name = ifmgr_iface.name();
	IfMgrIfAtom::VifMap::const_iterator ifmgr_vif_iter;

	for (ifmgr_vif_iter = ifmgr_iface.vifs().begin();
	     ifmgr_vif_iter != ifmgr_iface.vifs().end();
	     ++ifmgr_vif_iter) {
	    const IfMgrVifAtom& ifmgr_vif = ifmgr_vif_iter->second;
	    const string& ifmgr_vif_name = ifmgr_vif.name();
	    Vif* node_vif = NULL;

	    //
	    // Add new vif addresses and update existing ones
	    //
	    pim_vif_iter = configured_vifs().find(ifmgr_vif_name);
	    if (pim_vif_iter != configured_vifs().end()) {
		node_vif = &(pim_vif_iter->second);
	    }

	    if (is_ipv4()) {
		IfMgrVifAtom::IPv4Map::const_iterator a4_iter;

		for (a4_iter = ifmgr_vif.ipv4addrs().begin();
		     a4_iter != ifmgr_vif.ipv4addrs().end();
		     ++a4_iter) {
		    const IfMgrIPv4Atom& a4 = a4_iter->second;
		    VifAddr* node_vif_addr = node_vif->find_address(IPvX(a4.addr()));
		    IPvX addr(a4.addr());
		    IPvXNet subnet_addr(addr, a4.prefix_len());
		    IPvX broadcast_addr(IPvX::ZERO(family()));
		    IPvX peer_addr(IPvX::ZERO(family()));
		    if (a4.has_broadcast())
			broadcast_addr = IPvX(a4.broadcast_addr());
		    if (a4.has_endpoint())
			peer_addr = IPvX(a4.endpoint_addr());

		    if (node_vif_addr == NULL) {
			if (add_config_vif_addr(
				ifmgr_vif_name,
				addr,
				subnet_addr,
				broadcast_addr,
				peer_addr,
				error_msg)
			    != XORP_OK) {
			    XLOG_ERROR("Cannot add address %s to vif %s from "
				       "the set of configured vifs: %s",
				       cstring(addr), ifmgr_vif_name.c_str(),
				       error_msg.c_str());
			}
			continue;
		    }
		    if ((addr == node_vif_addr->addr())
			&& (subnet_addr == node_vif_addr->subnet_addr())
			&& (broadcast_addr == node_vif_addr->broadcast_addr())
			&& (peer_addr == node_vif_addr->peer_addr())) {
			continue;	// Nothing changed
		    }

		    // Update the address
		    if (delete_config_vif_addr(ifmgr_vif_name,
					       addr,
					       error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot delete address %s from vif %s "
				   "from the set of configured vifs: %s",
				   cstring(addr),
				   ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		    if (add_config_vif_addr(
			    ifmgr_vif_name,
			    addr,
			    subnet_addr,
			    broadcast_addr,
			    peer_addr,
			    error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot add address %s to vif %s from "
				   "the set of configured vifs: %s",
				   cstring(addr), ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		}
	    }

	    if (is_ipv6()) {
		IfMgrVifAtom::IPv6Map::const_iterator a6_iter;

		for (a6_iter = ifmgr_vif.ipv6addrs().begin();
		     a6_iter != ifmgr_vif.ipv6addrs().end();
		     ++a6_iter) {
		    const IfMgrIPv6Atom& a6 = a6_iter->second;
		    VifAddr* node_vif_addr = node_vif->find_address(IPvX(a6.addr()));
		    IPvX addr(a6.addr());
		    IPvXNet subnet_addr(addr, a6.prefix_len());
		    IPvX broadcast_addr(IPvX::ZERO(family()));
		    IPvX peer_addr(IPvX::ZERO(family()));
		    if (a6.has_endpoint())
			peer_addr = IPvX(a6.endpoint_addr());

		    if (node_vif_addr == NULL) {
			if (add_config_vif_addr(
				ifmgr_vif_name,
				addr,
				subnet_addr,
				broadcast_addr,
				peer_addr,
				error_msg)
			    != XORP_OK) {
			    XLOG_ERROR("Cannot add address %s to vif %s from "
				       "the set of configured vifs: %s",
				       cstring(addr), ifmgr_vif_name.c_str(),
				       error_msg.c_str());
			}
			continue;
		    }
		    if ((addr == node_vif_addr->addr())
			&& (subnet_addr == node_vif_addr->subnet_addr())
			&& (peer_addr == node_vif_addr->peer_addr())) {
			continue;	// Nothing changed
		    }

		    // Update the address
		    if (delete_config_vif_addr(ifmgr_vif_name,
					       addr,
					       error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot delete address %s from vif %s "
				   "from the set of configured vifs: %s",
				   cstring(addr),
				   ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		    if (add_config_vif_addr(
			    ifmgr_vif_name,
			    addr,
			    subnet_addr,
			    broadcast_addr,
			    peer_addr,
			    error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot add address %s to vif %s from "
				   "the set of configured vifs: %s",
				   cstring(addr), ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		}
	    }

	    //
	    // Delete vif addresses that don't exist anymore
	    //
	    {
		list<IPvX> delete_addresses_list;
		list<VifAddr>::const_iterator vif_addr_iter;
		for (vif_addr_iter = node_vif->addr_list().begin();
		     vif_addr_iter != node_vif->addr_list().end();
		     ++vif_addr_iter) {
		    const VifAddr& vif_addr = *vif_addr_iter;
		    if (vif_addr.addr().is_ipv4()
			&& (_iftree.find_addr(ifmgr_iface_name,
					      ifmgr_vif_name,
					      vif_addr.addr().get_ipv4()))
			    == NULL) {
			    delete_addresses_list.push_back(vif_addr.addr());
		    }
		    if (vif_addr.addr().is_ipv6()
			&& (_iftree.find_addr(ifmgr_iface_name,
					      ifmgr_vif_name,
					      vif_addr.addr().get_ipv6()))
			    == NULL) {
			    delete_addresses_list.push_back(vif_addr.addr());
		    }
		}

		// Delete the addresses
		list<IPvX>::iterator ipvx_iter;
		for (ipvx_iter = delete_addresses_list.begin();
		     ipvx_iter != delete_addresses_list.end();
		     ++ipvx_iter) {
		    const IPvX& ipvx = *ipvx_iter;
		    if (delete_config_vif_addr(ifmgr_vif_name, ipvx, error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot delete address %s from vif %s from "
				   "the set of configured vifs: %s",
				   cstring(ipvx), ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		}
	    }
	}
    }

    //
    // Remove vifs that don't exist anymore
    //
    list<string> delete_vifs_list;
    for (pim_vif_iter = configured_vifs().begin();
	 pim_vif_iter != configured_vifs().end();
	 ++pim_vif_iter) {
	Vif* node_vif = &pim_vif_iter->second;
	if (_iftree.find_vif(node_vif->name(), node_vif->name()) == NULL) {
	    // Add the vif to the list of old interfaces
	    delete_vifs_list.push_back(node_vif->name());
	}
    }
    // Delete the old vifs
    list<string>::iterator vif_name_iter;
    for (vif_name_iter = delete_vifs_list.begin();
	 vif_name_iter != delete_vifs_list.end();
	 ++vif_name_iter) {
	const string& vif_name = *vif_name_iter;
	if (delete_config_vif(vif_name, error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot delete vif %s from the set of configured "
		       "vifs: %s",
		       vif_name.c_str(), error_msg.c_str());
	}
    }
    
    // Done
    set_config_all_vifs_done(error_msg);
}

/**
 * PimNode::add_vif:
 * @vif: Information about the new PimVif to install.
 * @error_msg: The error message (if error).
 * 
 * Install a new PIM vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::add_vif(const Vif& vif, string& error_msg)
{
    //
    // Create a new PimVif
    //
    PimVif *pim_vif = new PimVif(this, vif);
    
    if (ProtoNode<PimVif>::add_vif(pim_vif) != XORP_OK) {
	// Cannot add this new vif
	error_msg = c_format("Cannot add vif %s: internal error",
			     vif.name().c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	
	delete pim_vif;
	return XORP_ERROR;
    }
    
    // Set the PIM Register vif index if needed
    if (pim_vif->is_pim_register())
	_pim_register_vif_index = pim_vif->vif_index();

    //
    // Resolve all destination prefixes whose next-hop vif name was not
    // resolved earlier (e.g., the vif was unknown).
    //
    _pim_mrib_table.resolve_prefixes_by_vif_name(pim_vif->name(),
						 pim_vif->vif_index());

    //
    // Update and check the primary and domain-wide addresses
    //
    do {
	if (pim_vif->update_primary_and_domain_wide_address(error_msg)
	    == XORP_OK) {
	    break;
	}
	if (pim_vif->addr_ptr() == NULL) {
	    // XXX: don't print an error if the vif has no addresses
	    break;
	}
	if (pim_vif->is_loopback()) {
	    // XXX: don't print an error if this is a loopback interface
	    break;
	}
	XLOG_ERROR("Error updating primary and domain-wide addresses "
		   "for vif %s: %s",
		   pim_vif->name().c_str(), error_msg.c_str());
	return XORP_ERROR;
    } while (false);

    XLOG_INFO("Interface added: %s", pim_vif->str().c_str());
    
    return XORP_OK;
}

/**
 * PimNode::add_vif:
 * @vif_name: The name of the new vif.
 * @vif_index: The vif index of the new vif.
 * @error_msg: The error message (if error).
 * 
 * Install a new PIM vif. If the vif exists, nothing is installed.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::add_vif(const string& vif_name, uint32_t vif_index, string& error_msg)
{
    PimVif *mvif;
    bool fake = false;

    if (vif_index <= 0) {
	// Well now...this interface must not currently exist.
	// We need to fake out an index
	vif_index = 1;

	while (true) {
	    mvif = vif_find_by_vif_index(vif_index);
	    if (mvif == NULL)
		break;
	    vif_index++;
	}
	fake = true;
    }

    mvif = vif_find_by_vif_index(vif_index);
    if (mvif) {
	if (mvif->name() == vif_name) {
	    return XORP_OK; // already have this vif
	}

	if (mvif->is_fake()) {
	    adjust_fake_vif(mvif, vif_index);
	}
	else {
	    // This is bad..we have vif_index clash.
	    error_msg = c_format("Cannot add vif %s: internal error, vif_index: %i",
				 vif_name.c_str(), vif_index);
	    XLOG_ERROR("%s", error_msg.c_str());
	    return XORP_ERROR;
	}
    }

    //
    // Create a new Vif
    //
    Vif vif(vif_name);
    vif.set_vif_index(vif_index);
    vif.set_is_fake(fake);
    return add_vif(vif, error_msg);
}

/**
 * PimNode::delete_vif:
 * @vif_name: The name of the vif to delete.
 * @error_msg: The error message (if error).
 * 
 * Delete an existing PIM vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::delete_vif(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot delete vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (ProtoNode<PimVif>::delete_vif(pim_vif) != XORP_OK) {
	error_msg = c_format("Cannot delete vif %s: internal error",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	delete pim_vif;
	return (XORP_ERROR);
    }
    
    // Reset the PIM Register vif index if needed
    if (_pim_register_vif_index == pim_vif->vif_index())
	_pim_register_vif_index = Vif::VIF_INDEX_INVALID;
    
    delete pim_vif;
    
    XLOG_INFO("Interface deleted: %s", vif_name.c_str());
    
    return (XORP_OK);
}

int
PimNode::set_vif_flags(const string& vif_name,
		       bool is_pim_register, bool is_p2p,
		       bool is_loopback, bool is_multicast,
		       bool is_broadcast, bool is_up, uint32_t mtu,
		       string& error_msg)
{
    bool is_changed = false;
    
    PimVif *pim_vif = find_or_create_vif(vif_name, error_msg);
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot set flags vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (pim_vif->is_pim_register() != is_pim_register) {
	pim_vif->set_pim_register(is_pim_register);
	is_changed = true;
    }
    if (pim_vif->is_p2p() != is_p2p) {
	pim_vif->set_p2p(is_p2p);
	is_changed = true;
    }
    if (pim_vif->is_loopback() != is_loopback) {
	pim_vif->set_loopback(is_loopback);
	is_changed = true;
    }
    if (pim_vif->is_multicast_capable() != is_multicast) {
	pim_vif->set_multicast_capable(is_multicast);
	is_changed = true;
    }
    if (pim_vif->is_broadcast_capable() != is_broadcast) {
	pim_vif->set_broadcast_capable(is_broadcast);
	is_changed = true;
    }
    if (pim_vif->is_underlying_vif_up() != is_up) {
	pim_vif->set_underlying_vif_up(is_up);
	is_changed = true;
    }
    if (pim_vif->mtu() != mtu) {
	pim_vif->set_mtu(mtu);
	is_changed = true;
    }
    
    // Set the PIM Register vif index if needed
    if (pim_vif->is_pim_register())
	_pim_register_vif_index = pim_vif->vif_index();
    
    if (is_changed) {
	XLOG_INFO("Interface flags changed: %s", pim_vif->str().c_str());
	pim_vif->notifyUpdated();
    }
    
    return (XORP_OK);
}

int
PimNode::add_vif_addr(const string& vif_name,
		      const IPvX& addr,
		      const IPvXNet& subnet_addr,
		      const IPvX& broadcast_addr,
		      const IPvX& peer_addr,
		      bool& should_send_hello,
		      string &error_msg)
{
    PimVif *pim_vif = find_or_create_vif(vif_name, error_msg);

    should_send_hello = false;

    if (pim_vif == NULL) {
	error_msg = c_format("Cannot add address on vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    const VifAddr vif_addr(addr, subnet_addr, broadcast_addr, peer_addr);
    
    //
    // Check the arguments
    //
    if (! addr.is_unicast()) {
	error_msg = c_format("Cannot add address on vif %s: "
			     "invalid unicast address: %s",
			     vif_name.c_str(), addr.str().c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if ((addr.af() != family())
	|| (subnet_addr.af() != family())
	|| (broadcast_addr.af() != family())
	|| (peer_addr.af() != family())) {
	error_msg = c_format("Cannot add address on vif %s: "
			     "invalid address family: %s ",
			     vif_name.c_str(), vif_addr.str().c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    VifAddr* node_vif_addr = pim_vif->find_address(addr);

    if ((node_vif_addr != NULL) && (*node_vif_addr == vif_addr))
	return (XORP_OK);		// Already have this address

    //
    // Spec:
    // "Before an interface goes down or changes primary IP address, a Hello
    // message with a zero HoldTime should be sent immediately (with the old IP
    // address if the IP address changed)."
    //
    // However, by adding or updating an existing address we cannot
    // change a valid primary address, hence we do nothing here.
    //

    if (node_vif_addr != NULL) {
	// Update the address
	XLOG_INFO("Updated existing address on interface %s: "
		  "old is %s new is %s",
		  pim_vif->name().c_str(), node_vif_addr->str().c_str(),
		  vif_addr.str().c_str());
	*node_vif_addr = vif_addr;
    } else {
	// Add a new address
	pim_vif->add_address(vif_addr);
	
	XLOG_INFO("Added new address to interface %s: %s",
		  pim_vif->name().c_str(), vif_addr.str().c_str());
    }

    //
    // Update and check the primary and domain-wide addresses
    //
    do {
	if (pim_vif->update_primary_and_domain_wide_address(error_msg)
	    == XORP_OK) {
	    break;
	}
	if (! (pim_vif->is_up() || pim_vif->is_pending_up())) {
	    // XXX: print an error only if the interface is UP or PENDING_UP
	    break;
	}
	if (pim_vif->is_loopback()) {
	    // XXX: don't print an error if this is a loopback interface
	    break;
	}
	XLOG_ERROR("Error updating primary and domain-wide addresses "
		   "for vif %s: %s",
		   pim_vif->name().c_str(), error_msg.c_str());
	return (XORP_ERROR);
    } while (false);

    //
    // Spec:
    // "If an interface changes one of its secondary IP addresses,
    // a Hello message with an updated Address_List option and a
    // non-zero HoldTime should be sent immediately."
    //
    if (pim_vif->is_up()) {
	// pim_vif->pim_hello_send();
	should_send_hello = true;
    }

    // Schedule the dependency-tracking tasks
    pim_mrt().add_task_my_ip_address(pim_vif->vif_index());
    pim_mrt().add_task_my_ip_subnet_address(pim_vif->vif_index());

    //
    // Inform the BSR about the change
    //
    pim_bsr().add_vif_addr(pim_vif->vif_index(), addr);

    // Let the VIF know it was updated..might want to start itself
    // if it was waiting on an IP address.
    pim_vif->notifyUpdated();
    
    return (XORP_OK);
}

int
PimNode::delete_vif_addr(const string& vif_name,
			 const IPvX& addr,
			 bool& should_send_hello,
			 string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);

    should_send_hello = false;

    if (pim_vif == NULL) {
	error_msg = c_format("Cannot delete address on vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    const VifAddr *tmp_vif_addr = pim_vif->find_address(addr);
    if (tmp_vif_addr == NULL) {
	error_msg = c_format("Cannot delete address on vif %s: "
			     "invalid address %s",
			     vif_name.c_str(), addr.str().c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    VifAddr vif_addr = *tmp_vif_addr;	// Get a copy

    //
    // Get the vif's old primary address and whether the vif is UP
    //
    bool old_vif_is_up = pim_vif->is_up() || pim_vif->is_pending_up();
    IPvX old_primary_addr = pim_vif->primary_addr();

    //
    // Spec:
    // "Before an interface goes down or changes primary IP address, a Hello
    // message with a zero HoldTime should be sent immediately (with the old IP
    // address if the IP address changed)."
    //
    if (pim_vif->is_up()) {
	if (pim_vif->primary_addr() == addr) {
	    pim_vif->pim_hello_stop();
	}
    }

    //
    // If an interface's primary address is deleted, first stop the vif.
    //
    if (old_vif_is_up) {
	if (pim_vif->primary_addr() == addr) {
	    string dummy_error_msg;
	    pim_vif->stop(dummy_error_msg, false, "primary addr deleted");
	}
    }

    if (pim_vif->delete_address(addr) != XORP_OK) {
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    XLOG_INFO("Deleted address on interface %s: %s",
	      pim_vif->name().c_str(), vif_addr.str().c_str());

    //
    // Update and check the primary and domain-wide addresses.
    // If the vif has no primary or a domain-wide address, then stop it.
    // If the vif's primary address was changed, then restart the vif.
    //
    do {
	string dummy_error_msg;

	if (pim_vif->update_primary_and_domain_wide_address(error_msg)
	    != XORP_OK) {
	    XLOG_ERROR("Error updating primary and domain-wide addresses "
		       "for vif %s: %s",
		       pim_vif->name().c_str(), error_msg.c_str());
	}
	if (pim_vif->primary_addr().is_zero()
	    || pim_vif->domain_wide_addr().is_zero()) {
	    pim_vif->stop(dummy_error_msg, false, "del-addr, new addr is zero");
	    break;
	}
	if (old_primary_addr == pim_vif->primary_addr())
	    break; // Nothing changed

	// Conditionally restart the interface
	pim_vif->stop(dummy_error_msg, false, "del-addr: stop before possible restart");
	if (old_vif_is_up)
	    pim_vif->start(dummy_error_msg, " restart after del-addr");
	break;
    } while (false);

    //
    // Spec:
    // "If an interface changes one of its secondary IP addresses,
    // a Hello message with an updated Address_List option and a
    // non-zero HoldTime should be sent immediately."
    //
    if (pim_vif->is_up()) {
	// pim_vif->pim_hello_send();
	should_send_hello = true;
    }
    
    // Schedule the dependency-tracking tasks
    pim_mrt().add_task_my_ip_address(pim_vif->vif_index());
    pim_mrt().add_task_my_ip_subnet_address(pim_vif->vif_index());
    
    //
    // Inform the BSR about the change
    //
    pim_bsr().delete_vif_addr(pim_vif->vif_index(), addr);

    return (XORP_OK);
}

/**
 * PimNode::enable_vif:
 * @vif_name: The name of the vif to enable.
 * @error_msg: The error message (if error).
 * 
 * Enable an existing PIM vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::enable_vif(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = find_or_create_vif(vif_name, error_msg);

    if (pim_vif) {
	pim_vif->enable("PimNode::enable_vif");

	// Check our wants-to-be-running list
	map<string, PVifPermInfo>::iterator i = perm_info.find(vif_name);
	if (i != perm_info.end()) {
	    i->second.should_enable = true;
	}
	else {
	    PVifPermInfo pv(vif_name, false, true);
	    perm_info[vif_name] = pv;
	}

	return XORP_OK;
    }
    else {
	return XORP_ERROR;
    }
}

PimVif*
PimNode::find_or_create_vif(const string& vif_name, string& error_msg) {
    PimVif* mvif = vif_find_by_name(vif_name);

    if (mvif == NULL) {
	add_vif(vif_name, 0, error_msg);
	mvif = vif_find_by_name(vif_name);
    }
    return mvif;
}

/**
 * PimNode::disable_vif:
 * @vif_name: The name of the vif to disable.
 * @error_msg: The error message (if error).
 * 
 * Disable an existing PIM vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::disable_vif(const string& vif_name, string& error_msg)
{

    // Check our wants-to-be-running list
    map<string, PVifPermInfo>::iterator i = perm_info.find(vif_name);
    if (i != perm_info.end()) {
	i->second.should_enable = false;
    }

    PimVif *pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot disable vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	// It's as disabled as it's going to get..don't fail the commit.
	error_msg = "";
	return XORP_OK;
    }
    
    pim_vif->disable("PimNode::disable_vif");
    
    return (XORP_OK);
}

/**
 * PimNode::start_vif:
 * @vif_name: The name of the vif to start.
 * @error_msg: The error message (if error).
 * 
 * Start an existing PIM vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::start_vif(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = find_or_create_vif(vif_name, error_msg);
    if (pim_vif == NULL) {
	error_msg.append(c_format("Cannot start vif %s: cannot find or create vif",
				  vif_name.c_str()));
	XLOG_ERROR("%s", error_msg.c_str());
	return XORP_ERROR;
    }

    if (pim_vif->start(error_msg, "PimNode::start_vif") != XORP_OK) {
	error_msg = c_format("Cannot start vif %s: %s",
			     vif_name.c_str(), error_msg.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return XORP_ERROR;
    }
    
    return XORP_OK;
}

/**
 * PimNode::stop_vif:
 * @vif_name: The name of the vif to stop.
 * @error_msg: The error message (if error).
 * 
 * Stop an existing PIM vif.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::stop_vif(const string& vif_name, string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot stop vif %s: no such vif (will continue)",
			     vif_name.c_str());
	XLOG_INFO("%s", error_msg.c_str());
	return XORP_OK;
    }
    
    if (pim_vif->stop(error_msg, true, "PimNode::stop_vif") != XORP_OK) {
	error_msg = c_format("Cannot stop vif %s: %s",
			     vif_name.c_str(), error_msg.c_str());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * PimNode::start_all_vifs:
 * @: 
 * 
 * Start PIM on all enabled interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::start_all_vifs()
{
    vector<PimVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	if (start_vif(pim_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * PimNode::stop_all_vifs:
 * @: 
 * 
 * Stop PIM on all interfaces it was running on.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::stop_all_vifs()
{
    vector<PimVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	if (stop_vif(pim_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * PimNode::enable_all_vifs:
 * @: 
 * 
 * Enable PIM on all interfaces.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::enable_all_vifs()
{
    vector<PimVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	if (enable_vif(pim_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * PimNode::disable_all_vifs:
 * @: 
 * 
 * Disable PIM on all interfaces. All running interfaces are stopped first.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::disable_all_vifs()
{
    vector<PimVif *>::iterator iter;
    string error_msg;
    int ret_value = XORP_OK;
    
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif == NULL)
	    continue;
	if (disable_vif(pim_vif->name(), error_msg) != XORP_OK)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

/**
 * PimNode::delete_all_vifs:
 * @: 
 * 
 * Delete all PIM vifs.
 **/
void
PimNode::delete_all_vifs()
{
    XLOG_INFO("pim-node: %p  start of delete-all vifs, size: %i\n",
	      this, (int)(proto_vifs().size()));

    list<string> vif_names;
    vector<PimVif *>::iterator iter;

    //
    // Create the list of all vif names to delete
    //
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = (*iter);
	if (pim_vif != NULL) {
	    string vif_name = pim_vif->name();
	    vif_names.push_back(pim_vif->name());
	}
    }

    //
    // Delete all vifs
    //
    list<string>::iterator vif_names_iter;
    for (vif_names_iter = vif_names.begin();
	 vif_names_iter != vif_names.end();
	 ++vif_names_iter) {
	const string& vif_name = *vif_names_iter;
	string error_msg;
	if (delete_vif(vif_name, error_msg) != XORP_OK) {
	    error_msg = c_format("Cannot delete vif %s: internal error",
				 vif_name.c_str());
	    XLOG_ERROR("%s", error_msg.c_str());
	}
    }

    XLOG_INFO("pim-node: %p  end of delete-all vifs, size: %i\n",
	      this, (int)(proto_vifs().size()));
}

/**
 * A method called when a vif has completed its shutdown.
 * 
 * @param vif_name the name of the vif that has completed its shutdown.
 */
void
PimNode::vif_shutdown_completed(const string& vif_name)
{
    vector<PimVif *>::iterator iter;

    //
    // If all vifs have completed the shutdown, then de-register with
    // the RIB and the MFEA.
    //
    for (iter = proto_vifs().begin(); iter != proto_vifs().end(); ++iter) {
	PimVif *pim_vif = *iter;
	if (pim_vif == NULL)
	    continue;
	if (! pim_vif->is_down())
	    return;
    }

    if (ServiceBase::status() == SERVICE_SHUTTING_DOWN) {
	//
	// De-register with the RIB
	//
	rib_register_shutdown();

	//
	// De-register with the FEA and MFEA
	//
	mfea_register_shutdown();
	fea_register_shutdown();
    }

    UNUSED(vif_name);
}

int
PimNode::proto_recv(const string& if_name,
		    const string& vif_name,
		    const IPvX& src_address,
		    const IPvX& dst_address,
		    uint8_t ip_protocol,
		    int32_t ip_ttl,
		    int32_t ip_tos,
		    bool ip_router_alert,
		    bool ip_internet_control,
		    const vector<uint8_t>& payload,
		    string& error_msg)
{
    PimVif *pim_vif = NULL;
    int ret_value = XORP_ERROR;

    debug_msg("Received message on %s/%s from %s to %s: "
	      "ip_ttl = %d ip_tos = %#x ip_router_alert = %d "
	      "ip_internet_control = %d rcvlen = %u\n",
	      if_name.c_str(), vif_name.c_str(),
	      cstring(src_address), cstring(dst_address),
	      ip_ttl, ip_tos, ip_router_alert, ip_internet_control,
	      XORP_UINT_CAST(payload.size()));

    UNUSED(if_name);
    UNUSED(ip_ttl);
    UNUSED(ip_tos);
    UNUSED(ip_router_alert);
    UNUSED(ip_internet_control);
    //
    // XXX: We registered to receive only one protocol, hence we ignore
    // the ip_protocol value.
    //
    UNUSED(ip_protocol);
 
    //
    // Check whether the node is up.
    //
    if (! is_up()) {
	error_msg = c_format("PIM node is not UP");
	return (XORP_ERROR);
    }
    
    //
    // Find the vif for that packet
    //
    pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot find vif with vif_name = %s",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    // Copy the data to the receiving #buffer_t
    BUFFER_RESET(_buffer_recv);
    BUFFER_PUT_DATA(&payload[0], _buffer_recv, payload.size());
    
    // Process the data by the vif
    ret_value = pim_vif->pim_recv(src_address, dst_address, _buffer_recv);
    
    return (ret_value);
    
 buflen_error:
    XLOG_UNREACHABLE();
    return (XORP_ERROR);
}

int
PimNode::pim_send(const string& if_name,
		  const string& vif_name,
		  const IPvX& src_address,
		  const IPvX& dst_address,
		  uint8_t ip_protocol,
		  int32_t ip_ttl,
		  int32_t ip_tos,
		  bool ip_router_alert,
		  bool ip_internet_control,
		  buffer_t *buffer,
		  string& error_msg)
{
    if (! (is_up() || is_pending_down())) {
	error_msg = c_format("PimNode::pim_send MLD/IGMP node is not UP");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    if (proto_send(if_name, vif_name, src_address, dst_address,
		   ip_protocol, ip_ttl, ip_tos, ip_router_alert,
		   ip_internet_control,
		   BUFFER_DATA_HEAD(buffer),
		   BUFFER_DATA_SIZE(buffer),
		   error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * PimNode::signal_message_recv:
 * @src_module_instance_name: The module instance name of the module-origin
 * of the message.
 * @message_type: The message type of the kernel signal.
 * At this moment, one of the following:
 * %MFEA_KERNEL_MESSAGE_NOCACHE (if a cache-miss in the kernel)
 * %MFEA_KERNEL_MESSAGE_WRONGVIF (multicast packet received on wrong vif)
 * %MFEA_KERNEL_MESSAGE_WHOLEPKT (typically, a packet that should be
 * encapsulated as a PIM-Register).
 * @vif_index: The vif index of the related interface (message-specific
 * relation).
 * @src: The source address in the message.
 * @dst: The destination address in the message.
 * @rcvbuf: The data buffer with the additional information in the message.
 * @rcvlen: The data length in @rcvbuf.
 * 
 * Receive a signal from the kernel (e.g., NOCACHE, WRONGVIF, WHOLEPKT).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::signal_message_recv(const string& src_module_instance_name,
			     int message_type,
			     uint32_t vif_index,
			     const IPvX& src,
			     const IPvX& dst,
			     const uint8_t *rcvbuf,
			     size_t rcvlen)
{
    int ret_value = XORP_ERROR;
    
    do {
	if (message_type == MFEA_KERNEL_MESSAGE_NOCACHE) {
	    ret_value = pim_mrt().signal_message_nocache_recv(
		src_module_instance_name,
		vif_index,
		src,
		dst);
	    break;
	}
	if (message_type == MFEA_KERNEL_MESSAGE_WRONGVIF) {
	    ret_value = pim_mrt().signal_message_wrongvif_recv(
		src_module_instance_name,
		vif_index,
		src,
		dst);
	    break;
	}
	if (message_type == MFEA_KERNEL_MESSAGE_WHOLEPKT) {
	    ret_value = pim_mrt().signal_message_wholepkt_recv(
		src_module_instance_name,
		vif_index,
		src,
		dst,
		rcvbuf,
		rcvlen);
	    break;
	}
	
	XLOG_WARNING("RX unknown signal from %s: "
		     "vif_index = %d src = %s dst = %s message_type = %d",
		     src_module_instance_name.c_str(),
		     vif_index,
		     cstring(src), cstring(dst),
		     message_type);
	return (XORP_ERROR);
    } while (false);
    
    return (ret_value);
}

/**
 * PimNode::add_membership:
 * @vif_index: The vif_index of the interface to add membership.
 * @source: The source address to add membership for
 * (IPvX::ZERO() for IGMPv1,2 and MLDv1).
 * @group: The group address to add membership for.
 * 
 * Add multicast membership on vif with vif_index of @vif_index for
 * source address of @source and group address of @group.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::add_membership(uint32_t vif_index, const IPvX& source,
			const IPvX& group)
{
    uint32_t lookup_flags = 0;
    uint32_t create_flags = 0;
    PimVif *pim_vif = NULL;
    bool is_ssm = false;

    if (source != IPvX::ZERO(family()))
	is_ssm = true;
    
    //
    // Check the arguments: the vif, source and group addresses.
    //
    pim_vif = vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL)
	return (XORP_ERROR);
    if (! (pim_vif->is_up()
	   || pim_vif->is_pending_up())) {
	return (XORP_ERROR);	// The vif is (going) DOWN
    }
    
    if (source != IPvX::ZERO(family())) {
	if (! source.is_unicast())
	    return (XORP_ERROR);
    }
    if (! group.is_multicast())
	return (XORP_ERROR);
    
    if (group.is_linklocal_multicast()
	|| group.is_interfacelocal_multicast()) {
	// XXX: don't route link or interface-local groups
	return (XORP_OK);
    }
    
    XLOG_TRACE(is_log_trace(), "Add membership for (%s, %s) on vif %s",
	       cstring(source), cstring(group), pim_vif->name().c_str());
    
    //
    // Setup the MRE lookup and create flags
    //
    if (is_ssm)
	lookup_flags |= PIM_MRE_SG;
    else
	lookup_flags |= PIM_MRE_WC;
    create_flags = lookup_flags;
    
    PimMre *pim_mre = pim_mrt().pim_mre_find(source, group, lookup_flags,
					     create_flags);
    
    if (pim_mre == NULL)
	return (XORP_ERROR);
    
    //
    // Modify to the local membership state
    //
    if (is_ssm) {
	//
	// (S, G) Join
	//
	// XXX: If the source was excluded, then don't exclude it anymore.
	// Otherwise, include the source.
	//
	XLOG_ASSERT(pim_mre->is_sg());
	if (pim_mre->local_receiver_exclude_sg().test(vif_index)) {
	    pim_mre->set_local_receiver_exclude(vif_index, false);
	} else {
	    pim_mre->set_local_receiver_include(vif_index, true);
	}
    } else {
	//
	// (*,G) Join
	//
	pim_mre->set_local_receiver_include(vif_index, true);
    }
    
    return (XORP_OK);
}

/**
 * PimNode::delete_membership:
 * @vif_index: The vif_index of the interface to delete membership.
 * @source: The source address to delete membership for
 * (IPvX::ZERO() for IGMPv1,2 and MLDv1).
 * @group: The group address to delete membership for.
 * 
 * Delete multicast membership on vif with vif_index of @vif_index for
 * source address of @source and group address of @group.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimNode::delete_membership(uint32_t vif_index, const IPvX& source,
			   const IPvX& group)
{
    uint32_t lookup_flags = 0;
    uint32_t create_flags = 0;
    PimVif *pim_vif = NULL;
    bool is_ssm = false;

    if (source != IPvX::ZERO(family()))
	is_ssm = true;
    
    //
    // Check the arguments: the vif, source and group addresses.
    //
    pim_vif = vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL)
	return (XORP_ERROR);
    if (! (pim_vif->is_up()
	   || pim_vif->is_pending_down()
	   || pim_vif->is_pending_up())) {
	return (XORP_ERROR);	// The vif is DOWN
    }
    
    if (source != IPvX::ZERO(family())) {
	if (! source.is_unicast())
	    return (XORP_ERROR);
    }
    if (! group.is_multicast())
	return (XORP_ERROR);
    
    if (group.is_linklocal_multicast()
	|| group.is_interfacelocal_multicast()) {
	// XXX: don't route link or interface-local groups
	return (XORP_OK);
    }
    
    XLOG_TRACE(is_log_trace(), "Delete membership for (%s, %s) on vif %s",
	       cstring(source), cstring(group), pim_vif->name().c_str());
    
    //
    // Setup the MRE lookup and create flags
    //
    if (is_ssm) {
	lookup_flags |= PIM_MRE_SG;
	create_flags = lookup_flags;	// XXX: create an entry for (S,G) Prune
    } else {
	lookup_flags |= PIM_MRE_WC;
	create_flags = 0;
    }
    
    PimMre *pim_mre = pim_mrt().pim_mre_find(source, group, lookup_flags,
					     create_flags);
    
    if (pim_mre == NULL)
	return (XORP_ERROR);
    
    //
    // Modify the local membership state
    //
    if (is_ssm) {
	//
	// (S, G) Prune
	//
	// XXX: If the source was included, then don't include it anymore.
	// Otherwise, exclude the source.
	//
	XLOG_ASSERT(pim_mre->is_sg());
	if (pim_mre->local_receiver_include_sg().test(vif_index)) {
	    pim_mre->set_local_receiver_include(vif_index, false);
	} else {
	    pim_mre->set_local_receiver_exclude(vif_index, true);
	}
    } else {
	//
	// (*,G) Prune
	//
	pim_mre->set_local_receiver_include(vif_index, false);
    }
    
    return (XORP_OK);
}

/**
 * PimNode::is_directly_connected:
 * @pim_vif: The virtual interface to test against.
 * @ipaddr_test: The address to test.
 * 
 * Note that the virtual interface the address is directly connected to
 * must be UP.
 * 
 * Return value: True if @ipaddr_test is directly connected to @pim_vif,
 * otherwise false.
 **/
bool
PimNode::is_directly_connected(const PimVif& pim_vif,
			       const IPvX& ipaddr_test) const
{
    if (! pim_vif.is_up())
	return (false);

    //
    // Test the alternative subnets
    //
    list<IPvXNet>::const_iterator iter;
    for (iter = pim_vif.alternative_subnet_list().begin();
	 iter != pim_vif.alternative_subnet_list().end();
	 ++iter) {
	const IPvXNet& ipvxnet = *iter;
	if (ipvxnet.contains(ipaddr_test))
	    return true;
    }

    //
    // Test the same subnet addresses, or the P2P addresses
    //
    return (pim_vif.is_same_subnet(ipaddr_test)
	    || pim_vif.is_same_p2p(ipaddr_test));
}

/**
 * PimNode::vif_find_pim_register:
 * @: 
 * 
 * Return the PIM Register virtual interface.
 * 
 * Return value: The PIM Register virtual interface if exists, otherwise NULL.
 **/
PimVif *
PimNode::vif_find_pim_register() const
{
    return (vif_find_by_vif_index(pim_register_vif_index()));
}

/**
 * PimNode::set_pim_vifs_dr:
 * @vif_index: The vif index to set/reset the DR flag.
 * @v: true if set the DR flag, otherwise false.
 * 
 * Set/reset the DR flag for vif index @vif_index.
 **/
void
PimNode::set_pim_vifs_dr(uint32_t vif_index, bool v)
{
    if (vif_index >= pim_vifs_dr().size())
	return;			// TODO: return an error instead?
    
    if (pim_vifs_dr().test(vif_index) == v)
	return;			// Nothing changed
    
    if (v)
	pim_vifs_dr().set(vif_index);
    else
	pim_vifs_dr().reset(vif_index);
    
    pim_mrt().add_task_i_am_dr(vif_index);
}

/**
 * PimNode::pim_vif_rpf_find:
 * @dst_addr: The address of the destination to search for.
 * 
 * Find the RPF virtual interface for a destination address.
 * 
 * Return value: The #PimVif entry for the RPF virtual interface to @dst_addr
 * if found, otherwise %NULL.
 **/
PimVif *
PimNode::pim_vif_rpf_find(const IPvX& dst_addr)
{
    Mrib *mrib;
    PimVif *pim_vif;

    //
    // Do the MRIB lookup
    //
    mrib = pim_mrib_table().find(dst_addr);
    if (mrib == NULL)
	return (NULL);

    //
    // Find the vif toward the destination address
    //
    pim_vif = vif_find_by_vif_index(mrib->next_hop_vif_index());

    return (pim_vif);
}

/**
 * PimNode::pim_nbr_rpf_find:
 * @dst_addr: The address of the destination to search for.
 * 
 * Find the RPF PIM neighbor for a destination address.
 * 
 * Return value: The #PimNbr entry for the RPF neighbor to @dst_addr if found,
 * otherwise %NULL.
 **/
PimNbr *
PimNode::pim_nbr_rpf_find(const IPvX& dst_addr)
{
    Mrib *mrib;
    
    //
    // Do the MRIB lookup
    //
    mrib = pim_mrib_table().find(dst_addr);
    
    //
    // Seach for the RPF neighbor router
    //
    return (pim_nbr_rpf_find(dst_addr, mrib));
}

/**
 * PimNode::pim_nbr_rpf_find:
 * @dst_addr: The address of the destination to search for.
 * @mrib: The MRIB information that was lookup already.
 * 
 * Find the RPF PIM neighbor for a destination address by using the
 * information in a pre-lookup MRIB.
 * 
 * Return value: The #PimNbr entry for the RPF neighbor to @dst_addr if found,
 * otherwise %NULL.
 **/
PimNbr *
PimNode::pim_nbr_rpf_find(const IPvX& dst_addr, const Mrib *mrib)
{
    bool is_same_subnet = false;
    PimNbr *pim_nbr = NULL;
    
    //
    // Check the MRIB information
    //
    if (mrib == NULL)
	return (NULL);
    
    //
    // Find the vif toward the destination address
    //
    PimVif *pim_vif = vif_find_by_vif_index(mrib->next_hop_vif_index());

    //
    // Test if the destination is on the same subnet.
    //
    // Note that we need to capture the case if the next-hop router
    // address toward a destination on the same subnet is set to one
    // of the addresses of the interface for that subnet.
    //
    do {
	if (mrib->next_hop_router_addr() == IPvX::ZERO(family())) {
	    is_same_subnet = true;
	    break;
	}
	if ((pim_vif != NULL)
	    && pim_vif->is_my_addr(mrib->next_hop_router_addr())) {
	    is_same_subnet = true;
	    break;
	}
	break;
    } while (false);

    //
    // Search for the neighbor router
    //
    if (is_same_subnet) {
	// A destination on the same subnet
	if (pim_vif != NULL) {
	    pim_nbr = pim_vif->pim_nbr_find(dst_addr);
	} else {
	    //
	    // TODO: XXX: try to remove all calls to pim_nbr_find_global().
	    // The reason we don't want to search for a neighbor across
	    // all network interfaces only by the neighbor's IP address is
	    // because in case of IPv6 the link-local addresses are unique
	    // only per subnet. In other words, there could be more than one
	    // neighbor routers with the same link-local address.
	    // To get rid of PimNode::pim_nbr_find_global(), we have to make
	    // sure that all valid MRIB entries have a valid next-hop vif
	    // index.
	    //
	    pim_nbr = pim_nbr_find_global(dst_addr);
	}
    } else {
	// Not a destination on the same subnet
	if (pim_vif != NULL)
	    pim_nbr = pim_vif->pim_nbr_find(mrib->next_hop_router_addr());
    }

    return (pim_nbr);
}

/**
 * PimNode::pim_nbr_find_global:
 * @nbr_addr: The address of the neighbor to search for.
 * 
 * Find a PIM neighbor by its address.
 * 
 * Note: this method should be used in very limited cases, because
 * in case of IPv6 a neighbor's IP address may not be unique within
 * the PIM neighbor database due to scope issues.
 * 
 * Return value: The #PimNbr entry for the neighbor if found, otherwise %NULL.
 **/
PimNbr *
PimNode::pim_nbr_find_global(const IPvX& nbr_addr)
{
    for (uint32_t i = 0; i < maxvifs(); i++) {
	PimVif *pim_vif = vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	// Exclude the PIM Register vif (as a safe-guard)
	if (pim_vif->is_pim_register())
	    continue;
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(nbr_addr);
	if (pim_nbr != NULL)
	    return (pim_nbr);
    }
    
    return (NULL);
}

//
// Add the PimMre to the dummy PimNbr with primary address of IPvX::ZERO()
//
void
PimNode::add_pim_mre_no_pim_nbr(PimMre *pim_mre)
{
    IPvX ipvx_zero(IPvX::ZERO(family()));
    PimNbr *pim_nbr = NULL;
    
    // Find the dummy PimNbr with primary address of IPvX::ZERO()
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	pim_nbr = *iter;
	if (pim_nbr->primary_addr() == ipvx_zero)
	    break;
	else
	    pim_nbr = NULL;
    }
    
    if (pim_nbr == NULL) {
	// Find the first vif. Note that the PIM Register vif is excluded.
	PimVif *pim_vif = NULL;
	for (uint32_t i = 0; i < maxvifs(); i++) {
	    pim_vif = vif_find_by_vif_index(i);
	    if (pim_vif == NULL)
		continue;
	    if (pim_vif->is_pim_register())
		continue;
	    break;
	}
	XLOG_ASSERT(pim_vif != NULL);
	pim_nbr = new PimNbr(pim_vif, ipvx_zero, PIM_VERSION_DEFAULT);
	processing_pim_nbr_list().push_back(pim_nbr);
    }
    XLOG_ASSERT(pim_nbr != NULL);
    
    pim_nbr->add_pim_mre(pim_mre);
}

//
// Delete the PimMre from the dummy PimNbr with primary address of IPvX::ZERO()
//
void
PimNode::delete_pim_mre_no_pim_nbr(PimMre *pim_mre)
{
    IPvX ipvx_zero(IPvX::ZERO(family()));
    PimNbr *pim_nbr = NULL;
    
    // Find the dummy PimNbr with primary address of IPvX::ZERO()
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	pim_nbr = *iter;
	if (pim_nbr->primary_addr() == ipvx_zero)
	    break;
	else
	    pim_nbr = NULL;
    }
    
    if (pim_nbr != NULL)
	pim_nbr->delete_pim_mre(pim_mre);
}

//
// Prepare all PimNbr entries with neighbor address of @pim_nbr_add to
// process their (*,*,RP) PimMre entries.
//
void
PimNode::init_processing_pim_mre_rp(uint32_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    do {
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		break;
	    PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	    if (pim_nbr == NULL)
		break;
	    pim_nbr->init_processing_pim_mre_rp();
	    return;
	}
    } while (false);
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->primary_addr() == pim_nbr_addr)
	    pim_nbr->init_processing_pim_mre_rp();
    }
}

//
// Prepare all PimNbr entries with neighbor address of @pim_nbr_add to
// process their (*,G) PimMre entries.
//
void
PimNode::init_processing_pim_mre_wc(uint32_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    do {
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		break;
	    PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	    if (pim_nbr == NULL)
		break;
	    pim_nbr->init_processing_pim_mre_wc();
	    return;
	}
    } while (false);
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->primary_addr() == pim_nbr_addr)
	    pim_nbr->init_processing_pim_mre_wc();
    }
}

//
// Prepare all PimNbr entries with neighbor address of @pim_nbr_add to
// process their (S,G) PimMre entries.
//
void
PimNode::init_processing_pim_mre_sg(uint32_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    do {
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		break;
	    PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	    if (pim_nbr == NULL)
		break;
	    pim_nbr->init_processing_pim_mre_sg();
	    return;
	}
    } while (false);
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->primary_addr() == pim_nbr_addr)
	    pim_nbr->init_processing_pim_mre_sg();
    }
}

//
// Prepare all PimNbr entries with neighbor address of @pim_nbr_add to
// process their (S,G,rpt) PimMre entries.
//
void
PimNode::init_processing_pim_mre_sg_rpt(uint32_t vif_index,
					const IPvX& pim_nbr_addr)
{
    do {
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		break;
	    PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	    if (pim_nbr == NULL)
		break;
	    pim_nbr->init_processing_pim_mre_sg_rpt();
	    return;
	}
    } while (false);
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->primary_addr() == pim_nbr_addr)
	    pim_nbr->init_processing_pim_mre_sg_rpt();
    }
}

PimNbr *
PimNode::find_processing_pim_mre_rp(uint32_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    if (vif_index != Vif::VIF_INDEX_INVALID) {
	PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    return (NULL);
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	if (pim_nbr == NULL)
	    return (NULL);
	if (pim_nbr->processing_pim_mre_rp_list().empty())
	    return (NULL);
	return (pim_nbr);
    }
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->primary_addr() != pim_nbr_addr)
	    continue;
	if (pim_nbr->processing_pim_mre_rp_list().empty())
	    continue;
	return (pim_nbr);
    }
    
    return (NULL);
}

PimNbr *
PimNode::find_processing_pim_mre_wc(uint32_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    if (vif_index != Vif::VIF_INDEX_INVALID) {
	PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    return (NULL);
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	if (pim_nbr == NULL)
	    return (NULL);
	if (pim_nbr->processing_pim_mre_wc_list().empty())
	    return (NULL);
	return (pim_nbr);
    }
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->primary_addr() != pim_nbr_addr)
	    continue;
	if (pim_nbr->processing_pim_mre_wc_list().empty())
	    continue;
	return (pim_nbr);
    }
    
    return (NULL);
}

PimNbr *
PimNode::find_processing_pim_mre_sg(uint32_t vif_index,
				    const IPvX& pim_nbr_addr)
{
    if (vif_index != Vif::VIF_INDEX_INVALID) {
	PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    return (NULL);
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	if (pim_nbr == NULL)
	    return (NULL);
	if (pim_nbr->processing_pim_mre_sg_list().empty())
	    return (NULL);
	return (pim_nbr);
    }
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->primary_addr() != pim_nbr_addr)
	    continue;
	if (pim_nbr->processing_pim_mre_sg_list().empty())
	    continue;
	return (pim_nbr);
    }
    
    return (NULL);
}

PimNbr *
PimNode::find_processing_pim_mre_sg_rpt(uint32_t vif_index,
					const IPvX& pim_nbr_addr)
{
    if (vif_index != Vif::VIF_INDEX_INVALID) {
	PimVif *pim_vif = vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    return (NULL);
	PimNbr *pim_nbr = pim_vif->pim_nbr_find(pim_nbr_addr);
	if (pim_nbr == NULL)
	    return (NULL);
	if (pim_nbr->processing_pim_mre_sg_rpt_list().empty())
	    return (NULL);
	return (pim_nbr);
    }
    
    list<PimNbr *>::iterator iter;
    for (iter = processing_pim_nbr_list().begin();
	 iter != processing_pim_nbr_list().end();
	 ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->primary_addr() != pim_nbr_addr)
	    continue;
	if (pim_nbr->processing_pim_mre_sg_rpt_list().empty())
	    continue;
	return (pim_nbr);
    }
    
    return (NULL);
}

//
// Statistics-related counters and values
//
void
PimNode::clear_pim_statistics()
{
    for (uint32_t i = 0; i < maxvifs(); i++) {
	PimVif *pim_vif = vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	pim_vif->clear_pim_statistics();
    }
}

int
PimNode::clear_pim_statistics_per_vif(const string& vif_name,
				      string& error_msg)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get statistics for vif %s: no such vif",
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
    pim_vif->clear_pim_statistics();
    
    return (XORP_OK);
}

#define GET_PIMSTAT_PER_NODE(stat_name)				\
uint32_t							\
PimNode::pimstat_##stat_name() const				\
{								\
    uint32_t sum = 0;						\
								\
    for (uint32_t i = 0; i < maxvifs(); i++) {			\
	PimVif *pim_vif = vif_find_by_vif_index(i);		\
	if (pim_vif == NULL)					\
	    continue;						\
	sum += pim_vif->pimstat_##stat_name();			\
    }								\
								\
    return (sum);						\
}

GET_PIMSTAT_PER_NODE(hello_messages_received)
GET_PIMSTAT_PER_NODE(hello_messages_sent)
GET_PIMSTAT_PER_NODE(hello_messages_rx_errors)
GET_PIMSTAT_PER_NODE(register_messages_received)
GET_PIMSTAT_PER_NODE(register_messages_sent)
GET_PIMSTAT_PER_NODE(register_messages_rx_errors)
GET_PIMSTAT_PER_NODE(register_stop_messages_received)
GET_PIMSTAT_PER_NODE(register_stop_messages_sent)
GET_PIMSTAT_PER_NODE(register_stop_messages_rx_errors)
GET_PIMSTAT_PER_NODE(join_prune_messages_received)
GET_PIMSTAT_PER_NODE(join_prune_messages_sent)
GET_PIMSTAT_PER_NODE(join_prune_messages_rx_errors)
GET_PIMSTAT_PER_NODE(bootstrap_messages_received)
GET_PIMSTAT_PER_NODE(bootstrap_messages_sent)
GET_PIMSTAT_PER_NODE(bootstrap_messages_rx_errors)
GET_PIMSTAT_PER_NODE(assert_messages_received)
GET_PIMSTAT_PER_NODE(assert_messages_sent)
GET_PIMSTAT_PER_NODE(assert_messages_rx_errors)
GET_PIMSTAT_PER_NODE(graft_messages_received)
GET_PIMSTAT_PER_NODE(graft_messages_sent)
GET_PIMSTAT_PER_NODE(graft_messages_rx_errors)
GET_PIMSTAT_PER_NODE(graft_ack_messages_received)
GET_PIMSTAT_PER_NODE(graft_ack_messages_sent)
GET_PIMSTAT_PER_NODE(graft_ack_messages_rx_errors)
GET_PIMSTAT_PER_NODE(candidate_rp_messages_received)
GET_PIMSTAT_PER_NODE(candidate_rp_messages_sent)
GET_PIMSTAT_PER_NODE(candidate_rp_messages_rx_errors)
//
GET_PIMSTAT_PER_NODE(unknown_type_messages)
GET_PIMSTAT_PER_NODE(unknown_version_messages)
GET_PIMSTAT_PER_NODE(neighbor_unknown_messages)
GET_PIMSTAT_PER_NODE(bad_length_messages)
GET_PIMSTAT_PER_NODE(bad_checksum_messages)
GET_PIMSTAT_PER_NODE(bad_receive_interface_messages)
GET_PIMSTAT_PER_NODE(rx_interface_disabled_messages)
GET_PIMSTAT_PER_NODE(rx_register_not_rp)
GET_PIMSTAT_PER_NODE(rp_filtered_source)
GET_PIMSTAT_PER_NODE(unknown_register_stop)
GET_PIMSTAT_PER_NODE(rx_join_prune_no_state)
GET_PIMSTAT_PER_NODE(rx_graft_graft_ack_no_state)
GET_PIMSTAT_PER_NODE(rx_graft_on_upstream_interface)
GET_PIMSTAT_PER_NODE(rx_candidate_rp_not_bsr)
GET_PIMSTAT_PER_NODE(rx_bsr_when_bsr)
GET_PIMSTAT_PER_NODE(rx_bsr_not_rpf_interface)
GET_PIMSTAT_PER_NODE(rx_unknown_hello_option)
GET_PIMSTAT_PER_NODE(rx_data_no_state)
GET_PIMSTAT_PER_NODE(rx_rp_no_state)
GET_PIMSTAT_PER_NODE(rx_aggregate)
GET_PIMSTAT_PER_NODE(rx_malformed_packet)
GET_PIMSTAT_PER_NODE(no_rp)
GET_PIMSTAT_PER_NODE(no_route_upstream)
GET_PIMSTAT_PER_NODE(rp_mismatch)
GET_PIMSTAT_PER_NODE(rpf_neighbor_unknown)
//
GET_PIMSTAT_PER_NODE(rx_join_rp)
GET_PIMSTAT_PER_NODE(rx_prune_rp)
GET_PIMSTAT_PER_NODE(rx_join_wc)
GET_PIMSTAT_PER_NODE(rx_prune_wc)
GET_PIMSTAT_PER_NODE(rx_join_sg)
GET_PIMSTAT_PER_NODE(rx_prune_sg)
GET_PIMSTAT_PER_NODE(rx_join_sg_rpt)
GET_PIMSTAT_PER_NODE(rx_prune_sg_rpt)

#undef GET_PIMSTAT_PER_NODE


#define GET_PIMSTAT_PER_VIF(stat_name)				\
int								\
PimNode::pimstat_##stat_name##_per_vif(const string& vif_name, uint32_t& result, string& error_msg) const \
{								\
    result = 0;							\
								\
    PimVif *pim_vif = vif_find_by_name(vif_name);		\
    if (pim_vif == NULL) {					\
	error_msg = c_format("Cannot get statistics for vif %s: no such vif", \
			     vif_name.c_str());			\
	return (XORP_ERROR);					\
    }								\
								\
    result = pim_vif->pimstat_##stat_name();				\
    return (XORP_OK);						\
}

GET_PIMSTAT_PER_VIF(hello_messages_received)
GET_PIMSTAT_PER_VIF(hello_messages_sent)
GET_PIMSTAT_PER_VIF(hello_messages_rx_errors)
GET_PIMSTAT_PER_VIF(register_messages_received)
GET_PIMSTAT_PER_VIF(register_messages_sent)
GET_PIMSTAT_PER_VIF(register_messages_rx_errors)
GET_PIMSTAT_PER_VIF(register_stop_messages_received)
GET_PIMSTAT_PER_VIF(register_stop_messages_sent)
GET_PIMSTAT_PER_VIF(register_stop_messages_rx_errors)
GET_PIMSTAT_PER_VIF(join_prune_messages_received)
GET_PIMSTAT_PER_VIF(join_prune_messages_sent)
GET_PIMSTAT_PER_VIF(join_prune_messages_rx_errors)
GET_PIMSTAT_PER_VIF(bootstrap_messages_received)
GET_PIMSTAT_PER_VIF(bootstrap_messages_sent)
GET_PIMSTAT_PER_VIF(bootstrap_messages_rx_errors)
GET_PIMSTAT_PER_VIF(assert_messages_received)
GET_PIMSTAT_PER_VIF(assert_messages_sent)
GET_PIMSTAT_PER_VIF(assert_messages_rx_errors)
GET_PIMSTAT_PER_VIF(graft_messages_received)
GET_PIMSTAT_PER_VIF(graft_messages_sent)
GET_PIMSTAT_PER_VIF(graft_messages_rx_errors)
GET_PIMSTAT_PER_VIF(graft_ack_messages_received)
GET_PIMSTAT_PER_VIF(graft_ack_messages_sent)
GET_PIMSTAT_PER_VIF(graft_ack_messages_rx_errors)
GET_PIMSTAT_PER_VIF(candidate_rp_messages_received)
GET_PIMSTAT_PER_VIF(candidate_rp_messages_sent)
GET_PIMSTAT_PER_VIF(candidate_rp_messages_rx_errors)
//
GET_PIMSTAT_PER_VIF(unknown_type_messages)
GET_PIMSTAT_PER_VIF(unknown_version_messages)
GET_PIMSTAT_PER_VIF(neighbor_unknown_messages)
GET_PIMSTAT_PER_VIF(bad_length_messages)
GET_PIMSTAT_PER_VIF(bad_checksum_messages)
GET_PIMSTAT_PER_VIF(bad_receive_interface_messages)
GET_PIMSTAT_PER_VIF(rx_interface_disabled_messages)
GET_PIMSTAT_PER_VIF(rx_register_not_rp)
GET_PIMSTAT_PER_VIF(rp_filtered_source)
GET_PIMSTAT_PER_VIF(unknown_register_stop)
GET_PIMSTAT_PER_VIF(rx_join_prune_no_state)
GET_PIMSTAT_PER_VIF(rx_graft_graft_ack_no_state)
GET_PIMSTAT_PER_VIF(rx_graft_on_upstream_interface)
GET_PIMSTAT_PER_VIF(rx_candidate_rp_not_bsr)
GET_PIMSTAT_PER_VIF(rx_bsr_when_bsr)
GET_PIMSTAT_PER_VIF(rx_bsr_not_rpf_interface)
GET_PIMSTAT_PER_VIF(rx_unknown_hello_option)
GET_PIMSTAT_PER_VIF(rx_data_no_state)
GET_PIMSTAT_PER_VIF(rx_rp_no_state)
GET_PIMSTAT_PER_VIF(rx_aggregate)
GET_PIMSTAT_PER_VIF(rx_malformed_packet)
GET_PIMSTAT_PER_VIF(no_rp)
GET_PIMSTAT_PER_VIF(no_route_upstream)
GET_PIMSTAT_PER_VIF(rp_mismatch)
GET_PIMSTAT_PER_VIF(rpf_neighbor_unknown)
//
GET_PIMSTAT_PER_VIF(rx_join_rp)
GET_PIMSTAT_PER_VIF(rx_prune_rp)
GET_PIMSTAT_PER_VIF(rx_join_wc)
GET_PIMSTAT_PER_VIF(rx_prune_wc)
GET_PIMSTAT_PER_VIF(rx_join_sg)
GET_PIMSTAT_PER_VIF(rx_prune_sg)
GET_PIMSTAT_PER_VIF(rx_join_sg_rpt)
GET_PIMSTAT_PER_VIF(rx_prune_sg_rpt)

#undef GET_PIMSTAT_PER_VIF
