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




//
// CLI (Command-Line Interface) implementation for XORP.
//


#include "cli_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/ipvxnet.hh"
#include "libxorp/token.hh"
#include "libxorp/utils.hh"

#include "libcomm/comm_api.h"

#include "cli_client.hh"
#include "cli_node.hh"
#include "cli_private.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//


/**
 * CliNode::CliNode:
 * @init_family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @init_module_id: The module ID (must be %XORP_MODULE_CLI).
 * 
 * CLI node constructor.
 **/
CliNode::CliNode(int init_family, xorp_module_id module_id)
	: ProtoNode<Vif>(init_family, module_id),
	_cli_port(0),		// XXX: not defined yet
	_next_session_id(0),
	_startup_cli_prompt(XORP_CLI_PROMPT),
	_cli_command_root(NULL, "", ""),
	_is_log_trace(false)
{
	string error_msg;

	if (module_id != XORP_MODULE_CLI) 
	{
		XLOG_FATAL("Invalid module ID = %d (must be 'XORP_MODULE_CLI' = %d)",
				module_id, XORP_MODULE_CLI);
	}

	cli_command_root()->set_allow_cd(true, _startup_cli_prompt);
	cli_command_root()->create_default_cli_commands();
	if (cli_command_root()->add_pipes(error_msg) != XORP_OK) 
	{
		XLOG_FATAL("Cannot add command pipes: %s", error_msg.c_str());
	}
}

/**
 * CliNode::~CliNode:
 * @: 
 * 
 * CLI node destructor.
 **/
CliNode::~CliNode()
{
	stop();

	// TODO: perform CLI-specific operations.
}

	int
CliNode::start()
{
	string error_msg;

	if (! is_enabled())
		return (XORP_OK);

	if (is_up() || is_pending_up())
		return (XORP_OK);

	if (ProtoNode<Vif>::start() != XORP_OK)
		return (XORP_ERROR);

	// Perform misc. CLI-specific start operations

	//
	// Start accepting connections
	//
	if (_cli_port != 0) 
	{
		if (sock_serv_open().is_valid()) 
		{
			EventLoop::instance().add_ioevent_cb(_cli_socket, IOT_ACCEPT,
					callback(this, &CliNode::accept_connection),
					XorpTask::PRIORITY_HIGHEST);
		}
	}

	if (add_internal_cli_commands(error_msg) != XORP_OK) 
	{
		XLOG_ERROR("Cannot add internal CLI commands: %s", error_msg.c_str());
		return (XORP_ERROR);
	}

	XLOG_TRACE(is_log_trace(), "CLI started");

	return (XORP_OK);
}

/**
 * CliNode::stop:
 * @: 
 * 
 * Stop the CLI operation.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
	int
CliNode::stop()
{
	if (is_down())
		return (XORP_OK);

	if (! is_up())
		return (XORP_ERROR);

	if (ProtoNode<Vif>::pending_stop() != XORP_OK)
		return (XORP_ERROR);

	// Perform misc. CLI-specific stop operations
	// TODO: add as necessary

	delete_pointers_list(_client_list);

	if (_cli_socket.is_valid())
		EventLoop::instance().remove_ioevent_cb(_cli_socket, IOT_ACCEPT);
	sock_serv_close();

	if (ProtoNode<Vif>::stop() != XORP_OK)
		return (XORP_ERROR);

	XLOG_TRACE(is_log_trace(), "CLI stopped");

	return (XORP_OK);
}

/**
 * Enable the node operation.
 * 
 * If an unit is not enabled, it cannot be start, or pending-start.
 */
	void
CliNode::enable()
{
	ProtoUnit::enable();

	XLOG_TRACE(is_log_trace(), "CLI enabled");
}

/**
 * Disable the node operation.
 * 
 * If an unit is disabled, it cannot be start or pending-start.
 * If the unit was runnning, it will be stop first.
 */
	void
CliNode::disable()
{
	stop();
	ProtoUnit::disable();

	XLOG_TRACE(is_log_trace(), "CLI disabled");
}

