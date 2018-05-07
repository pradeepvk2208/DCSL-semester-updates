#include "STAModel.hpp"

// #include "STAModel_io.raw"

const vector<string> STAModel::BBData::serialization_order = {"alloca", "load", "store", "add", "sub", "mul", "udiv", "sdiv", "urem", "srem", "icmp", "f_fadd", "f_fsub", "f_fmul", "f_fdiv", "f_frem", "f_fcmp", "d_fadd", "d_fsub", "d_fmul", "d_fdiv", "d_frem", "d_fcmp", "fptrunc", "fpext", "cast", "phi", "select", "br", "ret", "switch", "call", "getelementptr", "other"};

ostream &operator<<(ostream &out, const STAModel::BBData &bbdat)
{	// fixed: fix order of iteration
	for (const auto &i : STAModel::BBData::serialization_order)
	{
		out << bbdat.fixed_inst_count.at(i) << " ";
	}

	return out;
}

istream &operator>>(istream &in, STAModel::BBData &bbdat)
{	// fixed: fix order of iteration
	for (auto &i : STAModel::BBData::serialization_order)
	{
		size_t num;
		in >> num;
		bbdat.fixed_inst_count[i] = num;
	}

	return in;
}

ostream &operator<<(ostream &out, const mytree_node &mynode)
{
    out << mynode.bb << "\n";

	out << mynode.parents.size() << "\n";
	for (const auto &i : mynode.parents)
	{
		out << i << " ";
	}
	out << "\n";

	out << mynode.children.size() << "\n";
	for (const auto &i : mynode.children)
	{
		out << i << " ";
	}
	out << "\n";

	out << (mynode.inst_type.empty() ? "_no_inst_" : mynode.inst_type) << "\n";

	switch (mynode.orig_ret_type)
	{
		case mytype::Float:
		{
			out << 'f';
			break;
		}
		case mytype::Double:
		{
			out << 'd';
			break;
		}
		case mytype::Other:
		{
			out << 'o';
			break;
		}
		default:
		{
			my_throw("Unknown mytype");
		}
	}
	out << "\n";

	out << mynode.changeable << "\n";

	out << mynode.inst << "\n";

	out << mynode.locvr << "\n";

	return out;
}

istream &operator>>(istream &in, mytree_node &mynode)
{
    size_t sz;
    
    in >> sz;
    mynode.bb = sz;

	// cerr << "__0\n";
	
	in >> sz;
	// cerr << dec << sz << "\n";
	for (size_t i = 0; i < sz; ++i)
	{
		size_t idx;
		in >> idx;
		mynode.parents.insert(idx);
	}

	// cerr << "__1\n";
	
	in >> sz;
	// cerr << sz << "\n";
	for (size_t i = 0; i < sz; ++i)
	{
		size_t idx;
		in >> idx;
		// cerr << idx << " ";
		mynode.children.insert(idx);
	}
	// cerr << "\n";

	// cerr << "__2\n";
	
	string inst_nm;
	in >> inst_nm;
	// cerr << inst_nm << "\n";
	inst_nm = inst_nm == "_no_inst_" ? "" : inst_nm;
    mynode.inst_type = inst_nm;

	// cerr << "__3\n";
	
	char myty;
	in >> myty;
	// cerr << myty << "\n";
	switch (myty)
	{
		case 'f':
		{
			mynode.orig_ret_type = mytype::Float;
			break;
		}
		case 'd':
		{
			mynode.orig_ret_type = mytype::Double;
			break;
		}
		case 'o':
		{
			mynode.orig_ret_type = mytype::Other;
			break;
		}
		default:
		{
			cerr << "Char: '" << myty << "'\n";
			my_throw("Unknown mytype char");
		}
	}

	// cerr << "__4\n";
	
	bool changeable;
	in >> changeable;
	// cerr << changeable << "\n";
	mynode.changeable = changeable;
	
	// cerr << "__5\n";
	
	in >> hex >> sz >> dec;
	// cerr << hex << sz << dec << "\n";
	mynode.inst = reinterpret_cast<const Value *>(sz);
	
	// cerr << "__6\n";

	in >> hex >> sz >> dec;
	// cerr << hex << sz << dec << "\n";
	mynode.locvr = reinterpret_cast<const DILocalVariable *>(sz);
	
	return in;
}

ostream &operator<<(ostream &out, const STAModel &stamod)
{
	out << stamod.bbgraph.size() << "\n";
	for (const auto &i : stamod.bbgraph)
	{
		out << i.size() << " ";
		for (const auto &j : i)
		{
			out << j << " ";
		}
		out << "\n";
	}

	out << stamod.bbdata.size() << "\n";
	for (const auto &i : stamod.bbdata)
	{
		out << i << "\n";
	}

	out << stamod.arg_trees.size() << "\n";
	for (const auto &i : stamod.arg_trees)
	{
		out << i << " ";
	}
	out << "\n";

	out << stamod.localvar_trees.size() << "\n";
	for (const auto &i : stamod.localvar_trees)
	{
		out << i << " ";
	}
	out << "\n";

	out << stamod.mytree_nodes.size() << "\n";
	for (const auto &i : stamod.mytree_nodes)
	{
		out << i << "\n";
	}
	out << "\n";

	return out;
}

istream &operator>>(istream &in, STAModel &stamod)
{
	// cerr << "_0\n";

	stamod.pointers_valid = false;

	size_t sz;
	in >> sz;
	for (size_t i = 0; i < sz; ++i)
	{
		stamod.bbgraph.push_back({});
		size_t sz2;
		in >> sz2;
		for (size_t j = 0; j < sz2; ++j)
		{
			size_t idx;
			in >> idx;
			stamod.bbgraph.back().insert(idx);
		}
	}

	// cerr << "_1\n";
	
	in >> sz;
	// cerr << dec << sz << "\n";
	for (size_t i = 0; i < sz; ++i)
	{
		STAModel::BBData dat;
		in >> dat;
		// cerr << dat << "\n";
		stamod.bbdata.push_back(dat);
	}

	// cerr << "_2\n";
	
	in >> sz;
	// cerr << dec << sz << "\n";
	for (size_t i = 0; i < sz; ++i)
	{
		size_t num;
		in >> num;
		// cerr << dec << num << " ";
		stamod.arg_trees.push_back(num);
	}
	// cerr << "\n";

	// cerr << "_3\n";
	
	in >> sz;
	// cerr << dec << sz << "\n";
	for (size_t i = 0; i < sz; ++i)
	{
		size_t num;
		in >> num;
		// cerr << dec << num << " ";
		stamod.localvar_trees.push_back(num);
	}
	// cerr << "\n";

	// cerr << "_4\n";
	
	in >> sz;
	for (size_t i = 0; i < sz; ++i)
	{
		mytree_node node;
		in >> node;
		stamod.mytree_nodes.push_back(node);
	}

	// cerr << "_5\n";
	
	return in;
}
