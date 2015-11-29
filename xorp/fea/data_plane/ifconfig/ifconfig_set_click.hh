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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_click.hh,v 1.13 2008/10/02 21:57:07 bms Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_CLICK_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_CLICK_HH__

#include <xorp_config.h>
#ifdef XORP_USE_CLICK


#include "fea/ifconfig_set.hh"
#include "fea/data_plane/control_socket/click_socket.hh"

class RunCommand;


class IfConfigSetClick : public IfConfigSet,
	public ClickSocket 
{
	private:
		class ClickConfigGenerator;

	public:
		/**
		 * Constructor.
		 *
		 * @param fea_data_plane_manager the corresponding data plane manager
		 * (@ref FeaDataPlaneManager).
		 */
		IfConfigSetClick(FeaDataPlaneManager& fea_data_plane_manager);

		/**
		 * Virtual destructor.
		 */
		virtual ~IfConfigSetClick();

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
		 * Get a reference to the @ref IfTree instance.
		 *
		 * @return a reference to the @ref IfTree instance.
		 */
		const IfTree& iftree() const { return _iftree; }

		/**
		 * Receive a signal the work of a Click configuration generator is done.
		 *
		 * @param click_config_generator a pointer to the @ref
		 * IfConfigSetClick::ClickConfigGenerator instance that has completed
		 * its work.
		 * @param success if true, then the generator has successfully finished
		 * its work, otherwise false.
		 * @param error_msg the error message (if error).
		 */
		void click_config_generator_done(
				IfConfigSetClick::ClickConfigGenerator* click_config_generator,
				bool success,
				const string& error_msg);

	private:
		/**
		 * Determine if the interface's underlying provider implements discard
		 * semantics natively, or if they are emulated through other means.
		 *
		 * @param i the interface item to inspect.
		 * @return true if discard semantics are emulated.
		 */
		virtual bool is_discard_emulated(const IfTreeInterface& i) const;

		/**
		 * Determine if the interface's underlying provider implements unreachable
		 * semantics natively, or if they are emulated through other means.
		 *
		 * @param i the interface item to inspect.
		 * @return true if unreachable semantics are emulated.
		 */
		virtual bool is_unreachable_emulated(const IfTreeInterface& i) const;

		/**
		 * Start the configuration.
		 *
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_begin(string& error_msg);

		/**
		 * Complete the configuration.
		 *
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_end(string& error_msg);

		/**
		 * Begin the interface configuration.
		 *
		 * @param pulled_ifp pointer to the interface information pulled from
		 * the system.
		 * @param config_iface reference to the interface with the information
		 * to configure.
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_interface_begin(const IfTreeInterface* pulled_ifp,
				IfTreeInterface& config_iface,
				string& error_msg);

		/**
		 * End the interface configuration.
		 *
		 * @param pulled_ifp pointer to the interface information pulled from
		 * the system.
		 * @param config_iface reference to the interface with the information
		 * to configure.
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_interface_end(const IfTreeInterface* pulled_ifp,
				const IfTreeInterface& config_iface,
				string& error_msg);

		/**
		 * Begin the vif configuration.
		 *
		 * @param pulled_ifp pointer to the interface information pulled from
		 * the system.
		 * @param pulled_vifp pointer to the vif information pulled from
		 * the system.
		 * @param config_iface reference to the interface with the information
		 * to configure.
		 * @param config_vif reference to the vif with the information
		 * to configure.
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_vif_begin(const IfTreeInterface* pulled_ifp,
				const IfTreeVif* pulled_vifp,
				const IfTreeInterface& config_iface,
				const IfTreeVif& config_vif,
				string& error_msg);

		/**
		 * End the vif configuration.
		 *
		 * @param pulled_ifp pointer to the interface information pulled from
		 * the system.
		 * @param pulled_vifp pointer to the vif information pulled from
		 * the system.
		 * @param config_iface reference to the interface with the information
		 * to configure.
		 * @param config_vif reference to the vif with the information
		 * to configure.
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_vif_end(const IfTreeInterface* pulled_ifp,
				const IfTreeVif* pulled_vifp,
				const IfTreeInterface& config_iface,
				const IfTreeVif& config_vif,
				string& error_msg);

		/**
		 * Add IPv4 address information.
		 *
		 * If an entry for the same address already exists, is is overwritten
		 * with the new information.
		 *
		 * @param pulled_ifp pointer to the interface information pulled from
		 * the system.
		 * @param pulled_vifp pointer to the vif information pulled from
		 * the system.
		 * @param pulled_addrp pointer to the address information pulled from
		 * the system.
		 * @param config_iface reference to the interface with the information
		 * to configure.
		 * @param config_vif reference to the vif with the information
		 * to configure.
		 * @param config_addr reference to the address with the information
		 * to configure.
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_add_address(const IfTreeInterface* pulled_ifp,
				const IfTreeVif* pulled_vifp,
				const IfTreeAddr4* pulled_addrp,
				const IfTreeInterface& config_iface,
				const IfTreeVif& config_vif,
				const IfTreeAddr4& config_addr,
				string& error_msg);

		/**
		 * Delete IPv4 address information.
		 *
		 * @param pulled_ifp pointer to the interface information pulled from
		 * the system.
		 * @param pulled_vifp pointer to the vif information pulled from
		 * the system.
		 * @param pulled_addrp pointer to the address information pulled from
		 * the system.
		 * @param config_iface reference to the interface with the information
		 * to configure.
		 * @param config_vif reference to the vif with the information
		 * to configure.
		 * @param config_addr reference to the address with the information
		 * to configure.
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_delete_address(const IfTreeInterface* pulled_ifp,
				const IfTreeVif* pulled_vifp,
				const IfTreeAddr4* pulled_addrp,
				const IfTreeInterface& config_iface,
				const IfTreeVif& config_vif,
				const IfTreeAddr4& config_addr,
				string& error_msg);

		/**
		 * Add IPv6 address information.
		 *
		 * If an entry for the same address already exists, is is overwritten
		 * with the new information.
		 *
		 * @param pulled_ifp pointer to the interface information pulled from
		 * the system.
		 * @param pulled_vifp pointer to the vif information pulled from
		 * the system.
		 * @param pulled_addrp pointer to the address information pulled from
		 * the system.
		 * @param config_iface reference to the interface with the information
		 * to configure.
		 * @param config_vif reference to the vif with the information
		 * to configure.
		 * @param config_addr reference to the address with the information
		 * to configure.
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_add_address(const IfTreeInterface* pulled_ifp,
				const IfTreeVif* pulled_vifp,
				const IfTreeAddr6* pulled_addrp,
				const IfTreeInterface& config_iface,
				const IfTreeVif& config_vif,
				const IfTreeAddr6& config_addr,
				string& error_msg);

		/**
		 * Delete IPv6 address information.
		 *
		 * @param pulled_ifp pointer to the interface information pulled from
		 * the system.
		 * @param pulled_vifp pointer to the vif information pulled from
		 * the system.
		 * @param pulled_addrp pointer to the address information pulled from
		 * the system.
		 * @param config_iface reference to the interface with the information
		 * to configure.
		 * @param config_vif reference to the vif with the information
		 * to configure.
		 * @param config_addr reference to the address with the information
		 * to configure.
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		virtual int config_delete_address(const IfTreeInterface* pulled_ifp,
				const IfTreeVif* pulled_vifp,
				const IfTreeAddr6* pulled_addrp,
				const IfTreeInterface& config_iface,
				const IfTreeVif& config_vif,
				const IfTreeAddr6& config_addr,
				string& error_msg);

		int execute_click_config_generator(string& error_msg);
		void terminate_click_config_generator();
		int write_generated_config(bool is_kernel_click,
				const string& kernel_config,
				bool is_user_click,
				const string& user_config,
				string& error_msg);
		string regenerate_xorp_iftree_config() const;
		string regenerate_xorp_fea_click_config() const;

		/**
		 * Generate the next-hop to port mapping.
		 *
		 * @return the number of generated ports.
		 */
		int generate_nexthop_to_port_mapping();

		ClickSocketReader	_cs_reader;
		IfTree		_iftree;

		class ClickConfigGenerator 
		{
			public:
				ClickConfigGenerator(IfConfigSetClick& ifconfig_set_click,
						const string& command_name);
				~ClickConfigGenerator();
				int execute(const string& xorp_config, string& error_msg);

				const string& command_name() const { return _command_name; }
				const string& command_stdout() const { return _command_stdout; }

			private:
				void stdout_cb(RunCommand* run_command, const string& output);
				void stderr_cb(RunCommand* run_command, const string& output);
				void done_cb(RunCommand* run_command, bool success,
						const string& error_msg);

				IfConfigSetClick& _ifconfig_set_click;
				string		_command_name;
				list<string>	_command_argument_list;
				RunCommand*	_run_command;
				string		_command_stdout;
				string		_tmp_filename;
		};

		ClickConfigGenerator*	_kernel_click_config_generator;
		ClickConfigGenerator*	_user_click_config_generator;
		bool			_has_kernel_click_config;
		bool			_has_user_click_config;
		string			_generated_kernel_click_config;
		string			_generated_user_click_config;
};

#endif
#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_CLICK_HH__
