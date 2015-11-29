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

// $XORP: xorp/cli/cli_client.hh,v 1.38 2008/10/02 21:56:29 bms Exp $


#ifndef __CLI_CLI_CLIENT_HH__
#define __CLI_CLI_CLIENT_HH__

//
// CLI client definition.
//

#include "libxorp/xorp.h"
#include "libxorp/buffer.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "cli_node.hh"




//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class CliNode;
class CliCommand;

/**
 * @short The class for the CLI client.
 * 
 * There is one CLI client per CLI user (e.g., telnet connection).
 */
class CliClient 
{
	public:
		/**
		 * Constructor for a given CLI node and file descriptor.
		 * 
		 * @param init_cli_node the @ref CliNode CLI node this client belongs to.
		 * @param input_fd the file descriptor for the CLI client to read
		 * data from.
		 * @param output_fd the file descriptor for the CLI client to write
		 * data to.
		 * @param startup_cli_prompt the startup CLI prompt.
		 */
		CliClient(CliNode& init_cli_node, XorpFd input_fd, XorpFd output_fd,
				const string& startup_cli_prompt);

		/**
		 * Destructor
		 */
		virtual ~CliClient();

		/**
		 * Test if the processing of any pending commands or pending data has
		 * complated.
		 * 
		 * @return true if processing of any pending commands or pending data
		 * has completed, otherwise false.
		 */
		bool done() const;

		/**
		 * Start the connection.
		 * 
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		int		start_connection(string& error_msg);

		/**
		 * Stop the connection.
		 * 
		 * @param error_msg the error message (if error).
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		int		stop_connection(string& error_msg);

		/**
		 * Print a message to the CLI user.
		 * 
		 * @param msg C++ string with the message to display.
		 * @return the number of characters printed (not including
		 * the trailing '\0').
		 */
		int		cli_print(const string& msg);

		/**
		 * Flush the CLI output.
		 * 
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		int		cli_flush();

		/**
		 * Add/remove this client for displaying log messages.
		 * 
		 * @param v if true, add this client for displaying messages, otherwise
		 * remove it.
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		int		set_log_output(bool v);

		/**
		 * Test if this client has been added for displaying log messages.
		 * 
		 * @return true if this client has been added for displaying log messages,
		 * otherwise false.
		 */
		bool	is_log_output() const { return (_is_log_output); }

		/**
		 * Get the file descriptor for reading the input from the user.
		 * 
		 * @return the file descriptor for reading the input from the user.
		 */
		XorpFd	input_fd() { return (_input_fd); }

		/**
		 * Get the file descriptor for writing the output to the user.
		 * 
		 * @return the file descriptor for writing the output to the user.
		 */
		XorpFd	output_fd() { return (_output_fd); }

		/**
		 * Test if this client is associated with a terminal type input device.
		 * 
		 * @return true if this client is associated with a terminal type input
		 * device, otherwise false.
		 */
		bool	is_input_tty() const;

		/**
		 * Test if this client is associated with a terminal type output device.
		 * 
		 * @return true if this client is associated with a terminal type output
		 * device, otherwise false.
		 */
		bool	is_output_tty() const;

		/**
		 * Test if this client is associated with a network connection.
		 * 
		 * @return true if this client is associated with a network connection,
		 * otherwise false.
		 */
		bool	is_network() const;

		/**
		 * Set the flag that associates the client with a network connection.
		 * 
		 * @param v true if this client is associated with a network connection.
		 */
		void	set_network_client(bool v);

		/**
		 * Test if this client is associated with a telnet connection.
		 * 
		 * @return true if this client is associated with a telnet connection,
		 * otherwise false.
		 */
		bool	is_telnet() const;

		/**
		 * Test if this client is running in interactive mode.
		 * 
		 * @return true if this client is running in interactive mode,
		 * otherwise false.
		 */
		bool	is_interactive() const;

		//
		// Session info
		//
		/**
		 * Get the user name.
		 * 
		 * @return the user name.
		 */
		const string& cli_session_user_name() const 
		{
			return (_cli_session_user_name);
		}

		/**
		 * Get the terminal name.
		 * 
		 * @return the terminal name.
		 */
		const string& cli_session_term_name() const 
		{
			return (_cli_session_term_name);
		}

		/*
		 * Get the user network address.
		 * 
		 * @return the user network address.
		 */
		const IPvX&	cli_session_from_address() const 
		{
			return (_cli_session_from_address);
		}