/**
 * CliNode::add_enable_cli_access_from_subnet:
 * @subnet_addr: The subnet address to add.
 * 
 * Add a subnet address to the list of subnet addresses enabled
 * for CLI access.
 * This method can be called more than once to add a number of
 * subnet addresses.
 **/
	void
CliNode::add_enable_cli_access_from_subnet(const IPvXNet& subnet_addr)
{
	list<IPvXNet>::iterator iter;

	// Test if we have this subnet already
	for (iter = _enable_cli_access_subnet_list.begin();
			iter != _enable_cli_access_subnet_list.end();
			++iter) 
	{
		const IPvXNet& tmp_ipvxnet = *iter;
		if (tmp_ipvxnet == subnet_addr)
			return;		// Subnet address already added
	}

	_enable_cli_access_subnet_list.push_back(subnet_addr);
}

/**
 * CliNode::delete_enable_cli_access_from_subnet:
 * @subnet_addr: The subnet address to delete.
 * 
 * Delete a subnet address from the list of subnet addresses enabled
 * for CLI access.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
	int
CliNode::delete_enable_cli_access_from_subnet(const IPvXNet& subnet_addr)
{
	list<IPvXNet>::iterator iter;

	// Test if we have this subnet already
	for (iter = _enable_cli_access_subnet_list.begin();
			iter != _enable_cli_access_subnet_list.end();
			++iter) 
	{
		const IPvXNet& tmp_ipvxnet = *iter;
		if (tmp_ipvxnet == subnet_addr) 
		{
			_enable_cli_access_subnet_list.erase(iter);
			return (XORP_OK);	// Entry erased
		}
	}

	return (XORP_ERROR);		// No entry found
}

/**
 * CliNode::add_disable_cli_access_from_subnet:
 * @subnet_addr: The subnet address to add.
 * 
 * Add a subnet address to the list of subnet addresses disabled
 * for CLI access.
 * This method can be called more than once to add a number of
 * subnet addresses.
 **/
	void
CliNode::add_disable_cli_access_from_subnet(const IPvXNet& subnet_addr)
{
	list<IPvXNet>::iterator iter;

	// Test if we have this subnet already
	for (iter = _disable_cli_access_subnet_list.begin();
			iter != _disable_cli_access_subnet_list.end();
			++iter) 
	{
		const IPvXNet& tmp_ipvxnet = *iter;
		if (tmp_ipvxnet == subnet_addr)
			return;		// Subnet address already added
	}

	_disable_cli_access_subnet_list.push_back(subnet_addr);
}

/**
 * CliNode::delete_disable_cli_access_from_subnet:
 * @subnet_addr: The subnet address to delete.
 * 
 * Delete a subnet address from the list of subnet addresses disabled
 * for CLI access.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
	int
CliNode::delete_disable_cli_access_from_subnet(const IPvXNet& subnet_addr)
{
	list<IPvXNet>::iterator iter;

	// Test if we have this subnet already
	for (iter = _disable_cli_access_subnet_list.begin();
			iter != _disable_cli_access_subnet_list.end();
			++iter) 
	{
		const IPvXNet& tmp_ipvxnet = *iter;
		if (tmp_ipvxnet == subnet_addr) 
		{
			_disable_cli_access_subnet_list.erase(iter);
			return (XORP_OK);	// Entry erased
		}
	}

	return (XORP_ERROR);		// No entry found
}

bool
CliNode::is_allow_cli_access(const IPvX& ipvx) const
{
	list<IPvXNet>::const_iterator iter;
	IPvXNet best_enable = IPvXNet(IPvX::ZERO(ipvx.af()), 0);
	IPvXNet best_disable = IPvXNet(IPvX::ZERO(ipvx.af()), 0);
	bool best_enable_found = false;
	bool best_disable_found = false;

	// Find the longest-match subnet address that may enable access
	for (iter = _enable_cli_access_subnet_list.begin();
			iter != _enable_cli_access_subnet_list.end();
			++iter) 
	{
		const IPvXNet& ipvxnet = *iter;
		if (ipvx.af() != ipvxnet.masked_addr().af())
			continue;
		if (! ipvxnet.contains(ipvx))
			continue;
		if (best_enable.contains(ipvxnet))
			best_enable = ipvxnet;
		best_enable_found = true;
	}

	// Find the longest-match subnet address that may disable access
	for (iter = _disable_cli_access_subnet_list.begin();
			iter != _disable_cli_access_subnet_list.end();
			++iter) 
	{
		const IPvXNet& ipvxnet = *iter;
		if (ipvx.af() != ipvxnet.masked_addr().af())
			continue;
		if (! ipvxnet.contains(ipvx))
			continue;
		if (best_disable.contains(ipvxnet))
			best_disable = ipvxnet;
		best_disable_found = true;
	}

	if (! best_disable_found) 
	{
		// XXX: no disable match, so enable access by default
		return (true);
	}

	if (! best_enable_found) 
	{
		// XXX: no enable match, so definitely disable
		return (false);
	}

	// Both enable and disable match
	if (best_enable.prefix_len() > best_disable.prefix_len())
		return (true);

	return (false);		// XXX: disable even if conflicting config
}

/**
 * CliNode::find_cli_by_term_name:
 * @term_name: The CLI terminal name to search for.
 * 
 * Find a CLI client #CliClient for a given terminal name.
 * 
 * Return value: The CLI client with name of @term_name on success,
 * otherwise NULL.
 **/
