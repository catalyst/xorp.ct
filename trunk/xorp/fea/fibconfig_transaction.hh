// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/fibconfig_transaction.hh,v 1.1 2007/04/27 21:11:29 pavlin Exp $

#ifndef __FEA_FIBCONFIG_TRANSACTION_HH__
#define __FEA_FIBCONFIG_TRANSACTION_HH__

#include <map>
#include <list>
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/transaction.hh"

#include "fibconfig.hh"


/**
 * Class to store and execute FibConfig transactions.
 * An FibConfig transaction is a sequence of commands that should
 * executed atomically.
 */
class FibConfigTransactionManager : public TransactionManager
{
public:
    typedef TransactionManager::Operation Operation;

    enum { TIMEOUT_MS = 5000 };

    FibConfigTransactionManager(EventLoop& e, FibConfig& fibconfig,
				uint32_t max_pending = 10)
	: TransactionManager(e, TIMEOUT_MS, max_pending),
	  _fibconfig(fibconfig)
    {}

    FibConfig& fibconfig() { return _fibconfig; }

    /**
     * @return string representing first error during commit.  If string is
     * empty(), then no error occurred.
     */
    const string& error() const { return _error; }

protected:
    void unset_error();
    bool set_unset_error(const string& error);

    // Overriding methods
    void pre_commit(uint32_t tid);
    void post_commit(uint32_t tid);
    void operation_result(bool success, const TransactionOperation& op);

protected:
    FibConfig&	_fibconfig;
    string	_error;
};

/**
 * Base class for operations that can occur during an FibConfig transaction.
 */
class FibConfigTransactionOperation : public TransactionOperation {
public:
    FibConfigTransactionOperation(FibConfig& fibconfig)
	: _fibconfig(fibconfig) {}
    virtual ~FibConfigTransactionOperation() {}

protected:
    FibConfig& fibconfig() { return _fibconfig; }

private:
    FibConfig& _fibconfig;
};

/**
 * Class to store request to add routing entry to FibConfig and
 * dispatch it later.
 */
class FtiAddEntry4 : public FibConfigTransactionOperation {
public:
    FtiAddEntry4(FibConfig&	fibconfig,
		 const IPv4Net&	net,
		 const IPv4&	nexthop,
		 const string&	ifname,
		 const string&	vifname,
		 uint32_t	metric,
		 uint32_t	admin_distance,
		 bool		xorp_route,
		 bool		is_connected_route)
	: FibConfigTransactionOperation(fibconfig),
	  _fte(net, nexthop, ifname, vifname, metric, admin_distance,
	       xorp_route) {
	if (is_connected_route)
	    _fte.mark_connected_route();
    }

    bool dispatch() { return fibconfig().add_entry4(_fte); }

    string str() const { return string("AddEntry4: ") + _fte.str(); }

private:
    Fte4 _fte;
};

/**
 * Class to store request to delete routing entry to FibConfig and
 * dispatch it later.
 */
class FtiDeleteEntry4 : public FibConfigTransactionOperation {
public:
    FtiDeleteEntry4(FibConfig&		fibconfig,
		    const IPv4Net&	net,
		    const IPv4&		nexthop,
		    const string&	ifname,
		    const string&	vifname,
		    uint32_t		metric,
		    uint32_t		admin_distance,
		    bool		xorp_route,
		    bool		is_connected_route)
	: FibConfigTransactionOperation(fibconfig),
	  _fte(net, nexthop, ifname, vifname, metric, admin_distance,
	       xorp_route) {
	if (is_connected_route)
	    _fte.mark_connected_route();
    }

    bool dispatch() { return fibconfig().delete_entry4(_fte); }

    string str() const { return string("DeleteEntry4: ") + _fte.str();  }

private:
    Fte4 _fte;
};

/**
 * Class to store request to delete all routing entries to FibConfig and
 * dispatch it later.
 */
class FtiDeleteAllEntries4 : public FibConfigTransactionOperation {
public:
    FtiDeleteAllEntries4(FibConfig& fibconfig)
	: FibConfigTransactionOperation(fibconfig) {}

    bool dispatch() { return fibconfig().delete_all_entries4(); }

    string str() const { return string("DeleteAllEntries4");  }
};

/**
 * Class to store request to add routing entry to FibConfig and
 * dispatch it later.
 */
class FtiAddEntry6 : public FibConfigTransactionOperation {
public:
    FtiAddEntry6(FibConfig&	fibconfig,
		 const IPv6Net&	net,
		 const IPv6&	nexthop,
		 const string&  ifname,
		 const string&	vifname,
		 uint32_t	metric,
		 uint32_t	admin_distance,
		 bool		xorp_route,
		 bool		is_connected_route)
	: FibConfigTransactionOperation(fibconfig),
	  _fte(net, nexthop, ifname, vifname, metric, admin_distance,
	       xorp_route) {
	if (is_connected_route)
	    _fte.mark_connected_route();
    }

    bool dispatch() { return fibconfig().add_entry6(_fte); }

    string str() const { return string("AddEntry6: ") + _fte.str(); }

private:
    Fte6 _fte;
};

/**
 * Class to store request to delete routing entry to FibConfig
 * and dispatch it later.
 */
class FtiDeleteEntry6 : public FibConfigTransactionOperation {
public:
    FtiDeleteEntry6(FibConfig&		fibconfig,
		    const IPv6Net&	net,
		    const IPv6&		nexthop,
		    const string&	ifname,
		    const string&	vifname,
		    uint32_t		metric,
		    uint32_t		admin_distance,
		    bool		xorp_route,
		    bool		is_connected_route)
	: FibConfigTransactionOperation(fibconfig),
	  _fte(net, nexthop, ifname, vifname, metric, admin_distance,
	       xorp_route) {
	if (is_connected_route)
	    _fte.mark_connected_route();
    }

    bool dispatch() { return fibconfig().delete_entry6(_fte); }

    string str() const { return string("DeleteEntry6: ") + _fte.str();  }

private:
    Fte6 _fte;
};

/**
 * Class to store request to delete all routing entries to FibConfig
 * and dispatch it later.
 */
class FtiDeleteAllEntries6 : public FibConfigTransactionOperation {
public:
    FtiDeleteAllEntries6(FibConfig& fibconfig)
	: FibConfigTransactionOperation(fibconfig) {}

    bool dispatch() { return fibconfig().delete_all_entries6(); }

    string str() const { return string("DeleteAllEntries6");  }
};

#endif // __FEA_FIBCONFIG_TRANSACTION_HH__