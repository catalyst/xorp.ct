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




//
// CLI command "pipe" ("|") implementation.
//


#include "cli_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/token.hh"
#include "libxorp/utils.hh"

#include "cli_client.hh"
#include "cli_command_pipe.hh"


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
static int cli_pipe_dummy_func(const string& server_name,
		const string& cli_term_name,
		uint32_t cli_session_id,
		const vector<string>& command_global_name,
		const vector<string>& argv);


	CliPipe::CliPipe(const string& init_pipe_name)
: CliCommand(NULL, init_pipe_name, CliPipe::name2help(init_pipe_name)),
	_is_running(false),
	_counter(0),
	_bool_flag(false),
	_cli_client(NULL)
{
	_pipe_type = name2pipe_type(init_pipe_name);

	CLI_PROCESS_CALLBACK _cb = callback(cli_pipe_dummy_func);
	set_cli_process_callback(_cb);
	set_can_pipe(true);

	switch (pipe_type()) 
	{
		case CLI_PIPE_COMPARE:
			_start_func_ptr = &CliPipe::pipe_compare_start;
			_stop_func_ptr = &CliPipe::pipe_compare_stop;
			_process_func_ptr = &CliPipe::pipe_compare_process;
			_eof_func_ptr = &CliPipe::pipe_compare_eof;
			break;
		case CLI_PIPE_COMPARE_ROLLBACK:
			_start_func_ptr = &CliPipe::pipe_compare_rollback_start;
			_stop_func_ptr = &CliPipe::pipe_compare_rollback_stop;
			_process_func_ptr = &CliPipe::pipe_compare_rollback_process;
			_eof_func_ptr = &CliPipe::pipe_compare_rollback_eof;
			break;
		case CLI_PIPE_COUNT:
			_start_func_ptr = &CliPipe::pipe_count_start;
			_stop_func_ptr = &CliPipe::pipe_count_stop;
			_process_func_ptr = &CliPipe::pipe_count_process;
			_eof_func_ptr = &CliPipe::pipe_count_eof;
			break;
		case CLI_PIPE_DISPLAY:
			_start_func_ptr = &CliPipe::pipe_display_start;
			_stop_func_ptr = &CliPipe::pipe_display_stop;
			_process_func_ptr = &CliPipe::pipe_display_process;
			_eof_func_ptr = &CliPipe::pipe_display_eof;
			break;
		case CLI_PIPE_DISPLAY_DETAIL:
			_start_func_ptr = &CliPipe::pipe_display_detail_start;
			_stop_func_ptr = &CliPipe::pipe_display_detail_stop;
			_process_func_ptr = &CliPipe::pipe_display_detail_process;
			_eof_func_ptr = &CliPipe::pipe_display_detail_eof;
			break;
		case CLI_PIPE_DISPLAY_INHERITANCE:
			_start_func_ptr = &CliPipe::pipe_display_inheritance_start;
			_stop_func_ptr = &CliPipe::pipe_display_inheritance_stop;
			_process_func_ptr = &CliPipe::pipe_display_inheritance_process;
			_eof_func_ptr = &CliPipe::pipe_display_inheritance_eof;
			break;
		case CLI_PIPE_DISPLAY_XML:
			_start_func_ptr = &CliPipe::pipe_display_xml_start;
			_stop_func_ptr = &CliPipe::pipe_display_xml_stop;
			_process_func_ptr = &CliPipe::pipe_display_xml_process;
			_eof_func_ptr = &CliPipe::pipe_display_xml_eof;
			break;
		case CLI_PIPE_EXCEPT:
			_start_func_ptr = &CliPipe::pipe_except_start;
			_stop_func_ptr = &CliPipe::pipe_except_stop;
			_process_func_ptr = &CliPipe::pipe_except_process;
			_eof_func_ptr = &CliPipe::pipe_except_eof;
			break;
		case CLI_PIPE_FIND:
			_start_func_ptr = &CliPipe::pipe_find_start;
			_stop_func_ptr = &CliPipe::pipe_find_stop;
			_process_func_ptr = &CliPipe::pipe_find_process;
			_eof_func_ptr = &CliPipe::pipe_find_eof;
			break;
		case CLI_PIPE_HOLD:
			_start_func_ptr = &CliPipe::pipe_hold_start;
			_stop_func_ptr = &CliPipe::pipe_hold_stop;
			_process_func_ptr = &CliPipe::pipe_hold_process;
			_eof_func_ptr = &CliPipe::pipe_hold_eof;
			break;
		case CLI_PIPE_MATCH:
			_start_func_ptr = &CliPipe::pipe_match_start;
			_stop_func_ptr = &CliPipe::pipe_match_stop;
			_process_func_ptr = &CliPipe::pipe_match_process;
			_eof_func_ptr = &CliPipe::pipe_match_eof;
			break;
		case CLI_PIPE_NOMORE:
			_start_func_ptr = &CliPipe::pipe_nomore_start;
			_stop_func_ptr = &CliPipe::pipe_nomore_stop;
			_process_func_ptr = &CliPipe::pipe_nomore_process;
			_eof_func_ptr = &CliPipe::pipe_nomore_eof;
			break;
		case CLI_PIPE_RESOLVE:
			_start_func_ptr = &CliPipe::pipe_resolve_start;
			_stop_func_ptr = &CliPipe::pipe_resolve_stop;
			_process_func_ptr = &CliPipe::pipe_resolve_process;
			_eof_func_ptr = &CliPipe::pipe_resolve_eof;
			break;
		case CLI_PIPE_SAVE:
			_start_func_ptr = &CliPipe::pipe_save_start;
			_stop_func_ptr = &CliPipe::pipe_save_stop;
			_process_func_ptr = &CliPipe::pipe_save_process;
			_eof_func_ptr = &CliPipe::pipe_save_eof;
			break;
		case CLI_PIPE_TRIM:
			_start_func_ptr = &CliPipe::pipe_trim_start;
			_stop_func_ptr = &CliPipe::pipe_trim_stop;
			_process_func_ptr = &CliPipe::pipe_trim_process;
			_eof_func_ptr = &CliPipe::pipe_trim_eof;
			break;
		case CLI_PIPE_MAX:
		default:
			_start_func_ptr = &CliPipe::pipe_unknown_start;
			_stop_func_ptr = &CliPipe::pipe_unknown_stop;
			_process_func_ptr = &CliPipe::pipe_unknown_process;
			_eof_func_ptr = &CliPipe::pipe_unknown_eof;
			break;
	}
}

