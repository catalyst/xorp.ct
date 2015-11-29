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


#ifndef __FEA_FIBCONFIG_ENTRY_OBSERVER_HH__
#define __FEA_FIBCONFIG_ENTRY_OBSERVER_HH__



#include "fte.hh"
#include "iftree.hh"
#include "fea_data_plane_manager.hh"

class FibConfig;


class FibConfigEntryObserver 
{
	public:
		/**
		 * Constructor.
		 *
		 * @param fea_data_plane_manager the corresponding data plane manager
		 * (@ref FeaDataPlaneManager).
		 */
		FibConfigEntryObserver(FeaDataPlaneManager& fea_data_plane_manager)
			: _is_running(false),
			_fibconfig(fea_data_plane_manager.fibconfig()),
			_fea_data_plane_manager(fea_data_plane_manager)
	{}

		/**
		 * Virtual destructor.
		 */
		virtual ~FibConfigEntryObserver() {}

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
		 * Receive data from the underlying system.
		 * 
		 * @param buffer the buffer with the received data.
		 */
		virtual void receive_data(vector<uint8_t>& buffer) = 0;

		/** Routing table ID that we are interested in might have changed.  Maybe something
		 * can filter on this for increased efficiency.
		 */
		virtual int notify_table_id_change(uint32_t new_tbl) = 0;

	protected:
		// Misc other state
		bool	_is_running;

	private:
		FibConfig&		_fibconfig;
		FeaDataPlaneManager& _fea_data_plane_manager;
};

#endif // __FEA_FIBCONFIG_ENTRY_OBSERVER_HH__