CliClient *
CliNode::find_cli_by_term_name(const string& term_name) const
{
	list<CliClient *>::const_iterator iter;

	for (iter = _client_list.begin(); iter != _client_list.end(); ++ iter) 
	{
		CliClient *cli_client = *iter;
		if (term_name == cli_client->cli_session_term_name()) 
		{
			return (cli_client);
		}
	}

	return (NULL);
}

/**
 * CliNode::find_cli_by_session_id:
 * @session_id: The CLI session ID to search for.
 * 
 * Find a CLI client #CliClient for a given session ID.
 * 
 * Return value: The CLI client with session ID of @session_id on success,
 * otherwise NULL.
 **/
CliClient *
CliNode::find_cli_by_session_id(uint32_t session_id) const
{
	list<CliClient *>::const_iterator iter;

	for (iter = _client_list.begin();
			iter != _client_list.end();
			++ iter) 
	{
		CliClient *cli_client = *iter;
		if (cli_client->cli_session_session_id() == session_id)
			return (cli_client);
	}

	return (NULL);
}

/**
 * CliNode::xlog_output:
 * @obj: The #CliClient object to apply this function to.
 * @msg: A C-style string with the message to output.
 * 
 * Output a log message to a #CliClient object.
 * 
 * Return value: On success, the number of characters printed,
 * otherwise %XORP_ERROR.
 **/
	int
CliNode::xlog_output(void *obj, xlog_level_t level, const char *msg)
{
	CliClient *cli_client = static_cast<CliClient *>(obj);

	int ret_value = cli_client->cli_print(msg);
	if (ret_value >= 0 && 
			cli_client->cli_print("") >= 0 && cli_client->cli_flush() == 0) { 
		return ret_value;
	}
	return  XORP_ERROR;
	UNUSED(level);
}

//
// CLI add_cli_command
//
int
CliNode::add_cli_command(
		// Input values,
		const string&	processor_name,
		const string&	command_name,
		const string&	command_help,
		const bool&		is_command_cd,
		const string&	command_cd_prompt,
		const bool&		is_command_processor,
		// Output values,
		string&		error_msg)
{
	// Reset the error message
	error_msg = "";

	//
	// Check the request
	//
	if (command_name.empty()) 
	{
		error_msg = "ERROR: command name is empty";
		return (XORP_ERROR);
	}

	CliCommand *c0 = cli_command_root();
	CliCommand *c1 = NULL;

	if (! is_command_processor) 
	{
		if (is_command_cd) 
		{
			c1 = c0->add_command(command_name, command_help,
					command_cd_prompt, true, error_msg);
		} else 
		{
			c1 = c0->add_command(command_name, command_help, true, error_msg);
		}
	} else 
	{
		// Command processor
		c1 = c0->add_command(command_name, command_help, true,
				callback(this, &CliNode::send_process_command),
				error_msg);
		if (c1 != NULL)
			c1->set_can_pipe(true);
	}

	//
	// TODO: return the result of the command installation
	//

	if (c1 == NULL) 
	{
		error_msg = c_format("Cannot install command '%s': %s",
				command_name.c_str(),
				error_msg.c_str());
		return (XORP_ERROR);
	}

	c1->set_global_name(token_line2vector(command_name));
	c1->set_server_name(processor_name);

	return (XORP_OK);
}

