// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/cli/cli_node_net.cc,v 1.40 2005/07/26 06:49:52 pavlin Exp $"


//
// CLI network-related functions
//


#include "cli_module.h"
#include "libxorp/xorp.h"

#include <errno.h>

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/c_format.hh"
#include "libxorp/token.hh"

#include "libcomm/comm_api.h"

#include "cli_client.hh"
#include "cli_private.hh"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifdef HOST_OS_WINDOWS
#define DEFAULT_TERM_TYPE "ansi-nt"
#define FILENO(x) ((HANDLE)_get_osfhandle(_fileno(x)))
#define FDOPEN(x,y) _fdopen(_open_osfhandle((x),_O_RDWR|_O_TEXT),(y))
#else
#define DEFAULT_TERM_TYPE "vt100"
#define FILENO(x) fileno(x)
#define FDOPEN(x,y) fdopen((x), (y))
#endif

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
 * CliNode::sock_serv_open:
 * 
 * Open a socket for the CLI to listen on for connections.
 * 
 * Return value: The new socket to listen on success, othewise %XORP_ERROR.
 **/
XorpFd
CliNode::sock_serv_open()
{
    // Open the socket
    switch (family()) {
    case AF_INET:
	_cli_socket = comm_bind_tcp4(NULL, _cli_port, COMM_SOCK_NONBLOCKING);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	_cli_socket = comm_bind_tcp6(NULL, _cli_port, COMM_SOCK_NONBLOCKING);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	_cli_socket.clear();
	break;
    }
    
    return (_cli_socket);
}

