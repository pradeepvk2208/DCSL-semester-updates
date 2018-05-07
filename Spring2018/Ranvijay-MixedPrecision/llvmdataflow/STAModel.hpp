#ifndef _STAMODEL_
#define _STAMODEL_

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <deque>
#include <iostream>
#include <exception>
#include <cstdlib>

#if __cpp_exceptions
#define my_throw(x) throw runtime_error(x)
#else
#define my_throw(x) do {		\
	std::cerr << "Exception: " << x << "\n";		\
	std::exit(EXIT_FAILURE);	\
} while (false)
#endif

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"

#include "llvm/IR/DebugInfoMetadata.h"

using namespace std;
using namespace llvm;

enum class mytype
{
	Other,
	Double,
	Float
	// todo: add type for root node?
};

struct mytree_node
{
	size_t bb;
	// unordered_set<mytree_node *> parents;
	// unordered_set<mytree_node *> children;
	unordered_set<size_t> parents;
	unordered_set<size_t> children;
	string inst_type;
	mytype orig_ret_type;

	bool changeable;

	const Value *inst;	// needed only for tree creation
	const DILocalVariable *locvr;

	mytree_node(const Value *i = nullptr, bool change = true) : changeable(change), inst(i), locvr(nullptr) {}
	mytree_node(const DILocalVariable *i, bool change = true) : changeable(change), inst(nullptr), locvr(i) {}

	friend ostream &operator<<(ostream &out, const mytree_node &mynode);
	friend istream &operator>>(istream &in, mytree_node &mynode);
};

struct STAModel
{
	struct BBData
	{
		// static const vector<string> serialization_order = {"alloca", "load", "store", "add", "sub", "mul", "udiv", "sdiv", "urem", "srem", "icmp", "f_fadd", "f_fsub", "f_fmul", "f_fdiv", "f_frem", "f_fcmp", "d_fadd", "d_fsub", "d_fmul", "d_fdiv", "d_frem", "d_fcmp", "fptrunc", "fpext", "cast", "phi", "select", "br", "ret", "switch", "call", "getelementptr", "other"};
		static const vector<string> serialization_order;

		unordered_map<string, size_t> fixed_inst_count;
		BBData()
		{
			fixed_inst_count =
			{
				{"alloca", 0},
				{"load", 0},
				{"store", 0},

				{"add", 0},
				{"sub", 0},
				{"mul", 0},
				{"udiv", 0},
				{"sdiv", 0},
				{"urem", 0},
				{"srem", 0},
				{"icmp", 0},
				
				// float
				{"f_fadd", 0},
				{"f_fsub", 0},
				{"f_fmul", 0},
				{"f_fdiv", 0},
				{"f_frem", 0},
				{"f_fcmp", 0},
				// double
				{"d_fadd", 0},
				{"d_fsub", 0},
				{"d_fmul", 0},
				{"d_fdiv", 0},
				{"d_frem", 0},
				{"d_fcmp", 0},
				// mixed
				{"fptrunc", 0},
				{"fpext", 0},

				// other
				{"cast", 0},
				{"phi", 0},
				{"select", 0},
				
				{"br", 0},
				{"ret", 0},
				{"switch", 0},
				{"call", 0},
				{"getelementptr", 0},
				{"other", 0}
			};

			assert(serialization_order.size() == fixed_inst_count.size());	// to ensure that serialisation stores all inst counts and programmer didn't forget to change one and not the other
		}

		friend ostream &operator<<(ostream &out, const BBData &bbdat);
		friend istream &operator>>(istream &in, BBData &bbdat);	
	};

	bool pointers_valid;

	vector<unordered_set<size_t>> bbgraph;
	vector<BBData> bbdata;

	vector<size_t> arg_trees;
	vector<size_t> localvar_trees;
	vector<mytree_node> mytree_nodes;

	STAModel(bool pointers = true) : pointers_valid(pointers)
	{	
	}

	size_t getNodeIndex(const Value *in) const
	{
		for (size_t i = 0; i < mytree_nodes.size(); ++i)	// use find_if(...)
		{
			if (mytree_nodes[i].inst == in)
			{
				return i;
			}
		}
		errs() << *in << "\n";
		my_throw("Unknown Instruction");
	}