CliPipe::~CliPipe()
{

}

	string
CliPipe::name2help(const string& pipe_name)
{
	switch (name2pipe_type(pipe_name)) 
	{
		case CLI_PIPE_COMPARE:
			return ("Compare configuration changes with a prior version");
		case CLI_PIPE_COMPARE_ROLLBACK:
			return ("Compare configuration changes with a prior rollback version");
		case CLI_PIPE_COUNT:
			return ("Count occurrences");    
		case CLI_PIPE_DISPLAY:
			return ("Display additional configuration information");
		case CLI_PIPE_DISPLAY_DETAIL:
			return ("Display configuration data detail");
		case CLI_PIPE_DISPLAY_INHERITANCE:
			return ("Display inherited configuration data and source group");
		case CLI_PIPE_DISPLAY_XML:
			return ("Display XML content of the command");
		case CLI_PIPE_EXCEPT:
			return ("Show only text that does not match a pattern");
		case CLI_PIPE_FIND:
			return ("Search for the first occurrence of a pattern");
		case CLI_PIPE_HOLD:
			return ("Hold text without exiting the --More-- prompt");
		case CLI_PIPE_MATCH:
			return ("Show only text that matches a pattern");
		case CLI_PIPE_NOMORE:
			return ("Don't paginate output");
		case CLI_PIPE_RESOLVE:
			return ("Resolve IP addresses (NOT IMPLEMENTED YET)");
		case CLI_PIPE_SAVE:
			return ("Save output text to a file (NOT IMPLEMENTED YET)");
		case CLI_PIPE_TRIM:
			return ("Trip specified number of columns from the start line (NOT IMPLEMENTED YET)");
		case CLI_PIPE_MAX:
			// FALLTHROUGH
		default:
			return ("Pipe type unknown");
	}
}

	CliPipe::cli_pipe_t
