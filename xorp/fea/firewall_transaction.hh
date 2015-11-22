// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2008-2009 XORP, Inc.
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

// $XORP: xorp/fea/firewall_transaction.hh,v 1.5 2008/10/02 21:56:47 bms Exp $

#ifndef __FEA_FIREWALL_TRANSACTION_HH__
#define __FEA_FIREWALL_TRANSACTION_HH__

#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/transaction.hh"

#include "firewall_manager.hh"


//
// Firewall transactions.
//

/**
 * @short A class to store and execute firewall transactions.
 *
 * A firewall transaction is a sequence of commands that should
 * executed atomically.
 */
class FirewallTransactionManager : public TransactionManager {
public:
    /**
     * Constructor.
     *
     * @see FirewallManager.
     */
    FirewallTransactionManager()
	: TransactionManager( TIMEOUT_MS, MAX_PENDING)
    {}

    /**
     * Get the string with the first error during commit.
     *
     * @return the string with the first error during commit or an empty
     * string if no error.
     */
    const string& error() const 	{ return _first_error; }

protected:
    /**
     * Pre-commit method that is called before the first operation
     * in a commit.
     *
     * This is an overriding method.
     *
     * @param tid the transaction ID.
     */
    virtual void pre_commit(uint32_t tid);

    /**
     * Method that is called after each operation.
     *
     * This is an overriding method.
     *
     * @param success set to true if the operation succeeded, otherwise false.
     * @param op the operation that has been just called.
     */
    virtual void operation_result(bool success,
				  const TransactionOperation& op);

private:
    /**
     * Reset the string with the error.
     */
    void reset_error()			{ _first_error.erase(); }

    string	_first_error;		// The string with the first error
    uint32_t	_tid_exec;		// The transaction ID

    enum { TIMEOUT_MS = 5000, MAX_PENDING = 10 };
};

/**
 * Base class for operations that can occur during a firewall transaction.
 */
class FirewallTransactionOperation : public TransactionOperation {
public:
    FirewallTransactionOperation(FirewallManager& firewall_manager)
	: _firewall_manager(firewall_manager) {}
    virtual ~FirewallTransactionOperation() {}

    /**
     * Get the error message with the reason for a failure.
     *
     * @return the error message with the reason for a failure or
     * an empty string if no failure.
     */
    const string& error_reason() const { return (_error_reason); }

protected:
    FirewallManager& firewall_manager() { return _firewall_manager; }

    string	_error_reason;		// The reason for a failure

private:
    FirewallManager& _firewall_manager;
};

class FirewallAddEntry4 : public FirewallTransactionOperation {
public:
    FirewallAddEntry4(FirewallManager&	firewall_manager,
		      FirewallEntry&	firewall_entry)
	: FirewallTransactionOperation(firewall_manager),
	  _entry(firewall_entry)
	  {}

    bool dispatch() {
	if (firewall_manager().add_entry(_entry, _error_reason) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("AddEntry4: %s", _entry.str().c_str());
    }

private:
    FirewallEntry _entry;
};

class FirewallReplaceEntry4 : public FirewallTransactionOperation {
public:
    FirewallReplaceEntry4(FirewallManager&	firewall_manager,
			  FirewallEntry&	firewall_entry)
	: FirewallTransactionOperation(firewall_manager),
	  _entry(firewall_entry)
	  {}

    bool dispatch() {
	if (firewall_manager().replace_entry(_entry, _error_reason) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("ReplaceEntry4: %s", _entry.str().c_str());
    }

private:
    FirewallEntry _entry;
};

class FirewallDeleteEntry4 : public FirewallTransactionOperation {
public:
    FirewallDeleteEntry4(FirewallManager&	firewall_manager,
			 FirewallEntry&		firewall_entry)
	: FirewallTransactionOperation(firewall_manager),
	  _entry(firewall_entry)
	  {}

    bool dispatch() {
	if (firewall_manager().delete_entry(_entry, _error_reason) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("DeleteEntry4: %s", _entry.str().c_str());
    }

private:
    FirewallEntry _entry;
};

class FirewallDeleteAllEntries4 : public FirewallTransactionOperation {
public:
    FirewallDeleteAllEntries4(FirewallManager& firewall_manager)
	: FirewallTransactionOperation(firewall_manager)
    {}

    bool dispatch() {
	if (firewall_manager().delete_all_entries4(_error_reason) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return c_format("DeleteAllEntries4"); }
};

class FirewallAddEntry6 : public FirewallTransactionOperation {
public:
    FirewallAddEntry6(FirewallManager&	firewall_manager,
		      FirewallEntry&	firewall_entry)
	: FirewallTransactionOperation(firewall_manager),
	  _entry(firewall_entry)
	  {}

    bool dispatch() {
	if (firewall_manager().add_entry(_entry, _error_reason) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("AddEntry6: %s", _entry.str().c_str());
    }

private:
    FirewallEntry _entry;
};

class FirewallReplaceEntry6 : public FirewallTransactionOperation {
public:
    FirewallReplaceEntry6(FirewallManager&	firewall_manager,
			  FirewallEntry&	firewall_entry)
	: FirewallTransactionOperation(firewall_manager),
	  _entry(firewall_entry)
	  {}

    bool dispatch() {
	if (firewall_manager().replace_entry(_entry, _error_reason) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("ReplaceEntry6: %s", _entry.str().c_str());
    }

private:
    FirewallEntry _entry;
};

class FirewallDeleteEntry6 : public FirewallTransactionOperation {
public:
    FirewallDeleteEntry6(FirewallManager&	firewall_manager,
			 FirewallEntry&		firewall_entry)
	: FirewallTransactionOperation(firewall_manager),
	  _entry(firewall_entry)
	  {}

    bool dispatch() {
	if (firewall_manager().delete_entry(_entry, _error_reason) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("DeleteEntry6: %s", _entry.str().c_str());
    }

private:
    FirewallEntry _entry;
};

class FirewallDeleteAllEntries6 : public FirewallTransactionOperation {
public:
    FirewallDeleteAllEntries6(FirewallManager& firewall_manager)
	: FirewallTransactionOperation(firewall_manager)
    {}

    bool dispatch() {
	if (firewall_manager().delete_all_entries6(_error_reason) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return c_format("DeleteAllEntries6"); }
};

#endif // __FEA_FIREWALL_TRANSACTION_HH__