	size_t getNodeIndex(const DILocalVariable *in) const
	{
		for (size_t i = 0; i < mytree_nodes.size(); ++i)	// use find_if(...)
		{
			if (mytree_nodes[i].locvr == in)
			{
				return i;
			}
		}
		// errs() << *in << "\n";
		my_throw("Unknown Var");
	}

	void remove_casts(vector<mytree_node> &vec) const
	{
		for (size_t i = 0; i < vec.size(); ++i)
		{
			if (vec[i].inst_type == "fpext" || vec[i].inst_type == "fptrunc")
			{
				assert(vec[i].inst_type == "fpext" || vec[i].inst_type == "fptrunc");
				
				// cout << "CURR I: " << i << "\n";
				for (const auto &j : vec[i].parents)
				{
					// cout << "CURR J: " << j << "\n";
					size_t num_erased = vec[j].children.erase(i);
					// assert(num_erased == 1);

					for (const auto &k : vec[i].children)
					{
						// cout << "CURR K: " << k << "\n";
						auto insert_status = vec[j].children.insert(k);
						// assert(insert_status.second);
					}
				}

				for (const auto &k : vec[i].children)
				{
					size_t num_erased = vec[k].parents.erase(i);
					// assert(num_erased == 1);

					for (const auto &j : vec[i].parents)
					{
						auto insert_status = vec[k].parents.insert(j);
						// assert(insert_status.second);
					}
				}

				vec[i].parents = {};
				vec[i].children = {};
			}
		}

		// Checks
		unordered_set<size_t> removed_idxs = {};
		for (size_t i = 0; i < vec.size(); ++i)
		{
			//  if (vec[i].inst && isa<CastInst>(vec[i].inst))
			 if (vec[i].inst_type == "fpext" || vec[i].inst_type == "fptrunc")
			 {
				 assert(vec[i].parents.size() == 0 && vec[i].children.size() == 0);
				 removed_idxs.insert(i);
			 }
		}

		for (size_t i = 0; i < vec.size(); ++i)
		{
			 for (const auto &j : vec[i].parents)
			 {
				 if (! (removed_idxs.find(j) == removed_idxs.cend()))
				 {
					 if (pointers_valid)
					 {
						 errs() << "CHILD_TO_PARENT: " << *vec[i].inst << "\n";
					 }
					 my_throw("Parent has already been removed");
				 }
			 }
			 for (const auto &j : vec[i].children)
			 {
				 if (! (removed_idxs.find(j) == removed_idxs.cend()))
				 {
					 if (pointers_valid)
					 {
						 errs() << "PARENT_TO_CHILD: " << *vec[i].inst << "\n";
					 }
					 my_throw("Child has already been removed");
				 }
			 }
		}

		// todo: remove empty nodes from vector?
		// will need to readjust indices stored in children/parents
	}

