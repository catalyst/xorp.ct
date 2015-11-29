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

// $XORP: xorp/rtrmgr/template_tree.hh,v 1.27 2008/10/02 21:58:25 bms Exp $

#ifndef __RTRMGR_TEMPLATE_TREE_HH__
#define __RTRMGR_TEMPLATE_TREE_HH__






#include "path_segment.hh"
#include "rtrmgr_error.hh"
#include "xorp_client.hh"


class ModuleCommand;
class TemplateTreeNode;
class ConfPathSegment;

class TemplateTree 
{
	public:
		TemplateTree(const string& xorp_root_dir,
				bool verbose)  throw (InitError);
		virtual ~TemplateTree();

		bool load_template_tree(const string& config_template_dir,
				string& error_msg);
		bool parse_file(const string& filename, 
				const string& config_template_dir, string& error_msg);

		void extend_path(const string& segment, bool is_tag);
		void pop_path() throw (ParseError);
		void push_path(int type, char* initializer);
		void add_untyped_node(const string& segment, bool is_tag) throw (ParseError);
		void add_node(const string& segment, int type, char* initializer);
		const TemplateTreeNode* find_node(const list<string>& path_segments) const;
		const TemplateTreeNode* 
			find_node_by_type(const list<ConfPathSegment>& path_segments) const;
		string path_as_string();
		void add_cmd(char* cmd) throw (ParseError);
		void add_cmd_action(const string& cmd, const list<string>& action)
			throw (ParseError);
		string tree_str() const;
		void register_module(const string& name, ModuleCommand* mc);
		ModuleCommand* find_module(const string& name);
		bool check_variable_name(const string& s) const;
		TemplateTreeNode* root_node() const { return _root_node; }
		const string& xorp_root_dir() const { return _xorp_root_dir; }
		bool verbose() const { return _verbose; }

	protected:
		TemplateTreeNode* new_node(TemplateTreeNode* parent,
				const string& path,
				const string& varname,
				int type,
				const string& initializer);

		bool expand_template_tree(string& error_msg);
		bool check_template_tree(string& error_msg);


		TemplateTreeNode*	_root_node;
		TemplateTreeNode*	_current_node;
		map<string, ModuleCommand *> _registered_modules;
		list<PathSegment>	_path_segments;
		list<size_t>	_segment_lengths;
		string		_xorp_root_dir;	// The root of the XORP tree
		bool		_verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_TEMPLATE_TREE_HH__
