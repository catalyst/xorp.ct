/*
 * Copyright (c) 2001-2004 International Computer Science Institute
 * See LICENSE file for licensing, conditions, and warranties on use.
 *
 * DO NOT EDIT THIS FILE - IT IS PROGRAMMATICALLY GENERATED
 *
 * Generated by 'tgt-gen'.
 */

#ident "$XORP: xorp/xrl/targets/fib2mrib_base.cc,v 1.4 2004/06/10 22:42:10 hodson Exp $"


#include "fib2mrib_base.hh"


XrlFib2mribTargetBase::XrlFib2mribTargetBase(XrlCmdMap* cmds)
    : _cmds(cmds)
{
    if (_cmds)
	add_handlers();
}

XrlFib2mribTargetBase::~XrlFib2mribTargetBase()
{
    if (_cmds)
	remove_handlers();
}

bool
XrlFib2mribTargetBase::set_command_map(XrlCmdMap* cmds)
{
    if (_cmds == 0 && cmds) {
        _cmds = cmds;
        add_handlers();
        return true;
    }
    if (_cmds && cmds == 0) {
	remove_handlers();
        _cmds = cmds;
        return true;
    }
    return false;
}

const XrlCmdError
XrlFib2mribTargetBase::handle_common_0_1_get_target_name(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            0, (uint32_t)xa_inputs.size(), "common/0.1/get_target_name");
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string name;
    try {
	XrlCmdError e = common_0_1_get_target_name(
	    name);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "common/0.1/get_target_name", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("name", name);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_common_0_1_get_version(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            0, (uint32_t)xa_inputs.size(), "common/0.1/get_version");
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string version;
    try {
	XrlCmdError e = common_0_1_get_version(
	    version);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "common/0.1/get_version", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("version", version);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_common_0_1_get_status(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            0, (uint32_t)xa_inputs.size(), "common/0.1/get_status");
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    uint32_t status;
    string reason;
    try {
	XrlCmdError e = common_0_1_get_status(
	    status,
	    reason);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "common/0.1/get_status", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("status", status);
	pxa_outputs->add("reason", reason);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_common_0_1_shutdown(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            0, (uint32_t)xa_inputs.size(), "common/0.1/shutdown");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = common_0_1_shutdown();
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "common/0.1/shutdown", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fea_fib_client_0_1_add_route4(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 8) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            8, (uint32_t)xa_inputs.size(), "fea_fib_client/0.1/add_route4");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fea_fib_client_0_1_add_route4(
	    xa_inputs.get_ipv4net("network"),
	    xa_inputs.get_ipv4("nexthop"),
	    xa_inputs.get_string("ifname"),
	    xa_inputs.get_string("vifname"),
	    xa_inputs.get_uint32("metric"),
	    xa_inputs.get_uint32("admin_distance"),
	    xa_inputs.get_string("protocol_origin"),
	    xa_inputs.get_bool("xorp_route"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fea_fib_client/0.1/add_route4", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fea_fib_client_0_1_add_route6(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 8) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            8, (uint32_t)xa_inputs.size(), "fea_fib_client/0.1/add_route6");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fea_fib_client_0_1_add_route6(
	    xa_inputs.get_ipv6net("network"),
	    xa_inputs.get_ipv6("nexthop"),
	    xa_inputs.get_string("ifname"),
	    xa_inputs.get_string("vifname"),
	    xa_inputs.get_uint32("metric"),
	    xa_inputs.get_uint32("admin_distance"),
	    xa_inputs.get_string("protocol_origin"),
	    xa_inputs.get_bool("xorp_route"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fea_fib_client/0.1/add_route6", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fea_fib_client_0_1_replace_route4(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 8) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            8, (uint32_t)xa_inputs.size(), "fea_fib_client/0.1/replace_route4");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fea_fib_client_0_1_replace_route4(
	    xa_inputs.get_ipv4net("network"),
	    xa_inputs.get_ipv4("nexthop"),
	    xa_inputs.get_string("ifname"),
	    xa_inputs.get_string("vifname"),
	    xa_inputs.get_uint32("metric"),
	    xa_inputs.get_uint32("admin_distance"),
	    xa_inputs.get_string("protocol_origin"),
	    xa_inputs.get_bool("xorp_route"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fea_fib_client/0.1/replace_route4", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fea_fib_client_0_1_replace_route6(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 8) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            8, (uint32_t)xa_inputs.size(), "fea_fib_client/0.1/replace_route6");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fea_fib_client_0_1_replace_route6(
	    xa_inputs.get_ipv6net("network"),
	    xa_inputs.get_ipv6("nexthop"),
	    xa_inputs.get_string("ifname"),
	    xa_inputs.get_string("vifname"),
	    xa_inputs.get_uint32("metric"),
	    xa_inputs.get_uint32("admin_distance"),
	    xa_inputs.get_string("protocol_origin"),
	    xa_inputs.get_bool("xorp_route"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fea_fib_client/0.1/replace_route6", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fea_fib_client_0_1_delete_route4(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 3) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            3, (uint32_t)xa_inputs.size(), "fea_fib_client/0.1/delete_route4");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fea_fib_client_0_1_delete_route4(
	    xa_inputs.get_ipv4net("network"),
	    xa_inputs.get_string("ifname"),
	    xa_inputs.get_string("vifname"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fea_fib_client/0.1/delete_route4", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fea_fib_client_0_1_delete_route6(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 3) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            3, (uint32_t)xa_inputs.size(), "fea_fib_client/0.1/delete_route6");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fea_fib_client_0_1_delete_route6(
	    xa_inputs.get_ipv6net("network"),
	    xa_inputs.get_string("ifname"),
	    xa_inputs.get_string("vifname"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fea_fib_client/0.1/delete_route6", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fea_fib_client_0_1_resolve_route4(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            1, (uint32_t)xa_inputs.size(), "fea_fib_client/0.1/resolve_route4");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fea_fib_client_0_1_resolve_route4(
	    xa_inputs.get_ipv4net("network"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fea_fib_client/0.1/resolve_route4", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fea_fib_client_0_1_resolve_route6(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            1, (uint32_t)xa_inputs.size(), "fea_fib_client/0.1/resolve_route6");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fea_fib_client_0_1_resolve_route6(
	    xa_inputs.get_ipv6net("network"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fea_fib_client/0.1/resolve_route6", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fib2mrib_0_1_enable_fib2mrib(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            1, (uint32_t)xa_inputs.size(), "fib2mrib/0.1/enable_fib2mrib");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fib2mrib_0_1_enable_fib2mrib(
	    xa_inputs.get_bool("enable"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fib2mrib/0.1/enable_fib2mrib", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fib2mrib_0_1_start_fib2mrib(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            0, (uint32_t)xa_inputs.size(), "fib2mrib/0.1/start_fib2mrib");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fib2mrib_0_1_start_fib2mrib();
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fib2mrib/0.1/start_fib2mrib", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fib2mrib_0_1_stop_fib2mrib(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            0, (uint32_t)xa_inputs.size(), "fib2mrib/0.1/stop_fib2mrib");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fib2mrib_0_1_stop_fib2mrib();
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fib2mrib/0.1/stop_fib2mrib", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlFib2mribTargetBase::handle_fib2mrib_0_1_enable_log_trace_all(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u) handling %s",
            1, (uint32_t)xa_inputs.size(), "fib2mrib/0.1/enable_log_trace_all");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = fib2mrib_0_1_enable_log_trace_all(
	    xa_inputs.get_bool("enable"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for %s failed: %s",
            		 "fib2mrib/0.1/enable_log_trace_all", e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

void
XrlFib2mribTargetBase::add_handlers()
{
	if (_cmds->add_handler("common/0.1/get_target_name",
	    callback(this, &XrlFib2mribTargetBase::handle_common_0_1_get_target_name)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "common/0.1/get_target_name");
	}
	if (_cmds->add_handler("common/0.1/get_version",
	    callback(this, &XrlFib2mribTargetBase::handle_common_0_1_get_version)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "common/0.1/get_version");
	}
	if (_cmds->add_handler("common/0.1/get_status",
	    callback(this, &XrlFib2mribTargetBase::handle_common_0_1_get_status)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "common/0.1/get_status");
	}
	if (_cmds->add_handler("common/0.1/shutdown",
	    callback(this, &XrlFib2mribTargetBase::handle_common_0_1_shutdown)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "common/0.1/shutdown");
	}
	if (_cmds->add_handler("fea_fib_client/0.1/add_route4",
	    callback(this, &XrlFib2mribTargetBase::handle_fea_fib_client_0_1_add_route4)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fea_fib_client/0.1/add_route4");
	}
	if (_cmds->add_handler("fea_fib_client/0.1/add_route6",
	    callback(this, &XrlFib2mribTargetBase::handle_fea_fib_client_0_1_add_route6)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fea_fib_client/0.1/add_route6");
	}
	if (_cmds->add_handler("fea_fib_client/0.1/replace_route4",
	    callback(this, &XrlFib2mribTargetBase::handle_fea_fib_client_0_1_replace_route4)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fea_fib_client/0.1/replace_route4");
	}
	if (_cmds->add_handler("fea_fib_client/0.1/replace_route6",
	    callback(this, &XrlFib2mribTargetBase::handle_fea_fib_client_0_1_replace_route6)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fea_fib_client/0.1/replace_route6");
	}
	if (_cmds->add_handler("fea_fib_client/0.1/delete_route4",
	    callback(this, &XrlFib2mribTargetBase::handle_fea_fib_client_0_1_delete_route4)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fea_fib_client/0.1/delete_route4");
	}
	if (_cmds->add_handler("fea_fib_client/0.1/delete_route6",
	    callback(this, &XrlFib2mribTargetBase::handle_fea_fib_client_0_1_delete_route6)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fea_fib_client/0.1/delete_route6");
	}
	if (_cmds->add_handler("fea_fib_client/0.1/resolve_route4",
	    callback(this, &XrlFib2mribTargetBase::handle_fea_fib_client_0_1_resolve_route4)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fea_fib_client/0.1/resolve_route4");
	}
	if (_cmds->add_handler("fea_fib_client/0.1/resolve_route6",
	    callback(this, &XrlFib2mribTargetBase::handle_fea_fib_client_0_1_resolve_route6)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fea_fib_client/0.1/resolve_route6");
	}
	if (_cmds->add_handler("fib2mrib/0.1/enable_fib2mrib",
	    callback(this, &XrlFib2mribTargetBase::handle_fib2mrib_0_1_enable_fib2mrib)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fib2mrib/0.1/enable_fib2mrib");
	}
	if (_cmds->add_handler("fib2mrib/0.1/start_fib2mrib",
	    callback(this, &XrlFib2mribTargetBase::handle_fib2mrib_0_1_start_fib2mrib)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fib2mrib/0.1/start_fib2mrib");
	}
	if (_cmds->add_handler("fib2mrib/0.1/stop_fib2mrib",
	    callback(this, &XrlFib2mribTargetBase::handle_fib2mrib_0_1_stop_fib2mrib)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fib2mrib/0.1/stop_fib2mrib");
	}
	if (_cmds->add_handler("fib2mrib/0.1/enable_log_trace_all",
	    callback(this, &XrlFib2mribTargetBase::handle_fib2mrib_0_1_enable_log_trace_all)) == false) {
	    XLOG_ERROR("Failed to xrl handler finder://%s/%s", "fib2mrib", "fib2mrib/0.1/enable_log_trace_all");
	}
	_cmds->finalize();
}

void
XrlFib2mribTargetBase::remove_handlers()
{
	_cmds->remove_handler("common/0.1/get_target_name");
	_cmds->remove_handler("common/0.1/get_version");
	_cmds->remove_handler("common/0.1/get_status");
	_cmds->remove_handler("common/0.1/shutdown");
	_cmds->remove_handler("fea_fib_client/0.1/add_route4");
	_cmds->remove_handler("fea_fib_client/0.1/add_route6");
	_cmds->remove_handler("fea_fib_client/0.1/replace_route4");
	_cmds->remove_handler("fea_fib_client/0.1/replace_route6");
	_cmds->remove_handler("fea_fib_client/0.1/delete_route4");
	_cmds->remove_handler("fea_fib_client/0.1/delete_route6");
	_cmds->remove_handler("fea_fib_client/0.1/resolve_route4");
	_cmds->remove_handler("fea_fib_client/0.1/resolve_route6");
	_cmds->remove_handler("fib2mrib/0.1/enable_fib2mrib");
	_cmds->remove_handler("fib2mrib/0.1/start_fib2mrib");
	_cmds->remove_handler("fib2mrib/0.1/stop_fib2mrib");
	_cmds->remove_handler("fib2mrib/0.1/enable_log_trace_all");
}
