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
// PIM PIM_REGISTER_STOP messages processing.
//


#include "pim_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "pim_mre.hh"
#include "pim_mrt.hh"
#include "pim_vif.hh"


/**
 * PimVif::pim_register_stop_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Receive PIM_REGISTER_STOP message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
	int
PimVif::pim_register_stop_recv(PimNbr *pim_nbr,
		const IPvX& src,
		const IPvX& dst,
		buffer_t *buffer)
{
	int rcvd_family;
	uint8_t group_addr_reserved_flags;
	uint8_t group_mask_len;
	IPvX source_addr(family()), group_addr(family());
	UNUSED(dst);
	UNUSED(group_addr_reserved_flags);

	//
	// Parse the message
	//
	GET_ENCODED_GROUP_ADDR(rcvd_family, group_addr, group_mask_len,
			group_addr_reserved_flags, buffer);
	GET_ENCODED_UNICAST_ADDR(rcvd_family, source_addr, buffer);

	if (! group_addr.is_multicast()) 
	{
		XLOG_WARNING("RX %s from %s to %s: "
				"group address = %s must be multicast",
				PIMTYPE2ASCII(PIM_REGISTER_STOP),
				cstring(src), cstring(dst),
				cstring(group_addr));
		return (XORP_ERROR);
	}

	if (group_addr.is_linklocal_multicast()
			|| group_addr.is_interfacelocal_multicast()) 
	{
		XLOG_WARNING("RX %s from %s to %s: "
				"group address = %s must not be be link or "
				"interface-local multicast",
				PIMTYPE2ASCII(PIM_REGISTER_STOP),
				cstring(src), cstring(dst),
				cstring(group_addr));
		return (XORP_ERROR);
	}

	if (! (source_addr.is_unicast() || source_addr.is_zero())) 
	{
		XLOG_WARNING("RX %s from %s to %s: "
				"source address = %s must be either unicast or zero",
				PIMTYPE2ASCII(PIM_REGISTER_STOP),
				cstring(src), cstring(dst),
				cstring(source_addr));
		return (XORP_ERROR);
	}

	//
	// Process the Register-Stop data
	//
	pim_register_stop_process(src, source_addr, group_addr, group_mask_len);

	UNUSED(pim_nbr);
	return (XORP_OK);

	// Various error processing
rcvlen_error:
	XLOG_WARNING("RX %s from %s to %s: "
			"invalid message length",
			PIMTYPE2ASCII(PIM_REGISTER_STOP),
			cstring(src), cstring(dst));
	++_pimstat_rx_malformed_packet;
	return (XORP_ERROR);

rcvd_mask_len_error:
	XLOG_WARNING("RX %s from %s to %s: "
			"invalid group mask length = %d",
			PIMTYPE2ASCII(PIM_REGISTER_STOP),
			cstring(src), cstring(dst),
			group_mask_len);
	return (XORP_ERROR);

rcvd_family_error:
	XLOG_WARNING("RX %s from %s to %s: "
			"invalid address family inside = %d",
			PIMTYPE2ASCII(PIM_REGISTER_STOP),
			cstring(src), cstring(dst),
			rcvd_family);
	return (XORP_ERROR);
}

	int
PimVif::pim_register_stop_process(const IPvX& rp_addr,
		const IPvX& source_addr,
		const IPvX& group_addr,
		uint8_t group_mask_len)
{
	uint32_t	lookup_flags;
	PimMre	*pim_mre;
	UNUSED(rp_addr);

	lookup_flags = PIM_MRE_SG;

	if (group_mask_len != IPvX::addr_bitlen(family())) 
	{
		XLOG_WARNING("RX %s from %s to %s: "
				"invalid group mask length = %d "
				"instead of %u",
				PIMTYPE2ASCII(PIM_REGISTER_STOP),
				cstring(rp_addr), cstring(domain_wide_addr()),
				group_mask_len,
				XORP_UINT_CAST(IPvX::addr_bitlen(family())));
		return (XORP_ERROR);
	}

	if (! source_addr.is_zero()) 
	{
		// (S,G) Register-Stop
		pim_mre = pim_mrt().pim_mre_find(source_addr, group_addr,
				lookup_flags, 0);
		if (pim_mre == NULL) 
		{
			// XXX: we don't have this (S,G) state. Ignore
			// TODO: print a warning, or do something else?
			++_pimstat_unknown_register_stop;
			return (XORP_ERROR);
		}
		pim_mre->receive_register_stop();
		return (XORP_OK);
	}

	// (*,G) Register-Stop
	// Apply to all (S,G) entries for this group that are not in the NoInfo
	// state.
	// TODO: XXX: PAVPAVPAV: should schedule a timeslice task for this.
	PimMrtSg::const_gs_iterator iter_begin, iter_end, iter;
	iter_begin = pim_mrt().pim_mrt_sg().group_by_addr_begin(group_addr);
	iter_end = pim_mrt().pim_mrt_sg().group_by_addr_end(group_addr);
	for (iter = iter_begin; iter != iter_end; ++iter) 
	{
		PimMre *pim_mre = iter->second;
		if (! pim_mre->is_register_noinfo_state())
			pim_mre->receive_register_stop();
	}

	return (XORP_OK);
}

	int
PimVif::pim_register_stop_send(const IPvX& dr_addr,
		const IPvX& source_addr,
		const IPvX& group_addr,
		string& error_msg)
{
	uint8_t group_mask_len = IPvX::addr_bitlen(family());
	buffer_t *buffer = buffer_send_prepare();
	uint8_t group_addr_reserved_flags = 0;

	// Write all data to the buffer
	PUT_ENCODED_GROUP_ADDR(family(), group_addr, group_mask_len,
			group_addr_reserved_flags, buffer);
	PUT_ENCODED_UNICAST_ADDR(family(), source_addr, buffer);

	return (pim_send(domain_wide_addr(), dr_addr, PIM_REGISTER_STOP, buffer,
				error_msg));

invalid_addr_family_error:
	XLOG_UNREACHABLE();
	error_msg = c_format("TX %s from %s to %s: "
			"invalid address family error = %d",
			PIMTYPE2ASCII(PIM_REGISTER_STOP),
			cstring(domain_wide_addr()), cstring(dr_addr),
			family());
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);

buflen_error:
	XLOG_UNREACHABLE();
	error_msg = c_format("TX %s from %s to %s: "
			"packet cannot fit into sending buffer",
			PIMTYPE2ASCII(PIM_REGISTER_STOP),
			cstring(domain_wide_addr()), cstring(dr_addr));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
}
