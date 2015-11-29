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

// $XORP: xorp/fea/fibconfig_table_set.hh,v 1.16 2008/10/02 21:56:46 bms Exp $

#ifndef __FEA_FIBCONFIG_TABLE_SET_HH__
#define __FEA_FIBCONFIG_TABLE_SET_HH__



#include "fte.hh"
#include "iftree.hh"
#include "fea_data_plane_manager.hh"

class FibConfig;


class FibConfigTableSet 
{
	public:
		/**
		 * Constructor.
		 *
		 * @param fea_data_plane_manager the corresponding data plane manager
		 * (@ref FeaDataPlaneManager).
		 */
		FibConfigTableSet(FeaDataPlaneManager& fea_data_plane_manager)
			: _is_running(false),
			_fibconfig(fea_data_plane_manager.fibconfig()),
			_fea_data_plane_manager(fea_data_plane_manager),
			_in_configuration(false)
	{}

		/**
		 * Virtual destructor.
		 */
		virtual ~FibConfigTableSet() {}

		/**
		 * Get the @ref FibConfig instance.
		 *
		 * @return the @ref FibConfig instance.
		 */
		FibConfig&	fibconfig() { return _fibconfig; }

		/**
		 * Get the @ref FeaDataPlaneManager instance.
		 *
		 * @return the @ref FeaDataPlaneManager instance.
		 */
		FeaDataPlaneManager& fea_data_plane_manager() { return _fea_data_plane_manager; }

		/**
		 * Test whether this instance is running.
		 *
		 * @return true if the instance is running, otherwise false.
		 */
		virtual bool is_running() const { return _is_running; }

		/**
		 * Start operation.
		 * 
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int start(string& error_msg) = 0;

		/**
		 * Stop operation.
		 * 
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int stop(string& error_msg) = 0;

		/**
		 * Start a configuration interval.
		 *
		 * All modifications to must be within a marked "configuration" interval.
		 *
		 * This method provides derived classes with a mechanism to perform
		 * any actions necessary before forwarding table modifications can
		 * be made.
		 *
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int start_configuration(string& error_msg) 
		{
			// Nothing particular to do, just label start.
			return mark_configuration_start(error_msg);
		}

		/**
		 * End of configuration interval.
		 *
		 * This method provides derived classes with a mechanism to
		 * perform any actions necessary at the end of a configuration, eg
		 * write a file.
		 *
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int end_configuration(string& error_msg) 
		{
			// Nothing particular to do, just label start.
			return mark_configuration_end(error_msg);
		}

		/**
		 * Set the IPv4 unicast forwarding table.
		 *
		 * @param fte_list the list with all entries to install into
		 * the IPv4 unicast forwarding table.
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int set_table4(const list<Fte4>& fte_list) = 0;

		/**
		 * Delete all entries in the IPv4 unicast forwarding table.
		 *
		 * Must be within a configuration interval.
		 *
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int delete_all_entries4() = 0;

		/**
		 * Set the IPv6 unicast forwarding table.
		 *
		 * @param fte_list the list with all entries to install into
		 * the IPv6 unicast forwarding table.
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int set_table6(const list<Fte6>& fte_list) = 0;

		/**
		 * Delete all entries in the IPv6 unicast forwarding table.
		 *
		 * Must be within a configuration interval.
		 *
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int delete_all_entries6() = 0;

		/** Routing table ID that we are interested in might have changed.  Maybe something
		 * can filter on this for increased efficiency.
		 */
		virtual int notify_table_id_change(uint32_t new_tbl) = 0;

	protected:
		/**
		 * Mark start of a configuration.
		 *
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		int mark_configuration_start(string& error_msg) 
		{
			if (_in_configuration != true) 
			{
				_in_configuration = true;
				return (XORP_OK);
			}
			error_msg = c_format("Cannot start configuration: "
					"configuration in progress");
			return (XORP_ERROR);
		}

		/**
		 * Mark end of a configuration.
		 *
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		int mark_configuration_end(string& error_msg) 
		{
			if (_in_configuration != false) 
			{
				_in_configuration = false;
				return (XORP_OK);
			}
			error_msg = c_format("Cannot end configuration: "
					"configuration not in progress");
			return (XORP_ERROR);
		}

		bool in_configuration() const { return _in_configuration; }

		// Misc other state
		bool	_is_running;

	private:
		FibConfig&		_fibconfig;
		FeaDataPlaneManager& _fea_data_plane_manager;
		bool		_in_configuration;
};

#endif // __FEA_FIBCONFIG_TABLE_SET_HH__
