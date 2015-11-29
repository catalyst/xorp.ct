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



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/firewall_manager.hh"

#include "firewall_set_dummy.hh"


//
// Set firewall information into the underlying system.
//
// The mechanism to set the information is Dummy (for testing purpose).
//

	FirewallSetDummy::FirewallSetDummy(FeaDataPlaneManager& fea_data_plane_manager)
: FirewallSet(fea_data_plane_manager)
{
}

FirewallSetDummy::~FirewallSetDummy()
{
	string error_msg;

	if (stop(error_msg) != XORP_OK) 
	{
		XLOG_ERROR("Cannot stop the Dummy mechanism to set "
				"firewall information into the underlying system: %s",
				error_msg.c_str());
	}
}

	int
FirewallSetDummy::start(string& error_msg)
{
	UNUSED(error_msg);

	if (_is_running)
		return (XORP_OK);

	_is_running = true;

	return (XORP_OK);
}

	int
FirewallSetDummy::stop(string& error_msg)
{
	UNUSED(error_msg);

	if (! _is_running)
		return (XORP_OK);

	_is_running = false;

	return (XORP_OK);
}

	int
FirewallSetDummy::update_entries(const list<FirewallEntry>& added_entries,
		const list<FirewallEntry>& replaced_entries,
		const list<FirewallEntry>& deleted_entries,
		string& error_msg)
{
	list<FirewallEntry>::const_iterator iter;

	//
	// The entries to add
	//
	for (iter = added_entries.begin();
			iter != added_entries.end();
			++iter) 
	{
		const FirewallEntry& firewall_entry = *iter;
		if (add_entry(firewall_entry, error_msg) != XORP_OK)
			return (XORP_ERROR);
	}

	//
	// The entries to replace
	//
	for (iter = replaced_entries.begin();
			iter != replaced_entries.end();
			++iter) 
	{
		const FirewallEntry& firewall_entry = *iter;
		if (replace_entry(firewall_entry, error_msg) != XORP_OK)
			return (XORP_ERROR);
	}

	//
	// The entries to delete
	//
	for (iter = deleted_entries.begin();
			iter != deleted_entries.end();
			++iter) 
	{
		const FirewallEntry& firewall_entry = *iter;
		if (delete_entry(firewall_entry, error_msg) != XORP_OK)
			return (XORP_ERROR);
	}

	return (XORP_OK);
}

	int
FirewallSetDummy::set_table4(const list<FirewallEntry>& firewall_entry_list,
		string& error_msg)
{
	list<FirewallEntry> empty_list;

	if (delete_all_entries4(error_msg) != XORP_OK)
		return (XORP_ERROR);

	return (update_entries(firewall_entry_list, empty_list, empty_list,
				error_msg));
}

	int
FirewallSetDummy::delete_all_entries4(string& error_msg)
{
	UNUSED(error_msg);

	_firewall_entries4.clear();

	return (XORP_OK);
}

	int
FirewallSetDummy::set_table6(const list<FirewallEntry>& firewall_entry_list,
		string& error_msg)
{
	list<FirewallEntry> empty_list;

	if (delete_all_entries6(error_msg) != XORP_OK)
		return (XORP_ERROR);

	return (update_entries(firewall_entry_list, empty_list, empty_list,
				error_msg));
}

	int
FirewallSetDummy::delete_all_entries6(string& error_msg)
{
	UNUSED(error_msg);

	_firewall_entries6.clear();

	return (XORP_OK);
}

	int
FirewallSetDummy::add_entry(const FirewallEntry& firewall_entry,
		string& error_msg)
{
	FirewallTrie::iterator iter;
	FirewallTrie* ftp = NULL;
	uint32_t key = firewall_entry.rule_number();  // XXX: the map key

	UNUSED(error_msg);

	if (firewall_entry.is_ipv4())
		ftp = &_firewall_entries4;
	else
		ftp = &_firewall_entries6;

	//
	// XXX: If the entry already exists, then just update it.
	// Note that the replace_entry() implementation relies on this.
	//
	iter = ftp->find(key);
	if (iter == ftp->end()) 
	{
		ftp->insert(make_pair(key, firewall_entry));
	} else 
	{
		FirewallEntry& fe_tmp = iter->second;
		fe_tmp = firewall_entry;
	}

	return (XORP_OK);
}

	int
FirewallSetDummy::replace_entry(const FirewallEntry& firewall_entry,
		string& error_msg)
{
	//
	// XXX: The add_entry() method implementation covers the replace_entry()
	// semantic as well.
	//
	return (add_entry(firewall_entry, error_msg));
}

	int
FirewallSetDummy::delete_entry(const FirewallEntry& firewall_entry,
		string& error_msg)
{
	FirewallTrie::iterator iter;
	FirewallTrie* ftp = NULL;
	uint32_t key = firewall_entry.rule_number();  // XXX: the map key

	if (firewall_entry.is_ipv4())
		ftp = &_firewall_entries4;
	else
		ftp = &_firewall_entries6;

	// Find the entry
	iter = ftp->find(key);
	if (iter == ftp->end()) 
	{
		error_msg = c_format("Entry not found");
		return (XORP_ERROR);
	}
	ftp->erase(iter);

	return (XORP_OK);
}