		/**
		 * Get the start time for this connection.
		 * 
		 * @return a reference to @ref TimeVal with the starting time for
		 * this connection.
		 */
		const TimeVal& cli_session_start_time() const 
		{
			return (_cli_session_start_time);
		}

		/**
		 * Get the stop time for this connection.
		 * 
		 * @return a reference to @ref TimeVal with the end time for
		 * this connection.
		 */
		const TimeVal& cli_session_stop_time() const 
		{
			return (_cli_session_stop_time);
		}

		/**
		 * Test if the CLI session is active (i.e., it has been setup and ready
		 * to use).
		 * 
		 * @return true if the CLI session is active, otherwise false.
		 */
		bool	is_cli_session_active() const 
		{
			return (_is_cli_session_active);
		}

		/**
		 * Get the session ID.
		 * 
		 * @return the session ID for this client.
		 */
		uint32_t	cli_session_session_id() const 
		{
			return (_cli_session_session_id);
		}

		/**
		 * Set the user name.
		 * 
		 * @param v the user name to set.
		 */
		void	set_cli_session_user_name(const string& v) 
		{
			_cli_session_user_name = v;
		}

		/**
		 * Set the terminal name.
		 * 
		 * @param v the terminal name to set.
		 */
		void	set_cli_session_term_name(const string& v) 
		{
			_cli_session_term_name = v;
		}

		/**
		 * Set the user network address.
		 * 
		 * @param v the user network address to set.
		 */
		void	set_cli_session_from_address(const IPvX& v) 
		{
			_cli_session_from_address = v;
		}

		/**
		 * Set the start time for this connection.
		 * 
		 * @param v the start time for this connection.
		 */
		void	set_cli_session_start_time(const TimeVal& v) 
		{
			_cli_session_start_time = v;
		}

		/**
		 * Set the stop time for this connection.
		 * 
		 * @param v the stop time for this connection.
		 */
		void	set_cli_session_stop_time(const TimeVal& v) 
		{
			_cli_session_stop_time = v;
		}

		/**
		 * Set if this session is active.
		 * 
		 * @param v if true, the session is set as active, otherwise is
		 * set as non-active.
		 */
		void	set_is_cli_session_active(bool v) 
		{
			_is_cli_session_active = v;
		}

		/**
		 * Set the session ID.
		 * 
		 * @param v the session ID value to set.
		 */
		void	set_cli_session_session_id(uint32_t v) 
		{
			_cli_session_session_id = v;
		}

		/**
		 * Perform final processing of a command.
		 */
		void post_process_command();

		/**
		 * Flush the output of a command while it is still running
		 */
		void flush_process_command_output();

		//
		// Server communication state
		//
		/**
		 * Test if waiting for data from command processor.
		 * 
		 * @return true if waiting for data from command processor, otherwise
		 * false.
		 */
		bool is_waiting_for_data() const { return (_is_waiting_for_data); }

		/**
		 * Set a flag whether is waiting for data from command processor.
		 * 
		 * @param v if true, the session is set as waiting for data, otherwise
		 * is set as non-waiting.
		 */
		void set_is_waiting_for_data(bool v);

		/**
		 * Get the current CLI prompt.
		 * 
		 * @return the current CLI prompt.
		 */
		const string& current_cli_prompt() const { return (_current_cli_prompt); }

		/**
		 * Set the current CLI prompt.
		 * 
		 * @param cli_prompt the current CLI prompt to set.
		 */
		void	set_current_cli_prompt(const string& cli_prompt);

		/**
		 * Update the terminal-related information after it has been resized.
		 */
		void	terminal_resized();

	private:
		friend class CliPipe;

		CliNode&	cli_node() { return (_cli_node); }

		GetLine	*gl() { return (_gl); }

		/**
		 * Process octet that may be part of a telnet option.
		 *
		 * @param val the value of the next octet.
		 * @param is_telnet_option return-by-reference result: if true,
		 * then @ref val is part of a telnet option and it was processed.
		 * If it is false, then @ref value should be processed as input data.
		 * @return XORP_OK on success, otherwise XORP_ERROR.
		 */
		int		process_telnet_option(int val, bool& is_telnet_option);

		bool	telnet_iac() { return (_telnet_iac); }
		void	set_telnet_iac(bool v) { _telnet_iac = v; }
		bool	telnet_sb() { return (_telnet_sb); }
		void	set_telnet_sb(bool v) {_telnet_sb = v; }
		bool	telnet_dont() { return (_telnet_dont); }
		void	set_telnet_dont(bool v) { _telnet_dont = v; }
		bool	telnet_do() { return (_telnet_do); }
		void	set_telnet_do(bool v) { _telnet_do = v; }
		bool	telnet_wont() { return (_telnet_wont); }
		void	set_telnet_wont(bool v) { _telnet_wont = v; }
		bool	telnet_will() { return (_telnet_will); }
		void	set_telnet_will(bool v) { _telnet_will = v; }
		bool	telnet_binary() { return (_telnet_binary); }
		void	set_telnet_binary(bool v) { _telnet_binary = v; }