	int predict(vector<mytype> new_types_arg, vector<mytype> new_types_localvar) const
	{
		bool log_op = true;

		if (new_types_arg.size() != arg_trees.size())
		{
			cout << new_types_arg.size() << " " << arg_trees.size() << endl;
			my_throw("Wrong number of float/double arg types");
		}
		else if (new_types_localvar.size() != localvar_trees.size())
		{
			cout << new_types_localvar.size() << " " << localvar_trees.size() << endl;
			my_throw("Wrong number of float/double local var types");
		}

		assert (bbgraph.size() == bbdata.size());

		auto dynbbdata = bbdata;	// verified: copy not ref to orig
		auto dyntree = mytree_nodes;	// verified: copy not ref to orig

		deque<size_t> to_visit = {};
		// auto node_types = vector<vector<mytype>>(mytree_nodes.size());

		for (size_t i = 0; i < new_types_arg.size(); ++i)
		{
			if (mytree_nodes[arg_trees[i]].orig_ret_type != new_types_arg[i])
			{
				to_visit.push_back(arg_trees[i]);
				dyntree[arg_trees[i]].orig_ret_type = new_types_arg[i];
				if (log_op)
				{
					errs() << "Set dyntree[" << arg_trees[i] << "].orig_ret_type to " << (dyntree[arg_trees[i]].orig_ret_type == mytype::Double ? "double" : "float") << "\n";
				}
			}
			else
			{
				if (log_op)
				{
					errs() << "dyntree[" << arg_trees[i] << "].orig_ret_type unchanged at " << (dyntree[arg_trees[i]].orig_ret_type == mytype::Double ? "double" : "float") << "\n";
				}
			}
		}

		for (size_t i = 0; i < new_types_localvar.size(); ++i)
		{
			if (mytree_nodes[localvar_trees[i]].orig_ret_type != new_types_localvar[i])
			{
				to_visit.push_back(localvar_trees[i]);
				dyntree[localvar_trees[i]].orig_ret_type = new_types_localvar[i];
				if (log_op)
				{
					errs() << "Set dyntree[" << localvar_trees[i] << "].orig_ret_type to " << (dyntree[localvar_trees[i]].orig_ret_type == mytype::Double ? "double" : "float") << "\n";
				}
			}
			else
			{
				if (log_op)
				{
					errs() << "dyntree[" << localvar_trees[i] << "].orig_ret_type unchanged at " << (dyntree[localvar_trees[i]].orig_ret_type == mytype::Double ? "double" : "float") << "\n";
				}
			}
		}

		// errs() << "\nBEFORE TRAVERSING:\n";
		// print(errs());
		// errs() << "\n";

		remove_casts(dyntree);

		unordered_set<size_t> visited = {};
		while (to_visit.size())
		{
			// todo: for nodes with multiple parents, while visiting from a parent, 
			// assume other parentts stay the what they are then (either already changed or unchanged)
			// this way,	while visiting from final parent the correct value would be there
			// will probably need a special insert_casts() function to work on the final changed nodes

			const size_t curr_idx = to_visit.front();
			to_visit.pop_front();

			if (log_op && pointers_valid)
			{
				errs() << "VISITING: ";
				if (dyntree[curr_idx].inst)
				{
					errs() << *dyntree[curr_idx].inst;
				}
				else
				{
					errs() << "null";
				}
				errs() << "\n";
			}

			if (pointers_valid && dyntree[curr_idx].inst && isa<CastInst>(dyntree[curr_idx].inst))
			{
				// done: throw error since this never execute since casts have already been removed
				my_throw("Casts should already have been removed");
			}

			if (pointers_valid)
			{
				assert(! dyntree[curr_idx].inst || ! isa<CastInst>(dyntree[curr_idx].inst));
			}

			// if (find(visited.cbegin(), visited.cend(), curr_idx) == visited.cend())
			// {
				// if not visited
				// calaculate type from partents
				bool all_parents_are_float = true;
				mytype old_type = dyntree[curr_idx].orig_ret_type;
				if (log_op)
				{
					errs() << "Orig type: " << (dyntree[curr_idx].orig_ret_type == mytype::Double ? "double" : (dyntree[curr_idx].orig_ret_type == mytype::Float ? "float" : "undefined")) << "\n";
				}
				for (const auto &i : dyntree[curr_idx].parents)
				{
					if (log_op && pointers_valid)
					{
						errs() << "Parent " << i << ": ";
						if (dyntree[i].inst)
						{
							errs() << *dyntree[i].inst;
						}
						else
						{
							errs() << "null";
						}
						errs() << "; Type: " << (dyntree[i].orig_ret_type == mytype::Double ? "double" : "float") << "\n";
					}
					if (dyntree[i].orig_ret_type == mytype::Double)
					{
						all_parents_are_float = false;
						// not breaking loop in order to ensure assert in else{} checks out for all parents
					}
					else
					{
						// assert(dyntree[i].inst || dyntree[i].locvr);
						if (log_op && pointers_valid)
						{
							if (dyntree[i].inst)
							{
								errs() << "float parent: " << *dyntree[i].inst << "\n";
							}
							else
							{
								assert(dyntree[i].locvr);
								errs() << "float parent: " << dyntree[i].locvr->getName() << "\n";
							}
						}
						assert(dyntree[i].orig_ret_type == mytype::Float);
					}
				}
				bool type_changed;
				if (dyntree[curr_idx].parents.size())
				{
					// proceed only if there was at least 1 parent, ie non root node
					if (all_parents_are_float)
					{
						if (log_op)
						{
							errs() << "All parents float. Setting type to: float\n"; 
						}
						dyntree[curr_idx].orig_ret_type = mytype::Float;
					}
					else
					{
						if (log_op)
						{
							errs() << "At least 1 parent double. Setting type to: double\n"; 
						}
						dyntree[curr_idx].orig_ret_type = mytype::Double;
					}
					type_changed = (dyntree[curr_idx].orig_ret_type != old_type);
				}
				else
				{
					// if no parents, ie root node, tyoe_changed needs to compare agains original version
					type_changed = (dyntree[curr_idx].orig_ret_type != mytree_nodes[curr_idx].orig_ret_type);
				}

				if (type_changed)
				{
					if (log_op && pointers_valid)
					{
						errs() << "ADDING CHILDREN OF: ";
						if (dyntree[curr_idx].inst)
						{
							errs() << *dyntree[curr_idx].inst;
						}
						else
						{
							errs() << "null";
						}
						errs() << "\n";
					}
					for (const auto &i : dyntree[curr_idx].children)
					{
						// if (dyntree[i].orig_ret_type != dyntree[curr_idx].orig_ret_type)
						// {

						if (dyntree[i].changeable)
						{
							to_visit.push_back(i);
							if (log_op && pointers_valid)
							{
								errs() << "ADDED CHILD:" << *dyntree[i].inst << "\n";
							}
						}
						else
						{
							if (log_op && pointers_valid)
							{
								errs() << "DIDN'T ADD CHILD DUE TO UNCHANGEABILITY:" << *dyntree[i].inst << "\n";
							}
						}
					}
				}
				else
				{
					if (log_op)
					{
						errs() << "No change in type. Not adding any more children\n";
					}
				}

				visited.insert(curr_idx);
		}
		
		// errs() << "\nAFTER TRAVERSING:\n";
		// print(errs());
		// errs() << "\n";

		// verify: done: add casts

		// for unchanghed instruction, add their instruction counts directly
		for (size_t i = 0; i < dyntree.size(); ++i)
		{

			if ((dyntree[i].parents.size() == 0 && dyntree[i].children.size() == 0) || (dyntree[i].parents.size() == 0 && !dyntree[i].inst && find(arg_trees.cbegin(), arg_trees.cend(), i) != arg_trees.cend()))
			{
				// cout << "Skipping\n";

				// empty (castinst) node or root node
				// don't need to add anything
				continue;
            }

            string prefix = dyntree[i].orig_ret_type == mytype::Double ? "d_" : "f_";

			if (dyntree[i].inst_type == "load" || dyntree[i].inst_type == "store" || dyntree[i].inst_type == "ret" || 
				dyntree[i].inst_type == "getelementptr" || dyntree[i].inst_type == "alloca" || dyntree[i].inst_type == "select" || 
			    dyntree[i].inst_type == "phi")
			{
				// errs() << dyntree[i].inst_type << "\n";
				// errs() << dyntree[i].bb << "\n";
				++dynbbdata[dyntree[i].bb].fixed_inst_count[dyntree[i].inst_type];
			} 
			else if (dyntree[i].inst_type == "fadd" || dyntree[i].inst_type == "fsub" || dyntree[i].inst_type == "fmul" || dyntree[i].inst_type == "fdiv" || dyntree[i].inst_type == "frem" || dyntree[i].inst_type == "fcmp")
			{
                // cout << prefix + dyntree[i].inst_type << "\n";
                // cout << dyntree[i].bb << "\n";
                ++dynbbdata[dyntree[i].bb].fixed_inst_count[prefix + dyntree[i].inst_type];
			}
			else if (dyntree[i].inst_type == "fpext" || dyntree[i].inst_type == "fptrunc")
			{
				// errs() << "CAST: " << *dyntree[i].inst << "\n";
				// errs() << "# Parents: " << dyntree[i].parents.size() << "\n";
				// errs() << "# Children: " << dyntree[i].children.size() << "\n";
				++dynbbdata[dyntree[i].bb].fixed_inst_count[dyntree[i].inst_type];
			}
			else
			{
                // If not argument, process
                if (find(arg_trees.cbegin(), arg_trees.cend(), i) == arg_trees.cend())
                {
                    cerr << "UNKNOWN MYTREE INST TYPE: \'" << dyntree[i].inst_type << "\'\n";
					if (pointers_valid)
					{
						if (dyntree[i].inst)
						{
							if (pointers_valid)
							{
								errs() << *dyntree[i].inst;
							}
							else
							{
								errs() << "cant_print";
							}
							errs() << "\n";
						}
						else
						{
							errs() << "null\n";
						}
					}
                }
            }

			// add casts if the children are of different type
			auto curr_type = dyntree[i].orig_ret_type;
            bool children_same = true;

			for (const auto &j : dyntree[i].children)
			{
				auto child_type = dyntree[j].orig_ret_type;
				// all instructions up until now use the same datatype as their operands

				// can move out of the loop to have only a single cast and use that casted value?
				// NO, value of variuable might be different for different uses
				// BUT SSA ensures that a single value isn't written to again
				// todo: check of other optimisation levels
				if (child_type != curr_type)
				{
					children_same = false;
					if (curr_type == mytype::Double && child_type == mytype::Float)
					{
						// fptrunc for child's basic block
						++dynbbdata[dyntree[j].bb].fixed_inst_count["fptrunc"];
					}
					else if (curr_type == mytype::Float && child_type == mytype::Double)
					{
						// fpext for child's basi block
						++dynbbdata[dyntree[j].bb].fixed_inst_count["fpext"];
					}
					else
					{
						// assert(dyntree[i].orig_ret_type == mytype::Float || dyntree[i].orig_ret_type == mytype::Double);
						// assert(dyntree[j].orig_ret_type == mytype::Float || dyntree[j].orig_ret_type == mytype::Double);
						if (pointers_valid)
						{
							errs() << "Parent: " << *dyntree[i].inst << "\n";
							errs() << "Parent Type: " << (dyntree[i].orig_ret_type == mytype::Double ? "double" : "float") << "\n";
							errs() << "Child: " << *dyntree[j].inst << "\n";
							errs() << "Child Type: " << (dyntree[j].orig_ret_type == mytype::Double ? "double" : "float") << "\n";
						}
						my_throw("Unknown parent-child type combination found");
					}
				}
            }
        }
		// errs() << "Score: " << score(dynbbdata) << "\n";
		
		// Printing modified model
		if (log_op && pointers_valid)
		{
			auto &out = cerr;
			auto stamod = *this; 

			out << "############################################\n";

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
		
			out << dynbbdata.size() << "\n";
			for (const auto &i : dynbbdata)
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
		
			out << dyntree.size() << "\n";
			for (const auto &i : dyntree)
			{
				out << i << "\n";
				if (i.inst)
				{
					errs() << "MY: " << *i.inst << "\n";
				}
				else
				{
					assert(i.locvr);
					errs() << "MY: " << i.locvr->getName() << "\n";
				}
			}
			out << "\n";
			out << "############################################\n";
		}

		if (false && pointers_valid)
		{
			for (size_t i = 0; i < dyntree.size(); ++i)
			{
				if (dyntree[i].orig_ret_type != mytype::Float)
				{
					deque<size_t> faulty = {};
					faulty.push_back(i);

					cerr << "FAULT CHAIN:\n";
					while (not faulty.empty())
					{
						auto j = faulty.front();
						faulty.pop_front();
						errs() << "----------------------------------------------------\n";
						cerr << "IDX: " << j << "\n";
						cerr << dyntree[j] << "\n";
						if (dyntree[j].inst)
						{
							errs() << "MY: " << *dyntree[j].inst << "\n";
						}
						else
						{
							assert(dyntree[j].locvr);
							errs() << "MY: " << dyntree[j].locvr->getName() << "\n";
						}
						errs() << "----------------------------------------------------\n";

						for (const auto &k : dyntree[j].parents)
						{
							if (dyntree[k].orig_ret_type != mytype::Float)
							{
								faulty.push_back(k);
							}
						}
					}
					break;
				}
			}
		}

        return score(dynbbdata);
	}

