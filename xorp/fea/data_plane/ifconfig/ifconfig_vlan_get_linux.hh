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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_vlan_get_linux.hh,v 1.5 2008/10/02 21:57:09 bms Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_GET_LINUX_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_GET_LINUX_HH__

#include "fea/ifconfig_vlan_get.hh"


class IfConfigVlanGetLinux : public IfConfigVlanGet 
{
	public:
		/**
		 * Constructor.
		 *
		 * @param fea_data_plane_manager the corresponding data plane manager
		 * (@ref FeaDataPlaneManager).
		 */
		IfConfigVlanGetLinux(FeaDataPlaneManager& fea_data_plane_manager, bool is_dummy);

		/**
		 * Virtual destructor.
		 */
		virtual ~IfConfigVlanGetLinux();

		/**
		 * Start operation.
		 * 
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int start(string& error_msg);

		/**
		 * Stop operation.
		 * 
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int stop(string& error_msg);

		/**
		 * Pull the VLAN network interface information from the underlying system.
		 * 
		 * The VLAN information is added to the existing state in the iftree.
		 *
		 * @param iftree the IfTree storage to store the pulled information.
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int pull_config(IfTree& iftree, bool& modified);

	private:
		int read_config(IfTree& iftree, bool& modified);
		bool _is_dummy;
		int _s4;
};

#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_GET_LINUX_HH__