		void	update_terminal_size();

		size_t	window_width() { return ((size_t)_window_width); }
		void	set_window_width(uint16_t v) { _window_width = v; }
		size_t	window_height() { return ((size_t)_window_height); }
		void	set_window_height(uint16_t v) { _window_height = v; }

		Buffer&	command_buffer() { return (_command_buffer); }
		Buffer&	telnet_sb_buffer() { return (_telnet_sb_buffer); }


		CliCommand	*cli_command_root() { return (cli_node().cli_command_root());}
		int		buff_curpos() { return (_buff_curpos); }
		void	set_buff_curpos(int v) { _buff_curpos = v; }

		CliCommand	*current_cli_command() { return (_current_cli_command); }
		void	set_current_cli_command(CliCommand *v);

		int		process_command(const string& command_line);
		void	interrupt_command();

		CliPipe	*add_pipe(const string& pipe_name);
		CliPipe	*add_pipe(const string& pipe_name,
				const list<string>& args_list);
		void	delete_pipe_all();
		bool	is_pipe_mode() { return (_is_pipe_mode); }
		void	set_pipe_mode(bool v) { _is_pipe_mode = v; }

		int		block_connection(bool is_blocked);
		void	client_read(XorpFd fd, IoEventType type);
		void	process_input_data();
		void	schedule_process_input_data();

		static	CPL_MATCH_FN(command_completion_func);
		int		process_char(const string& line, uint8_t val,
				bool& stop_processing);
		int		process_char_page_mode(uint8_t val);
		int		preprocess_char(uint8_t val, bool& stop_processing);
		void	command_line_help(const string& line, int word_end,
				bool remove_last_input_char);
		bool	is_multi_command_prefix(const string& command_line);

		void	process_line_through_pipes(string& pipe_line);
		// Output paging related functions
		bool	is_page_mode() { return (_is_page_mode); }
		void	set_page_mode(bool v);
		bool	is_page_buffer_mode() { return (*_is_page_buffer_mode); }
		void	set_page_buffer_mode(bool v) { *_is_page_buffer_mode = v; }
		vector<string>& page_buffer() { return (*_page_buffer); }
		const string& page_buffer_line(size_t line_n) const;
		void	reset_page_buffer() { page_buffer().clear(); set_page_buffer_last_line_n(0); }
		size_t	page_buffer_lines_n() { return (page_buffer().size()); }
		void	append_page_buffer_line(const string& buffer_line);
		void	concat_page_buffer_line(const string& buffer_line, size_t pos);
		size_t	page_buffer_last_line_n() { return (*_page_buffer_last_line_n); }
		void	set_page_buffer_last_line_n(size_t v) { *_page_buffer_last_line_n = v; }
		void	incr_page_buffer_last_line_n() { (*_page_buffer_last_line_n)++; }
		void	decr_page_buffer_last_line_n() { (*_page_buffer_last_line_n)--; }
		size_t	page_buffer_window_lines_n();
		size_t	page_buffer_last_window_line_n();
		size_t	page_buffer2window_line_n(size_t buffer_line_n);
		size_t	window_lines_n(size_t buffer_line_n);
		size_t	calculate_first_page_buffer_line_by_window_size(
				size_t last_buffer_line_n,
				size_t max_window_size);

		bool	is_help_mode() { return (_is_help_mode); }
		void	set_help_mode(bool v) { _is_help_mode = v; }
		bool	is_nomore_mode() { return (_is_nomore_mode); }
		void	set_nomore_mode(bool v) { _is_nomore_mode = v; }
		bool	is_hold_mode() { return (_is_hold_mode); }
		void	set_hold_mode(bool v) { _is_hold_mode = v; }
		bool	is_prompt_flushed() const { return _is_prompt_flushed; }
		void	set_prompt_flushed(bool v) { _is_prompt_flushed = v; }

		CliNode&	_cli_node;		// The CLI node I belong to
		XorpFd	_input_fd;		// File descriptor to read the input
		XorpFd	_output_fd;		// File descriptor to write the output
		FILE	*_input_fd_file;	// The FILE version of _input_fd
		FILE	*_output_fd_file;	// The FILE version of _output_fd