CliPipe::name2pipe_type(const string& pipe_name)
{
	string token_line = pipe_name;
	string token;

	token = pop_token(token_line);

	if (token.empty())
		return (CLI_PIPE_MAX);

	if (token == "compare")
		return (CLI_PIPE_COMPARE);
	if (token == "count")
		return (CLI_PIPE_COUNT);
	if (token == "display") 
	{
		token = pop_token(token_line);
		if (token.empty())
			return (CLI_PIPE_DISPLAY);
		if (token == "detail")
			return (CLI_PIPE_DISPLAY_DETAIL);
		if (token == "inheritance")
			return (CLI_PIPE_DISPLAY_INHERITANCE);
		if (token == "xml")
			return (CLI_PIPE_DISPLAY_XML);
		return (CLI_PIPE_MAX);
	}
	if (token == "except")
		return (CLI_PIPE_EXCEPT);
	if (token == "find")
		return (CLI_PIPE_FIND);
	if (token == "hold")
		return (CLI_PIPE_HOLD);
	if (token == "match")
		return (CLI_PIPE_MATCH);
	if (token == "no-more")
		return (CLI_PIPE_NOMORE);
	if (token == "resolve")
		return (CLI_PIPE_RESOLVE);
	if (token == "save")
		return (CLI_PIPE_SAVE);
	if (token == "trim")
		return (CLI_PIPE_TRIM);

	return (CLI_PIPE_MAX);
}

	int
CliPipe::pipe_compare_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_compare_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_compare_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_compare_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_compare_rollback_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_compare_rollback_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_compare_rollback_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_compare_rollback_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_count_start(string& input_line, string& error_msg)
{
	_counter = 0;
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_count_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_count_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size()) 
	{
		input_line = "";
		_counter++;
		return (XORP_OK);
	} else 
	{
		return (XORP_ERROR);
	}
}

	int
CliPipe::pipe_count_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	pipe_count_process(input_line);
	input_line += c_format("Count: %d lines\n", _counter);

	return (XORP_OK);
}

	int
