// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2011 XORP, Inc and Others
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

#include <xorp_config.h>



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "fea/ifconfig.hh"

#include "ifconfig_property_linux.hh"


//
// Get information about the data plane property.
//
// The mechanism to obtain the information is for Linux systems.
//


IfConfigPropertyLinux::IfConfigPropertyLinux(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigProperty(fea_data_plane_manager)
{
}

IfConfigPropertyLinux::~IfConfigPropertyLinux()
{
}

bool
IfConfigPropertyLinux::test_have_ipv4() const
{
    XorpFd s = comm_sock_open(AF_INET, SOCK_DGRAM, 0, 0);
    if (! s.is_valid())
	return (false);

    comm_close(s);

    return (true);
}

bool
IfConfigPropertyLinux::test_have_ipv6() const
{
#ifndef HAVE_IPV6
    return (false);
#else
    XorpFd s = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, 0);
    if (! s.is_valid())
	return (false);

    comm_close(s);
    return (true);
#endif // HAVE_IPV6
}