/**
 * CliNode::sock_serv_close:
 * @: 
 * 
 * Close the socket that is used by the CLI to listen on for connections.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliNode::sock_serv_close()
{
    int ret_value = XORP_OK;

    if (!_cli_socket.is_valid())
	return (XORP_OK);	// Nothing to do

    if (comm_close(_cli_socket) < 0)
	ret_value = XORP_ERROR;

    _cli_socket.clear();

    return (ret_value);
}

void
CliNode::accept_connection(XorpFd fd, IoEventType type)
{
    string error_msg;

    debug_msg("Received connection on socket = %p, family = %d\n",
	      fd.str().c_str(), family());
    
    XLOG_ASSERT(type == IOT_ACCEPT);
    
    XorpFd client_socket = comm_sock_accept(fd);
    
    if (client_socket.is_valid()) {
	if (add_connection(client_socket, client_socket, true, error_msg)
	    == NULL) {
	    XLOG_ERROR("Cannot accept CLI connection: %s", error_msg.c_str());
	}
    }
}

CliClient *
CliNode::add_connection(XorpFd input_fd, XorpFd output_fd, bool is_network,
			string& error_msg)
{
    string dummy_error_msg;
    CliClient *cli_client = NULL;
    
    debug_msg("Added connection with input_fd = %s, output_fd = %s, "
	      "family = %d\n",
	      input_fd.str().c_str(), output_fd.str().c_str(), family());
    
    cli_client = new CliClient(*this, input_fd, output_fd);
    cli_client->set_network_client(is_network);
    _client_list.push_back(cli_client);

#ifdef HOST_OS_WINDOWS
    if (cli_client->is_interactive())
	SetConsoleMode(input_fd, ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
#endif
    
    //
    // Set peer address (for network connection only)
    //
    if (cli_client->is_network()) {
	struct sockaddr_storage ss;
	socklen_t len = sizeof(ss);
	// XXX
	if (getpeername(cli_client->input_fd(), (struct sockaddr *)&ss,
			&len) < 0) {
	    error_msg = c_format("Cannot get peer name");
	    // Error getting peer address
	    delete_connection(cli_client, dummy_error_msg);
	    return (NULL);
	}
	IPvX peer_addr = IPvX::ZERO(family());

	// XXX: The fandango below can go away once the IPvX
	// constructors are fixed to do the right thing.
	switch (ss.ss_family) {
	case AF_INET:
	{
	    struct sockaddr_in *s_in = (struct sockaddr_in *)&ss;
	    peer_addr.copy_in(*s_in);
	}
	break;
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    struct sockaddr_in6 *s_in6 = (struct sockaddr_in6 *)&ss;
	    peer_addr.copy_in(*s_in6);
	}
	break;
#endif // HAVE_IPV6
	default:
	    // Invalid address family
	    error_msg = c_format("Cannot set peer address: "
				 "invalid address family (%d)",
				 ss.ss_family);
	    delete_connection(cli_client, dummy_error_msg);
	    return (NULL);
	}
	cli_client->set_cli_session_from_address(peer_addr);
    }
    
    //
    // Check access control for this peer address
    //
    if (! is_allow_cli_access(cli_client->cli_session_from_address())) {
	error_msg = c_format("CLI access from address %s is not allowed",
			     cli_client->cli_session_from_address().str().c_str());
	delete_connection(cli_client, dummy_error_msg);
	return (NULL);
    }
    
    if (cli_client->start_connection(error_msg) < 0) {
	// Error connecting to the client
	delete_connection(cli_client, dummy_error_msg);
	return (NULL);
    }
    
    //
    // Connection OK
    //
    
    //
    // Set user name
    //
    cli_client->set_cli_session_user_name("guest");	// TODO: get user name
    
    //
    // Set terminal name
    //
    {
	string term_name = "cli_unknown";
	uint32_t i = 0;
	
	for (i = 0; i < CLI_MAX_CONNECTIONS; i++) {
	    term_name = c_format("cli%u", XORP_UINT_CAST(i));
	    if (find_cli_by_term_name(term_name) == NULL)
		break;
	}
	if (i >= CLI_MAX_CONNECTIONS) {
	    // Too many connections
	    error_msg = c_format("Too many CLI connections (max is %u)",
				 XORP_UINT_CAST(CLI_MAX_CONNECTIONS));
	    delete_connection(cli_client, dummy_error_msg);
	    return (NULL);
	}
	cli_client->set_cli_session_term_name(term_name);
    }
    
    //
    // Set session id
    //
    {
	uint32_t session_id = ~0U;	// XXX: ~0U has no particular meaning
	uint32_t i = 0;
	
	for (i = 0; i < CLI_MAX_CONNECTIONS; i++) {
	    session_id = _next_session_id++;
	    if (find_cli_by_session_id(session_id) == NULL)
		break;
	}
	if (i >= CLI_MAX_CONNECTIONS) {
	    // This should not happen: there are available session slots,
	    // but no available session IDs.
	    XLOG_FATAL("Cannot assign CLI session ID");
	    return (NULL);
	}
	cli_client->set_cli_session_session_id(session_id);
    }
    
    //
    // Set session start time
    //
    {
	TimeVal now;
	
	eventloop().current_time(now);
	cli_client->set_cli_session_start_time(now);
    }
    cli_client->set_is_cli_session_active(true);
    
    return (cli_client);
}

int
CliNode::delete_connection(CliClient *cli_client, string& error_msg)
{
    list<CliClient *>::iterator iter;

    iter = find(_client_list.begin(), _client_list.end(), cli_client);
    if (iter == _client_list.end()) {
	error_msg = c_format("Cannot delete CLI connection: invalid client");
	return (XORP_ERROR);
    }

    debug_msg("Delete connection on input fd = %s, output fd = %s, "
	      "family = %d\n",
	      cli_client->input_fd().str().c_str(),
	      cli_client->output_fd().str().c_str(),
	      family());
    cli_client->cli_flush();
    
    // The callback when deleting this client
    if (! _cli_client_delete_callback.is_empty())
	_cli_client_delete_callback->dispatch(cli_client);

    if (cli_client->is_network()) {
	// XXX: delete the client only if this was a network connection
	_client_list.erase(iter);
	delete cli_client;
    }
    
    return (XORP_OK);
}

// XXX: Making this work with Windows in non-blocking mode is a bit crufty.

#ifdef HOST_OS_WINDOWS

// Helper function to process a raw keystroke into ASCII.
int
CliClient::process_key(KEY_EVENT_RECORD &ke)
{
    int ch = ke.uChar.AsciiChar;

    switch (ke.wVirtualKeyCode) {
    case VK_SPACE:
	return (' ');
	break;
    case VK_RETURN:
	return ('\n');
	break;
    case VK_BACK:
	return (0x08);
	break;
    default:
	if (ch >= 1 && ch <= 255)
	return (ch);
	break;
    }
    return (0);
}

// Helper function to return the number of ASCII key-downs in the
// console input buffer on Win32, and optionally put them in
// a user-provided buffer which we hand-off to libtecla.
size_t
CliClient::peek_keydown_events(HANDLE h, char *buffer, int buffer_size)
{
    INPUT_RECORD inr[64];
    DWORD nevents = 0;
    size_t nkeystrokes = 0;
    size_t remaining = buffer_size;
    char *cp = buffer;
    int ch;

    BOOL result = PeekConsoleInputA(h, inr, sizeof(inr)/sizeof(inr[0]),
				    &nevents);
    if (result == FALSE) {
	XLOG_ERROR("PeekConsoleInput() failed: %lu\n", GetLastError());
	return (0);
    }
    if (nevents == 0)
	return (0);

    for (size_t i = 0; i < nevents; i++) {
	if (inr[i].EventType == KEY_EVENT &&
	    inr[i].Event.KeyEvent.bKeyDown == TRUE) {
	    ch = process_key(inr[i].Event.KeyEvent);
	    if (ch != 0) {
		++nkeystrokes;
		// If buffer specified, copy to buffer,
		// otherwise we're just counting what's in the queue.
		if (buffer != NULL) {
		    if (remaining == 0)
			break;
		    else {
			*cp++ = ch;
			--remaining;
		    } /* if */
		} /* if */
	    } /* if */
	} /* if */
    } /* for */

    // Return the number of keystrokes placed into the input buffer.
    return (nkeystrokes);
}