	void print(raw_ostream &cout) const
	{
		cout << "MYTREE:\n";
		for (size_t i = 0; i < mytree_nodes.size(); ++i)
		{
			cout << "NODE " << i << ":\n";

			cout << "Parents: ";
			for (const auto &j : mytree_nodes[i].parents)
			{
				cout << j << ", ";
			}
			cout << "\n";

			cout << "Children: ";
			for (const auto &j : mytree_nodes[i].children)
			{
				cout << j << ", ";
			}
			cout << "\n";

			cout << "Bb: " << mytree_nodes[i].bb << "\n";

			cout << "Inst: ";
			if (mytree_nodes[i].inst)
			{
				cout << *mytree_nodes[i].inst;
			}
			else
			{
				cout << "null";
			}
			cout << "\n";

			cout << "inst_type: " << mytree_nodes[i].inst_type << "\n";

			cout << "\n";
		}

		// for (size_t i = 0; i < bbdata.size(); ++i)
		// {
		//	 cout << "BASIC BLOCK: " << i << "\n";
		//	 for (const auto &j : bbdata[i].fixed_inst_count)
		//	 {
		//		 cout << j.first << ": " << j.second << "\n";
		//	 }
		//	 cout << "\n";
		// }

		cout << "\n";
	}

	int score(const vector<BBData> &dynbbdata) const
	{
		// for sm_35 ONLY
		// assuming 1/960 = 1

        // cout << "__0\n";

		auto nums = vector<int>(8, 0);

		int cycles = 0;
		for (const auto &i : dynbbdata)
		{
			for (const auto &j : i.fixed_inst_count)
			{
				if (j.first == "add" || j.first == "sub" || j.first == "mul")
				{
					cycles += j.second * 6;
					nums[0] = nums[0] + j.second;
				}
				else if (j.first == "udiv" || j.first == "sdiv")
				{
					// todo: floating point cast?
					// reciprocal + multiplication
					cycles += j.second * (30 + 6);
					nums[1] = nums[1] + j.second;
				}
				else if (j.first == "urem" || j.first == "srem")
				{
					// todo
				}
				else if (j.first == "f_fadd" || j.first == "f_fsub" || j.first == "f_fmul")
				{
					cycles += j.second * 5;
					nums[2] = nums[2] + j.second;
				}
				else if (j.first == "f_fdiv")
				{
					// reciprocal + multiplicTION
					cycles += j.second * (30 + 5);
					nums[3] = nums[3] + j.second;
				}
				else if (j.first == "f_frem")
				{
					// todo
				}
				else if (j.first == "d_fadd" || j.first == "d_fsub" || j.first == "d_fmul")
				{
					cycles += j.second * 15;
					nums[4] = nums[4] + j.second;
				}
				else if (j.first == "d_fdiv")
				{
					// todo: floating point 32 cast?
					// reciprocal + multiplicTION
					cycles += j.second * (30 + 15);
					nums[5] = nums[5] + j.second;
				}
				else if (j.first == "d_frem")
				{
					// todo
				}
				else if (j.first == "icmp" || j.first == "fcmp")
				{
					cycles += j.second * 6;
					nums[6] = nums[6] + j.second;
				}
				else if (j.first == "fptrunc" || j.first == "fpext")
				{
					cycles += j.second * 30;
					nums[7] = nums[7] + j.second;
				}
			}
        }
        
        // cout << "__9\n";
		for (const auto &i : nums)
		{
			cout << i << " ";
		}
		cout << "\n";

		return cycles;
	}

	friend ostream &operator<<(ostream &out, const STAModel &stamod);
	friend istream &operator>>(istream &in, STAModel &stamod);
};

ostream &operator<<(ostream &out, const STAModel::BBData &bbdat);
istream &operator>>(istream &in, STAModel::BBData &bbdat);

ostream &operator<<(ostream &out, const mytree_node &mynode);
istream &operator>>(istream &in, mytree_node &mynode);

ostream &operator<<(ostream &out, const STAModel &stamod);
istream &operator>>(istream &in, STAModel &stamod);

#endif