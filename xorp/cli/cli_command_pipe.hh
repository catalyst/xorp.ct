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

// $XORP: xorp/cli/cli_command_pipe.hh,v 1.18 2008/10/02 21:56:29 bms Exp $


#ifndef __CLI_CLI_COMMAND_PIPE_HH__
#define __CLI_CLI_COMMAND_PIPE_HH__


//
// CLI command "pipe" ("|") definition.
//
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif




#ifdef HAVE_REGEX_H
#  include <regex.h>
#else // ! HAVE_REGEX_H
#  ifdef HAVE_PCRE_H
#    include <pcre.h>
#  endif
#  ifdef HAVE_PCREPOSIX_H
#    include <pcreposix.h>
#  endif
#endif // ! HAVE_REGEX_H

#include "cli_command.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

/**
 * @short The class for the "pipe" ("|") command.
 */
class CliPipe : public CliCommand 
{
	public:
		/**
		 * Constructor for a given pipe name.
		 * 
		 * Currently, the list of recognized pipe names are:
		 *  count
		 *  except
		 *  find
		 *  hold
		 *  match
		 *  no-more
		 *  resolve
		 *  save
		 *  trim
		 * 
		 * @param init_pipe_name the pipe name (see above about the list of
		 * recogined pipe names).
		 */
		CliPipe(const string& init_pipe_name);

		/**
		 * Destructor
		 */
		virtual ~CliPipe();

	private:
		friend class CliClient;

		bool is_invalid() { return (_pipe_type == CLI_PIPE_MAX); }
		void add_pipe_arg(const string& v) { _pipe_args_list.push_back(v); }
		void set_cli_client(CliClient *v) { _cli_client = v; }

		int start_func(string& input_line, string& error_msg) { return (this->*_start_func_ptr)(input_line, error_msg); }
		int stop_func(string& error_msg) { return (this->*_stop_func_ptr)(error_msg); }
		int process_func(string& input_line) { return (this->*_process_func_ptr)(input_line); }
		int eof_func(string& input_line) { return (this->*_eof_func_ptr)(input_line); }

		// The "pipe" types
		enum cli_pipe_t 
		{
			CLI_PIPE_COMPARE		= 0,
			CLI_PIPE_COMPARE_ROLLBACK	= 1,
			CLI_PIPE_COUNT			= 2,
			CLI_PIPE_DISPLAY		= 3,
			CLI_PIPE_DISPLAY_DETAIL		= 4,
			CLI_PIPE_DISPLAY_INHERITANCE	= 5,
			CLI_PIPE_DISPLAY_XML		= 6,
			CLI_PIPE_EXCEPT			= 7,
			CLI_PIPE_FIND			= 8,
			CLI_PIPE_HOLD			= 9,
			CLI_PIPE_MATCH			= 10,
			CLI_PIPE_NOMORE			= 11,
			CLI_PIPE_RESOLVE		= 12,
			CLI_PIPE_SAVE			= 13,
			CLI_PIPE_TRIM			= 14,
			CLI_PIPE_MAX
		};
		string name2help(const string& pipe_name);
		cli_pipe_t name2pipe_type(const string& pipe_name);
		cli_pipe_t pipe_type() { return (_pipe_type); }

		// The line processing functions
		typedef int (CliPipe::*StartPipe)(string& input_line, string& error_msg);
		typedef int (CliPipe::*StopPipe)(string& error_msg);
		typedef int (CliPipe::*LineProcess)(string& input_line);
		StartPipe	_start_func_ptr;
		StopPipe	_stop_func_ptr;
		LineProcess	_process_func_ptr;
		LineProcess	_eof_func_ptr;

		int pipe_compare_start(string& input_line, string& error_msg);
		int pipe_compare_stop(string& error_msg);
		int pipe_compare_process(string& input_line);
		int pipe_compare_eof(string& input_line);
		int pipe_compare_rollback_start(string& input_line, string& error_msg);
		int pipe_compare_rollback_stop(string& error_msg);
		int pipe_compare_rollback_process(string& input_line);
		int pipe_compare_rollback_eof(string& input_line);
		int pipe_count_start(string& input_line, string& error_msg);
		int pipe_count_stop(string& error_msg);
		int pipe_count_process(string& input_line);
		int pipe_count_eof(string& input_line);
		int pipe_display_start(string& input_line, string& error_msg);
		int pipe_display_stop(string& error_msg);
		int pipe_display_process(string& input_line);
		int pipe_display_eof(string& input_line);
		int pipe_display_detail_start(string& input_line, string& error_msg);
		int pipe_display_detail_stop(string& error_msg);
		int pipe_display_detail_process(string& input_line);
		int pipe_display_detail_eof(string& input_line);
		int pipe_display_inheritance_start(string& input_line, string& error_msg);
		int pipe_display_inheritance_stop(string& error_msg);
		int pipe_display_inheritance_process(string& input_line);
		int pipe_display_inheritance_eof(string& input_line);
		int pipe_display_xml_start(string& input_line, string& error_msg);
		int pipe_display_xml_stop(string& error_msg);
		int pipe_display_xml_process(string& input_line);
		int pipe_display_xml_eof(string& input_line);
		int pipe_except_start(string& input_line, string& error_msg);
		int pipe_except_stop(string& error_msg);
		int pipe_except_process(string& input_line);
		int pipe_except_eof(string& input_line);
		int pipe_find_start(string& input_line, string& error_msg);
		int pipe_find_stop(string& error_msg);
		int pipe_find_process(string& input_line);
		int pipe_find_eof(string& input_line);
		int pipe_hold_start(string& input_line, string& error_msg);
		int pipe_hold_stop(string& error_msg);
		int pipe_hold_process(string& input_line);
		int pipe_hold_eof(string& input_line);
		int pipe_match_start(string& input_line, string& error_msg);
		int pipe_match_stop(string& error_msg);
		int pipe_match_process(string& input_line);
		int pipe_match_eof(string& input_line);
		int pipe_nomore_start(string& input_line, string& error_msg);
		int pipe_nomore_stop(string& error_msg);
		int pipe_nomore_process(string& input_line);
		int pipe_nomore_eof(string& input_line);
		int pipe_resolve_start(string& input_line, string& error_msg);
		int pipe_resolve_stop(string& error_msg);
		int pipe_resolve_process(string& input_line);
		int pipe_resolve_eof(string& input_line);
		int pipe_save_start(string& input_line, string& error_msg);
		int pipe_save_stop(string& error_msg);
		int pipe_save_process(string& input_line);
		int pipe_save_eof(string& input_line);
		int pipe_trim_start(string& input_line, string& error_msg);
		int pipe_trim_stop(string& error_msg);
		int pipe_trim_process(string& input_line);
		int pipe_trim_eof(string& input_line);
		int pipe_unknown_start(string& input_line, string& error_msg);
		int pipe_unknown_stop(string& error_msg);
		int pipe_unknown_process(string& input_line);
		int pipe_unknown_eof(string& input_line);

		cli_pipe_t		_pipe_type;
		vector<string>	_pipe_args_list; // The arguments for the pipe command
		bool		_is_running;	// True if pipe is running
		int			_counter;	// Internal counter to keep state
		regex_t		_preg;		// Regular expression (internal form)
		bool		_bool_flag;	// Internal bool flag to keep state

		CliClient		*_cli_client;	// The CliClient I belong to, or NULL
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __CLI_CLI_COMMAND_PIPE_HH__