bool
CliClient::poll_conin()
{
    // Peek but don't flush the buffer; only invoke callback if needed.
    // XXX: This gross hack will go away when we have WinDispatcher.
    _keycnt = peek_keydown_events((HANDLE)input_fd(), NULL, 0);
    if (_keycnt > 0)
	client_read(input_fd(), IOT_READ);
    return true;
}
#endif

#ifdef HOST_OS_WINDOWS
#define CONIN_POLL_INTERVAL	250	// milliseconds
#endif

int
CliClient::start_connection(string& error_msg)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(error_msg);
    if (!is_interactive()) {
#endif
    if (cli_node().eventloop().add_ioevent_cb(input_fd(), IOT_READ,
					    callback(this, &CliClient::client_read))
	== false)
	return (XORP_ERROR);
#ifdef HOST_OS_WINDOWS
    } else {
	// stdio, so schedule a periodic timer every 400ms
	// to poll for input.
	// XXX: This will go away when we bring in the new WinDispatcher;
	// we can then merely ask to be signalled by the console handle
	// as for any other system event.
	_poll_conin_timer = cli_node().eventloop().new_periodic(
				CONIN_POLL_INTERVAL,
				callback(this, &CliClient::poll_conin));
    } // !is_interactive
#endif
    
    //
    // Setup the telnet options
    //
    if (is_telnet()) {
	char will_echo_cmd[] = { IAC, WILL, TELOPT_ECHO, '\0' };
	char will_sga_cmd[]  = { IAC, WILL, TELOPT_SGA, '\0'  };
	char dont_linemode_cmd[] = { IAC, DONT, TELOPT_LINEMODE, '\0' };
	char do_window_size_cmd[] = { IAC, DO, TELOPT_NAWS, '\0' };
	char do_transmit_binary_cmd[] = { IAC, DO, TELOPT_BINARY, '\0' };
	char will_transmit_binary_cmd[] = { IAC, WILL, TELOPT_BINARY, '\0' };

	send(input_fd(), will_echo_cmd, sizeof(will_echo_cmd), 0);
	send(input_fd(), will_sga_cmd, sizeof(will_sga_cmd), 0);
	send(input_fd(), dont_linemode_cmd, sizeof(dont_linemode_cmd), 0);
	send(input_fd(), do_window_size_cmd, sizeof(do_window_size_cmd), 0);
	send(input_fd(), do_transmit_binary_cmd, sizeof(do_transmit_binary_cmd), 0);
	send(input_fd(), will_transmit_binary_cmd, sizeof(will_transmit_binary_cmd), 0);
    }
    
