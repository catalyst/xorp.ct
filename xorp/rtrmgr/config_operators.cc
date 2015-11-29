// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "rtrmgr_error.hh"
#include "config_operators.hh"


    string
operator_to_str(ConfigOperator op)
{
    switch (op) 
    {
	case OP_NONE:
	    XLOG_UNREACHABLE();
	case OP_EQ:
	    return string("==");
	case OP_NE:
	    return string("!=");
	case OP_LT:
	    return string("<");
	case OP_LTE:
	    return string("<=");
	case OP_GT:
	    return string(">");
	case OP_GTE:
	    return string(">=");
	case OP_IPNET_EQ:
	    return string("exact");
	case OP_IPNET_NE:
	    return string("not");
	case OP_IPNET_LT:
	    return string("longer");
	case OP_IPNET_GT:
	    return string("shorter");
	case OP_IPNET_LE:
	    return string("orlonger");
	case OP_IPNET_GE:
	    return string("orshorter");
	case OP_ASSIGN:
	    return string(":");
	case OP_ADD:
	    return string("+");
	case OP_ADD_EQ:
	    return string("+=");
	case OP_SUB:
	    return string("-");
	case OP_SUB_EQ:
	    return string("-=");
	case OP_MUL:
	    return string("*");
	case OP_MUL_EQ:
	    return string("*=");
	case OP_DIV:
	    return string("/");
	case OP_DIV_EQ:
	    return string("/=");
	case OP_LSHIFT:
	    return string("<<");
	case OP_LSHIFT_EQ:
	    return string("<<=");
	case OP_RSHIFT:
	    return string(">>");
	case OP_RSHIFT_EQ:
	    return string(">>=");
	case OP_BITAND:
	    return string("&");
	case OP_BITAND_EQ:
	    return string("&=");
	case OP_BITOR:
	    return string("|");
	case OP_BITOR_EQ:
	    return string("|=");
	case OP_BITXOR:
	    return string("^");
	case OP_BITXOR_EQ:
	    return string("^=");
	case OP_DEL:
	    return string("del");
    }
    XLOG_UNREACHABLE();
}

    ConfigOperator
lookup_operator(const string& s) throw (ParseError)
{
    if (s == "==") 
    {
	return OP_EQ;
    } else if (s == "!=") 
    {
	return OP_NE;
    } else if (s == "<") 
    {
	return OP_LT;
    } else if (s == "<=") 
    {
	return OP_LTE;
    } else if (s == ">") 
    {
	return OP_GT;
    } else if (s == ">=") 
    {
	return OP_GTE;
    } else if (s == "exact") 
    {
	return OP_IPNET_EQ;
    } else if (s == "not") 
    {
	return OP_IPNET_NE;
    } else if (s == "longer") 
    {
	return OP_IPNET_LT;
    } else if (s == "shorter") 
    {
	return OP_IPNET_GT;
    } else if (s == "orlonger") 
    {
	return OP_IPNET_LE;
    } else if (s == "orshorter") 
    {
	return OP_IPNET_GE;
    } else if (s == ":") 
    {
	return OP_ASSIGN;
    } else if (s == "=") 
    {
	return OP_ASSIGN;
    } else if (s == "+") 
    {
	return OP_ADD;
    } else if (s == "add" || s == "+=") 
    {
	return OP_ADD_EQ;
    } else if (s == "-") 
    {
	return OP_SUB;
    } else if (s == "sub" || s == "-=") 
    {
	return OP_SUB_EQ;
    } else if (s == "*") 
    {
	return OP_MUL;
    } else if (s == "mul" || s == "*=") 
    {
	return OP_MUL_EQ;
    } else if (s == "/") 
    {
	return OP_DIV;
    } else if (s == "div" || s == "/=") 
    {
	return OP_DIV_EQ;
    } else if (s == "<<") 
    {
	return OP_LSHIFT;
    } else if (s == "lshift" || s == "<<=") 
    {
	return OP_LSHIFT_EQ;
    } else if (s == ">>") 
    {
	return OP_RSHIFT;
    } else if (s == "rshift" || s == ">>=") 
    {
	return OP_RSHIFT_EQ;
    } else if (s == "&") 
    {
	return OP_BITAND;
    } else if (s == "bit_and" || s == "&=") 
    {
	return OP_BITAND_EQ;
    } else if (s == "|") 
    {
	return OP_BITOR;
    } else if (s == "bit_or" || s == "|=") 
    {
	return OP_BITOR_EQ;
    } else if (s == "^") 
    {
	return OP_BITXOR;
    } else if (s == "bit_xor" || s == "^=") 
    {
	return OP_BITXOR_EQ;
    } else if (s == "del") 
    {
	return OP_DEL;
    }

    //
    // Invalid operator string
    //
    string error_msg = c_format("Bad operator: %s", s.c_str());
    xorp_throw(ParseError, error_msg);
}