//
// CLI delete_cli_command
//
int
CliNode::delete_cli_command(
		// Input values,
		const string&	processor_name,
		const string&	command_name,
		// Output values,
		string&		error_msg)
{
	// Reset the error message
	error_msg = "";

	//
	// Check the request
	//
	if (command_name.empty()) 
	{
		error_msg = "ERROR: command name is empty";
		return (XORP_ERROR);
	}

	CliCommand *c0 = cli_command_root();

	if (c0->delete_command(command_name) != XORP_OK) 
	{
		error_msg = c_format("Cannot delete command '%s'",
				command_name.c_str());
		return (XORP_ERROR);
	}

	UNUSED(processor_name);

	return (XORP_OK);
}

//
// Send a command request to a remote node
//
	int
CliNode::send_process_command(const string& server_name,
		const string& cli_term_name,
		uint32_t cli_session_id,
		const vector<string>& command_global_name,
		const vector<string>& argv)
{
	if (server_name.empty())
		return (XORP_ERROR);
	if (cli_term_name.empty())
		return (XORP_ERROR);
	if (command_global_name.empty())
		return (XORP_ERROR);

	CliClient *cli_client = find_cli_by_session_id(cli_session_id);
	if (cli_client == NULL)
		return (XORP_ERROR);
	if (cli_client != find_cli_by_term_name(cli_term_name))
		return (XORP_ERROR);

	//
	// Send the request
	//
	if (! _send_process_command_callback.is_empty()) 
	{
		(_send_process_command_callback)->dispatch(server_name,
				server_name,
				cli_term_name,
				cli_session_id,
				command_global_name,
				argv);
	}

	cli_client->set_is_waiting_for_data(true);

	return (XORP_OK);
}

//
// Process the response of a command processed by a remote node
//
	void
CliNode::recv_process_command_output(const string * , // processor_name,
		const string *cli_term_name,
		const uint32_t *cli_session_id,
		const string *command_output)
{
	//
	// Find if a client is waiting for that command
	//
	if ((cli_term_name == NULL) || (cli_session_id == NULL))
		return;
	CliClient *cli_client = find_cli_by_session_id(*cli_session_id);
	if (cli_client == NULL)
		return;
	if (cli_client != find_cli_by_term_name(*cli_term_name))
		return;

	if (! cli_client->is_waiting_for_data()) 
	{
		// ERROR: client is not waiting for that data (e.g., probably too late)
		return;
	}

	//
	// Print the result and reset client status
	//
	if (command_output != NULL) 
	{
		cli_client->cli_print(c_format("%s", command_output->c_str()));
	}
	cli_client->cli_flush();
	cli_client->set_is_waiting_for_data(false);
	cli_client->post_process_command();
}

	CliClient *
CliNode::add_client(XorpFd input_fd, XorpFd output_fd, bool is_network,
		const string& startup_cli_prompt, string& error_msg)
{
	return (add_connection(input_fd, output_fd, is_network,
				startup_cli_prompt, error_msg));
}

	int
CliNode::remove_client(CliClient *cli_client, string& error_msg)
{
	if (delete_connection(cli_client, error_msg) != XORP_OK)
		return (XORP_ERROR);

	// XXX: Remove the client itself if it is still around
	list<CliClient *>::iterator iter;
	iter = find(_client_list.begin(), _client_list.end(), cli_client);
	if (iter != _client_list.end()) 
	{
		_client_list.erase(iter);
	}

	return (XORP_OK);
}