CliPipe::pipe_display_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_display_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_detail_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_detail_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_detail_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_display_detail_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_inheritance_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_inheritance_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_inheritance_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_display_inheritance_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_xml_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_xml_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_display_xml_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_display_xml_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_except_start(string& input_line, string& error_msg)
{
	int error_code;
	string arg1;

	if (_pipe_args_list.empty()) 
	{
		error_msg = c_format("missing argument for \"except\" pipe command.");
		return (XORP_ERROR);
	}
	arg1 = _pipe_args_list[0];
	// TODO: check the flags
	error_code = regcomp(&_preg, arg1.c_str(),
			REG_EXTENDED | REG_ICASE | REG_NOSUB | REG_NEWLINE);
	if (error_code != 0) 
	{
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		regerror(error_code, &_preg, buffer, sizeof(buffer));
		error_msg = c_format("error initializing regular expression state: %s.",
				buffer);
		return (XORP_ERROR);
	}

	_is_running = true;

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_except_stop(string& error_msg)
{
	regfree(&_preg);
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_except_process(string& input_line)
{
	int ret_value;

	if (! _is_running)
		return (XORP_ERROR);

	if (! input_line.size())
		return (XORP_ERROR);

	ret_value = regexec(&_preg, input_line.c_str(), 0, NULL, 0);
	if (ret_value == 0) 
	{
		// Match
		input_line = "";
	} else 
	{
		// No-match
	}

	return (XORP_OK);
}

	int
CliPipe::pipe_except_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	regfree(&_preg);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_find_start(string& input_line, string& error_msg)
{
	int error_code;
	string arg1;

	if (_pipe_args_list.empty()) 
	{
		error_msg = c_format("missing argument for \"find\" pipe command.");
		return (XORP_ERROR);
	}
	arg1 = _pipe_args_list[0];
	// TODO: check the flags
	error_code = regcomp(&_preg, arg1.c_str(),
			REG_EXTENDED | REG_ICASE | REG_NOSUB | REG_NEWLINE);
	if (error_code != 0) 
	{
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		regerror(error_code, &_preg, buffer, sizeof(buffer));
		error_msg = c_format("error initializing regular expression state: %s.",
				buffer);
		return (XORP_ERROR);
	}

	_is_running = true;

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_find_stop(string& error_msg)
{
	regfree(&_preg);
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_find_process(string& input_line)
{
	int ret_value;

	if (! _is_running)
		return (XORP_ERROR);

	if (! input_line.size())
		return (XORP_ERROR);

	if (! _bool_flag) 
	{
		// Evaluate the line
		ret_value = regexec(&_preg, input_line.c_str(), 0, NULL, 0);
		if (ret_value == 0) 
		{
			// Match
			_bool_flag = true;
		} else 
		{
			// No-match
		}
	}
	if (! _bool_flag)
		input_line = "";	// Don't print yet

	return (XORP_OK);
}

	int
CliPipe::pipe_find_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	regfree(&_preg);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_hold_start(string& input_line, string& error_msg)
{
	if (_cli_client != NULL) 
	{
		if (_cli_client->is_interactive())
			_cli_client->set_nomore_mode(false);
	}

	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_hold_stop(string& error_msg)
{
	if (_cli_client != NULL) 
	{
		if (_cli_client->is_interactive())
			_cli_client->set_nomore_mode(false); // XXX: default is to hold
	}

	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_hold_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_hold_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (_cli_client != NULL) 
	{
		_cli_client->set_hold_mode(true);
	}

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_match_start(string& input_line, string& error_msg)
{
	int error_code;
	string arg1;

	if (_pipe_args_list.empty()) 
	{
		error_msg = c_format("missing argument for \"match\" pipe command.");
		return (XORP_ERROR);
	}
	arg1 = _pipe_args_list[0];
	// TODO: check the flags
	error_code = regcomp(&_preg, arg1.c_str(),
			REG_EXTENDED | REG_ICASE | REG_NOSUB | REG_NEWLINE);
	if (error_code != 0) 
	{
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		regerror(error_code, &_preg, buffer, sizeof(buffer));
		error_msg = c_format("error initializing regular expression state: %s.",
				buffer);
		return (XORP_ERROR);
	}

	_is_running = true;

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_match_stop(string& error_msg)
{
	regfree(&_preg);
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_match_process(string& input_line)
{
	int ret_value;

	if (! _is_running)
		return (XORP_ERROR);

	if (! input_line.size())
		return (XORP_ERROR);

	ret_value = regexec(&_preg, input_line.c_str(), 0, NULL, 0);
	if (ret_value == 0) 
	{
		// Match
	} else 
	{
		// No-match
		input_line = "";
	}

	return (XORP_OK);
}

	int
CliPipe::pipe_match_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	regfree(&_preg);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_nomore_start(string& input_line, string& error_msg)
{
	if (_cli_client != NULL) 
	{
		_cli_client->set_nomore_mode(true);
	}

	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_nomore_stop(string& error_msg)
{
	if (_cli_client != NULL) 
	{
		if (_cli_client->is_interactive())
			_cli_client->set_nomore_mode(false);
	}
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_nomore_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_nomore_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (_cli_client != NULL) 
	{
		if (_cli_client->is_interactive())
			_cli_client->set_nomore_mode(false);
	}

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_resolve_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_resolve_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_resolve_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	// TODO: implement it
	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_resolve_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_save_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_save_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_save_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	// TODO: implement it
	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_save_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_trim_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_trim_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_trim_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	// TODO: implement it
	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_trim_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

	int
CliPipe::pipe_unknown_start(string& input_line, string& error_msg)
{
	_is_running = true;

	UNUSED(input_line);
	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_unknown_stop(string& error_msg)
{
	_is_running = false;

	UNUSED(error_msg);
	return (XORP_OK);
}

	int
CliPipe::pipe_unknown_process(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	if (input_line.size())
		return (XORP_OK);
	else
		return (XORP_ERROR);
}

	int
CliPipe::pipe_unknown_eof(string& input_line)
{
	if (! _is_running)
		return (XORP_ERROR);

	UNUSED(input_line);
	return (XORP_OK);
}

//
// A dummy function
//
	static int
cli_pipe_dummy_func(const string&		, // server_name,
		const string&		, // cli_term_name
		uint32_t			, // cli_session_id
		const vector<string>&	, // command_global_name
		const vector<string>&	  // argv
		)
{
	return (XORP_OK);
}