#ifdef HAVE_TERMIOS_H
    //
    // Put the terminal in non-canonical and non-echo mode.
    // In addition, disable signals INTR, QUIT, [D]SUSP
    // (i.e., force their value to be received when read from the terminal).
    //
    if (is_output_tty()) {
	struct termios termios;
	
	while (tcgetattr(output_fd(), &termios) != 0) {
	    if (errno != EINTR) {
		error_msg = c_format("start_connection(): "
				     "tcgetattr() error: %s", 
				     strerror(errno));
		return (XORP_ERROR);
	    }
	}
	
	// Save state regarding the modified terminal flags
	if (termios.c_lflag & ICANON)
	    _is_modified_stdio_termios_icanon = true;
	if (termios.c_lflag & ECHO)
	    _is_modified_stdio_termios_echo = true;
	if (termios.c_lflag & ISIG)
	    _is_modified_stdio_termios_isig = true;
	
	// Reset the flags
	termios.c_lflag &= ~(ICANON | ECHO | ISIG);
	while (tcsetattr(output_fd(), TCSADRAIN, &termios) != 0) {
	    if (errno != EINTR) {
		error_msg = c_format("start_connection(): "
				     "tcsetattr() error: %s", 
				     strerror(errno));
		return (XORP_ERROR);
	    }
	}
    }