		enum ClientType
		{
			CLIENT_MIN		= 0,
			CLIENT_TERMINAL		= 0,
			CLIENT_FILE		= 1,
			CLIENT_MAX
		};
		ClientType	_client_type;		// Type of the client: term, file, etc.
		GetLine	*_gl;			// The GetLine for libtecla

		bool	_telnet_iac;		// True if the last octet was IAC
		bool	_telnet_sb;		// True if subnegotiation has began
		bool	_telnet_dont;		// True if the last octet was DONT
		bool	_telnet_do;		// True if the last octet was DO
		bool	_telnet_wont;		// True if the last octet was WONT
		bool	_telnet_will;		// True if the last octet was WILL
		bool	_telnet_binary;		// True if the client "do" binary mode

		uint16_t	_window_width;		// The CLI client window width
		uint16_t	_window_height;		// The CLI client window height
		Buffer	_command_buffer;
		Buffer	_telnet_sb_buffer;

		// The modified terminal flags
		bool	_is_modified_stdio_termios_icanon;
		bool	_is_modified_stdio_termios_echo;
		bool	_is_modified_stdio_termios_isig;
		//
		// The original VMIN and VTIME members of the termios.c_cc[] array.
		// TODO: those should have type cc_t, but we are using 'int' instead
		// to avoid complicated conditional declarations.
		//
		int		_saved_stdio_termios_vmin;
		int		_saved_stdio_termios_vtime;

		// The command we are currently executing and its arguments
		CliCommand	*_executed_cli_command;	// The command currently executed
		vector<string> _executed_cli_command_name;	// The command name
		vector<string> _executed_cli_command_args;	// The arguments

		CliCommand	*_current_cli_command;	// The command we have "cd" to
		string	_current_cli_prompt;	// The CLI prompt
		int		_buff_curpos;		// The cursor position in the buffer

		string	_buffer_line;		// To buffer data when pipe-processing
		list<CliPipe *> _pipe_list;		// A list with the CLI pipe commands
		bool	_is_pipe_mode;		// True if pipe processing enabled
		bool	_is_nomore_mode;	// True if disabled "more" mode
		bool	_is_hold_mode;		// True if enabled "hold" mode

		// Output paging related stuff
		bool	_is_page_mode;		// True if the output is paged

		bool	*_is_page_buffer_mode;	// True if enabled page_buffer_mode
		vector<string> *_page_buffer;
		size_t	*_page_buffer_last_line_n;

		bool	_is_output_buffer_mode;	// True if enabled output_buffer_mode
		vector<string> _output_buffer;	// The output buffer: line per element
		size_t	_output_buffer_last_line_n; // The current last (visable) line

		bool	_is_help_buffer_mode;	// True if enabled help_buffer_mode
		vector<string> _help_buffer;	// The help buffer: line per element
		size_t	_help_buffer_last_line_n; // The current last (visable) line
		bool	_is_help_mode;		// True if enabled help mode

		bool	_is_prompt_flushed;	// True if we have flushed the prompt

		// The strings to save the action names that some keys were bind to
		string	_action_name_up_arrow;
		string	_action_name_down_arrow;
		string	_action_name_tab;
		string	_action_name_newline_n;
		string	_action_name_newline_r;
		string	_action_name_spacebar;
		string	_action_name_ctrl_a;
		string	_action_name_ctrl_b;
		string	_action_name_ctrl_c;
		string	_action_name_ctrl_d;
		string	_action_name_ctrl_e;
		string	_action_name_ctrl_f;
		string	_action_name_ctrl_h;
		string	_action_name_ctrl_k;
		string	_action_name_ctrl_l;
		string	_action_name_ctrl_m;
		string	_action_name_ctrl_n;
		string	_action_name_ctrl_p;
		string	_action_name_ctrl_u;
		string	_action_name_ctrl_x;

		//
		// Session info state
		//
		string	_cli_session_user_name;
		string	_cli_session_term_name;
		IPvX	_cli_session_from_address;
		TimeVal	_cli_session_start_time;
		TimeVal	_cli_session_stop_time;
		bool	_is_cli_session_active;
		uint32_t	_cli_session_session_id;	// The unique session ID.
		bool	_is_network;

		//
		// Log-related state
		//
		bool	_is_log_output;

		//
		// Server communication state
		//
		bool _is_waiting_for_data;		// True if waiting for external data

		//
		// Misc state
		//
		vector<uint8_t>	_pending_input_data;
		XorpTimer		_process_pending_input_data_timer;
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __CLI_CLI_CLIENT_HH__
