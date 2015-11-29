// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/pim/pim_mrib_table.hh,v 1.15 2008/10/02 21:57:53 bms Exp $


#ifndef __PIM_PIM_MRIB_TABLE_HH__
#define __PIM_PIM_MRIB_TABLE_HH__


//
// PIM Multicast Routing Information Base Table header file.
//




#include "libxorp/timer.hh"
#include "mrt/mrib_table.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class IPvXNet;
class PimMrt;
class PimNode;

// PIM-specific Multicast Routing Information Base Table

/**
 * @short PIM-specific Multicast Routing Information Base Table
 */
class PimMribTable : public MribTable 
{
	public:
		/**
		 * Constructor.
		 * 
		 * @param pim_node the PimNode this table belongs to.
		 */
		PimMribTable(PimNode& pim_node);

		/**
		 * Destructor.
		 */
		virtual ~PimMribTable();

		// Redirection functions (to the pim_node)    

		/**
		 * Get the PimNode this table belongs to.
		 * 
		 * @return the PimNode this table belongs to.
		 * @see PimTable.
		 */
		PimNode&	pim_node() const { return (_pim_node);			}

		/**
		 * Get the address family.
		 * 
		 * @return the address family (AF_INET or AF_INET6 for
		 * IPv4 and IPv6 respectively).
		 */
		int		family();

		/**
		 * Get the corresponding PIM Multicast Routing Table.
		 * 
		 * @return the corresponding PIM Multicast Routing Table.
		 * @see PimMrt.
		 */
		PimMrt&	pim_mrt();

		/**
		 * Clear the table by removing all entries.
		 */
		void	clear();

		/**
		 * Search the table and find the corresponding Mrib entry for a given
		 * destination address.
		 * 
		 * @param address the destination address to search for.
		 * @return the Mrib entry for the destination address.
		 * @see Mrib.
		 */
		Mrib	*find(const IPvX& address) const;

		/**
		 * Add a MRIB entry to the MRIB table.
		 * 
		 * Note that if the MRIB entry is for one of my own addresses, then we
		 * check the next-hop interface. If it points toward the loopback
		 * interface (e.g., in case of KAME IPv6 stack), then we overwrite it
		 * with the network interface this address belongs to.
		 * 
		 * @param tid the transaction ID.
		 * @param mrib the MRIB entry to add.
		 * @param next_hop_vif_name the next-hop vif name. It is used for
		 * later resolving of Mrib::_next_hop_vif_index if the vif name
		 * is not known yet.
		 * @see Mrib.
		 */
		void	add_pending_insert(uint32_t tid, const Mrib& mrib,
				const string& next_hop_vif_name);

		/**
		 * Remove a MRIB entry from the MRIB table.
		 * 
		 * @param tid the transaction ID.
		 * @param mrib the MRIB entry to remove.
		 */
		void	add_pending_remove(uint32_t tid, const Mrib& mrib);

		/**
		 * Remove all MRIB entries from the MRIB table.
		 * 
		 * @param tid the transaction ID.
		 */
		void	add_pending_remove_all_entries(uint32_t tid);

		/**
		 * Commit all pending MRIB entries to the MRIB table.
		 * 
		 * @param tid the transaction ID for the pending MRIB entries to commit.
		 */
		void	commit_pending_transactions(uint32_t tid);

		/**
		 * Apply all changes to the table.
		 * 
		 * Note that this may trigger various changes to the PIM protocol state
		 * machines.
		 */
		void	apply_mrib_changes();

		/**
		 * Get the list of modified prefixes since the last commit.
		 * 
		 * @return the list of modified prefixes since the last commit.
		 */
		list<IPvXNet>& modified_prefix_list() { return (_modified_prefix_list); }

		/**
		 * Resolve all destination prefixes whose next-hop vif name was not
		 * resolved earlier (e.g., the vif was unknown).
		 * 
		 * @param next_hop_vif_name the name of the resolved vif.
		 * @param next_hop_vif_index the index of the resolved vif.
		 */
		void	resolve_prefixes_by_vif_name(const string& next_hop_vif_name,
				uint32_t next_hop_vif_index);

	private:
		PimNode&	_pim_node;		// The PIM node this table belongs to.

		/**
		 * Add/merge a modified prefix to the '_modified_prefix_list'.
		 */
		void	add_modified_prefix(const IPvXNet& modified_prefix);

		/**
		 * Add a destination prefix whose next-hop vif name was not resolved
		 * (e.g., the vif is unknown).
		 */
		void	add_unresolved_prefix(const IPvXNet& dest_prefix,
				const string& next_hop_vif_name);

		/**
		 * Delete a destination prefix whose next-hop vif name was not resolved
		 * earlier (e.g., the vif was unknown).
		 */
		void	delete_unresolved_prefix(const IPvXNet& dest_prefix);

		// The merged and enlarged list of modified prefixes that need
		// to be applied to the PimMrt.
		list<IPvXNet> _modified_prefix_list;

		// The map of unresolved prefixes whose next-hop vif name was not resolved
		map<IPvXNet, string> _unresolved_prefixes;
};

#endif // __PIM_PIM_MRIB_TABLE_HH__