#endif /* HAVE_TERMIOS_H */
    
    //
    // Setup the read/write file descriptors
    //
    if (input_fd() == XorpFd(FILENO(stdin))) {
	_input_fd_file = stdin;
    } else {
	_input_fd_file = FDOPEN(input_fd(), "r");
	if (_input_fd_file == NULL) {
	    error_msg = c_format("Cannot associate a stream with the "
				 "input file descriptor: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }
    if (output_fd() == XorpFd(FILENO(stdout))) {
	_output_fd_file = stdout;
    } else {
	_output_fd_file = FDOPEN(output_fd(), "w");
	if (_output_fd_file == NULL) {
	    error_msg = c_format("Cannot associate a stream with the "
				 "output file descriptor: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }
    
    _gl = new_GetLine(1024, 2048);		// TODO: hardcoded numbers
    if (_gl == NULL) {
	error_msg = c_format("Cannot create a new GetLine instance");
	return (XORP_ERROR);
    }
    
    // XXX: always set to network type
    gl_set_is_net(_gl, 1);
    
    // Set the terminal
    string term_name = DEFAULT_TERM_TYPE;
    if (is_output_tty()) {
	term_name = getenv("TERM");
	if (term_name.empty())
	    term_name = DEFAULT_TERM_TYPE;	// Set to default
    }

    // XXX: struct winsize is a tty-ism
#if 0
    // Get the terminal size
    if (is_output_tty()) {
	struct winsize window_size;

	if (ioctl(output_fd(), TIOCGWINSZ, &window_size) < 0) {
	    XLOG_ERROR("Cannot get window size (ioctl(TIOCGWINSZ) failed): %s",
		       strerror(errno));
	} else {
	    // Set the window width and height
	    uint16_t new_window_width, new_window_height;

	    new_window_width = window_size.ws_col;
	    new_window_height = window_size.ws_row;

	    if (new_window_width > 0) {
		set_window_width(new_window_width);
	    } else {
		cli_print(c_format("Invalid window width (%u); "
				   "window width unchanged (%u)\n",
				   new_window_width,
				   XORP_UINT_CAST(window_width())));
	    }
	    if (new_window_height > 0) {
		set_window_height(new_window_height);
	    } else {
		cli_print(c_format("Invalid window height (%u); "
				   "window height unchanged (%u)\n",
				   new_window_height,
				   XORP_UINT_CAST(window_height())));
	    }

	    gl_terminal_size(gl(), window_width(), window_height());
	    debug_msg("Client window size changed to width = %u "
		      "height = %u\n",
		      XORP_UINT_CAST(window_width()),
		      XORP_UINT_CAST(window_height()));
	}
    }
#endif

    // Change the input and output streams for libtecla
    if (gl_change_terminal(_gl, _input_fd_file, _output_fd_file,
			   term_name.c_str())
	!= 0) {
	error_msg = c_format("Cannot change the I/O streams");
	_gl = del_GetLine(_gl);
	return (XORP_ERROR);
    }

    // Add the command completion hook
    if (gl_customize_completion(_gl, this, command_completion_func) != 0) {
	error_msg = c_format("Cannot customize command-line completion");
	_gl = del_GetLine(_gl);
	return (XORP_ERROR);
    }

    //
    // Key bindings
    //

    // Bind Ctrl-C to no-op
    gl_configure_getline(_gl, "bind ^C user-event4", NULL, NULL);

    // Bind Ctrl-W to delete the word before the cursor, because
    // the default libtecla behavior is to delete the whole line.
    gl_configure_getline(_gl, "bind ^W backward-delete-word", NULL, NULL);

    // Print the welcome message
    char hostname[MAXHOSTNAMELEN];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
	XLOG_ERROR("gethostname() failed: %s", strerror(errno));
	// XXX: if gethostname() fails, then default to "xorp"
	strncpy(hostname, "xorp", sizeof(hostname) - 1);
    }
    hostname[sizeof(hostname) - 1] = '\0';
    cli_print(c_format("%s%s\n", XORP_CLI_WELCOME, hostname));

    // Show the prompt
    cli_print(current_cli_prompt());

    return (XORP_OK);
}

int
CliClient::stop_connection(string& error_msg)
{
#ifdef HAVE_TERMIOS_H
    //
    // Restore the terminal settings
    //
    if (is_output_tty()) {
	if (! (_is_modified_stdio_termios_icanon
	       || _is_modified_stdio_termios_echo
	       || _is_modified_stdio_termios_isig)) {
	    return (XORP_OK);		// Nothing to restore
	}
	
	struct termios termios;
	
	while (tcgetattr(output_fd(), &termios) != 0) {
	    if (errno != EINTR) {
		XLOG_ERROR("stop_connection(): tcgetattr() error: %s", 
			   strerror(errno));
		return (XORP_ERROR);
	    }
	}
	// Restore the modified terminal flags
	if (_is_modified_stdio_termios_icanon)
	    termios.c_lflag |= ICANON;
	if (_is_modified_stdio_termios_echo)
	    termios.c_lflag |= ECHO;
	if (_is_modified_stdio_termios_isig)
	    termios.c_lflag |= ISIG;
	while (tcsetattr(output_fd(), TCSADRAIN, &termios) != 0) {
	    if (errno != EINTR) {
		error_msg = c_format("stop_connection(): "
				     "tcsetattr() error: %s",
				     strerror(errno));
		return (XORP_ERROR);
	    }
	}
    }
#endif /* HAVE_TERMIOS_H */

    error_msg = "";
    return (XORP_OK);
}

//
// If @v is true, block the client terminal, otherwise unblock it.
//
void
CliClient::set_is_waiting_for_data(bool v)
{
    _is_waiting_for_data = v;
    // block_connection(v);
}

//
// If @is_blocked is true, block the connection (by removing its I/O
// event hook), otherwise add its socket back to the event loop.
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
int
CliClient::block_connection(bool is_blocked)
{
    if (!input_fd().is_valid())
	return (XORP_ERROR);
    
    if (is_blocked) {
	cli_node().eventloop().remove_ioevent_cb(input_fd(), IOT_READ);
	return (XORP_OK);
    }

    if (cli_node().eventloop().add_ioevent_cb(input_fd(), IOT_READ,
					    callback(this, &CliClient::client_read))
	== false)
	return (XORP_ERROR);

    return (XORP_OK);
}

void
CliClient::client_read(XorpFd fd, IoEventType type)
{
    string dummy_error_msg;

    char buf[1024];		// TODO: 1024 size must be #define
    int n;
    
    XLOG_ASSERT(type == IOT_READ);

#ifdef HOST_OS_WINDOWS
    if (!is_interactive()) {
	n = recv(fd, buf, sizeof(buf), 0);
    } else {
	// Read keystrokes into buffer and flush them.
	n = peek_keydown_events((HANDLE)fd, buf, sizeof(buf));
	if (n > 0)
	    FlushConsoleInputBuffer((HANDLE)fd);
    }
#else /* !HOST_OS_WINDOWS */
    n = read(fd, buf, sizeof(buf) - 1);
#endif /* HOST_OS_WINDOWS */
    
    debug_msg("client_read %d octet(s)\n", n);
    if (n <= 0) {
	cli_node().delete_connection(this, dummy_error_msg);
	return;
    }

    // Add the new data to the buffer with the pending data
    size_t old_size = _pending_input_data.size();
    _pending_input_data.resize(old_size + n);
    memcpy(&_pending_input_data[old_size], buf, n);

    process_input_data();
}

void
CliClient::process_input_data()
{
    int ret_value;
    string dummy_error_msg;
    vector<uint8_t> input_data = _pending_input_data;
    bool stop_processing = false;

    //
    // XXX: Remove the stored input data. Later we will add-back
    // only the data which we couldn't process.
    //
    _pending_input_data.clear();

    // Process the input data
    vector<uint8_t>::iterator iter;
    for (iter = input_data.begin(); iter != input_data.end(); ++iter) {
	uint8_t val = *iter;
	bool stop_processing_tmp = false;
	bool save_input = false;
	bool ignore_current_character = false;
	
	if (is_telnet()) {
	    // Filter-out the Telnet commands
	    int ret = process_telnet_option(val);
	    if (ret < 0) {
		// Kick-out the client
		// TODO: print more informative message about the client:
		// E.g. where it came from, etc.
		XLOG_WARNING("Removing client (socket = %s family = %d): "
			     "error processing telnet option",
			     input_fd().str().c_str(),
			     cli_node().family());
		cli_node().delete_connection(this, dummy_error_msg);
		return;
	    }
	    if (ret == 0) {
		// Telnet option processed
		continue;
	    }
	}
	
	preprocess_char(val, stop_processing_tmp, ignore_current_character);
	if (stop_processing_tmp && (! stop_processing)) {
	    stop_processing = true;
	    save_input = true;
	}

	if (val == CHAR_TO_CTRL('c')) {
	    //
	    // Interrupt current command
	    //
	    interrupt_command();
	    _pending_input_data.clear();
	    return;
	}

	if (! stop_processing) {
	    //
	    // Get a character and process it
	    //
	    do {
		char *line;
		line = gl_get_line_net(gl(),
				       current_cli_prompt().c_str(),
				       (char *)command_buffer().data(),
				       buff_curpos(),
				       val);
		ret_value = XORP_ERROR;
		if (line == NULL) {
		    ret_value = XORP_ERROR;
		    break;
		}
		if (is_page_mode()) {
		    ret_value = process_char_page_mode(val);
		    break;
		}
		ret_value = process_char(string(line), val,
					 stop_processing_tmp,
					 ignore_current_character);
		if (stop_processing_tmp && (! stop_processing)) {
		    stop_processing = true;
		    save_input = true;
		}
		break;
	    } while (false);

	    if (ret_value != XORP_OK) {
		// Either error or end of input
		cli_print("\nEnd of connection.\n");
		cli_node().delete_connection(this, dummy_error_msg);
		return;
	    }
	}

	if (save_input) {
	    //
	    // Stop processing and save the remaining input data for later
	    // processing.
	    // However we continue scanning the rest of the data
	    // primarily to look for Ctrl-C input.
	    //
	    vector<uint8_t>::iterator iter2 = iter;
	    if (ignore_current_character)
		++iter2;
	    if (iter2 != input_data.end())
		_pending_input_data.assign(iter2, input_data.end());
	}
    }
    cli_flush();		// Flush-out the output
}

//
// Preprocess a character before 'libtecla' get its hand on it
//
int
CliClient::preprocess_char(uint8_t val, bool& stop_processing,
			   bool& ignore_current_character)
{
    stop_processing = false;
    ignore_current_character = false;

    if ((val == '\n') || (val == '\r')) {
	// New command
	if (is_waiting_for_data())
	    stop_processing = true;

	return (XORP_OK);
    }

    //
    // XXX: Bind/unbind the 'SPACE' to complete-word so it can
    // complete a half-written command.
    // TODO: if the beginning of the line, shall we explicitly unbind as well?
    //
    if (val == ' ') {
	int tmp_buff_curpos = buff_curpos();
	char *tmp_line = (char *)command_buffer().data();
	string command_line = string(tmp_line, tmp_buff_curpos);
	if (! is_multi_command_prefix(command_line)) {
	    // Un-bind the "SPACE" to complete-word
	    // Don't ask why we need six '\' to specify the ASCII value
	    // of 'SPACE'...
	    gl_configure_getline(gl(),
				 "bind \\\\\\040 ",
				 NULL, NULL);
	} else {
	    // Bind the "SPACE" to complete-word
	    gl_configure_getline(gl(),
				 "bind \\\\\\040   complete-word",
				 NULL, NULL);
	}

	return (XORP_OK);
    }
    
    return (XORP_OK);
}

//
// Process octet that may be part of a telnet option.
// Parameter 'val' is the value of the next octet.
// Return -1 if the caller should remove this client.
// Return 0 if @val is part of a telnet option, and the telnet option was
// processed.
// Return 1 if @val is not part of a telnet option and should be processed
// as input data.
//
int
CliClient::process_telnet_option(int val)
{
    if (val == IAC) {
	// Probably a telnet command
	if (! telnet_iac()) {
	    set_telnet_iac(true);
	    return (0);
	}
	set_telnet_iac(false);
    }
    if (telnet_iac()) {
	switch (val) {
	case SB:
	    // Begin subnegotiation of the indicated option.
	    telnet_sb_buffer().reset();
	    set_telnet_sb(true);
	    break;
	case SE:
	    // End subnegotiation of the indicated option.
	    if (! telnet_sb())
		break;
	    switch(telnet_sb_buffer().data(0)) {
	    case TELOPT_NAWS:
		// Telnet Window Size Option
		if (telnet_sb_buffer().data_size() < 5)
		    break;
		{
		    uint16_t new_window_width, new_window_height;
		    
		    new_window_width   = 256*telnet_sb_buffer().data(1);
		    new_window_width  += telnet_sb_buffer().data(2);
		    new_window_height  = 256*telnet_sb_buffer().data(3);
		    new_window_height += telnet_sb_buffer().data(4);

		    if (new_window_width > 0) {
			set_window_width(new_window_width);
		    } else {
			cli_print(c_format("Invalid window width (%u); "
					   "window width unchanged (%u)\n",
					   new_window_width,
					   XORP_UINT_CAST(window_width())));
		    }
		    if (new_window_height > 0) {
			set_window_height(new_window_height);
		    } else {
			cli_print(c_format("Invalid window height (%u); "
					   "window height unchanged (%u)\n",
					   new_window_height,
					   XORP_UINT_CAST(window_height())));
		    }

		    gl_terminal_size(gl(), window_width(), window_height());
		    debug_msg("Client window size changed to width = %u "
			      "height = %u\n",
			      XORP_UINT_CAST(window_width()),
			      XORP_UINT_CAST(window_height()));
		}
		break;
	    default:
		break;
	    }
	    telnet_sb_buffer().reset();
	    set_telnet_sb(false);
	    break;
	case DONT:
	    // you are not to use option
	    set_telnet_dont(true);
	    break;
	case DO:
	    // please, you use option
	    set_telnet_do(true);
	    break;
	case WONT:
	    // I won't use option
	    set_telnet_wont(true);
	    break;
	case WILL:
	    // I will use option
	    set_telnet_will(true);
	    break;
	case GA:
	    // you may reverse the line
	    break;
	case EL:
	    // erase the current line
	    break;
	case EC:
	    // erase the current character
	    break;
	case AYT:
	    // are you there
	    break;
	case AO:
	    // abort output--but let prog finish
	    break;
	case IP:
	    // interrupt process--permanently
	    break;
	case BREAK:
	    // break
	    break;
	case DM:
	    // data mark--for connect. cleaning
	    break;
	case NOP:
	    // nop
	    break;
	case EOR:
	    // end of record (transparent mode)
	    break;
	case ABORT:
	    // Abort process
	    break;
	case SUSP:
	    // Suspend process
	    break;
	case xEOF:
	    // End of file: EOF is already used...
	    break;
	case TELOPT_BINARY:
	    if (telnet_do())
		set_telnet_binary(true);
	    else
		set_telnet_binary(false);
	    break;
	default:
	    break;
	}
	set_telnet_iac(false);
	return (0);
    }
    //
    // Cleanup the telnet options state
    //
    if (telnet_sb()) {
	// A negotiated option value
	if (telnet_sb_buffer().add_data(val) < 0) {
	    // This client is sending too much options. Kick it out!
	    return (XORP_ERROR);
	}
	return (0);
    }
    if (telnet_dont()) {
	// Telnet DONT option code
	set_telnet_dont(false);
	return (0);
    }
    if (telnet_do()) {
	// Telnet DO option code
	set_telnet_do(false);
	return (0);
    }
    if (telnet_wont()) {
	// Telnet WONT option code
	set_telnet_wont(false);
	return (0);
    }
    if (telnet_will()) {
	// Telnet WILL option code
	set_telnet_will(false);
	return (0);
    }
    
    return (1);
}
