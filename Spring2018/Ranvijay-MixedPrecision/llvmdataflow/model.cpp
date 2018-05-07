#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <deque>
#include <exception>
#include <algorithm>
#include <iostream>
#include <fstream>
#define OLD_NDEBUG NDEBUG
#undef NDEBUG
#include <cassert>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/CFG.h"

#include "STAModel.hpp"

using namespace llvm;
using namespace std;

#define LUL_FUN_NM "_Z45ApplyMaterialPropertiesAndUpdateVolume_kernelidddPdS_S_S_dddddPiS_S_S_S_dS_dS0_iPKiS2_i"

#include "STAModel.cpp"	// Importing .cpp: needs to be here since can't link

namespace {

const Type *getBaseTypeFromPointerOrArray(const Type *curr_type)
{
	while (isa<PointerType>(curr_type) || isa<ArrayType>(curr_type))
	{
		if (isa<PointerType>(curr_type))
		{
			curr_type = dyn_cast<PointerType>(curr_type)->getElementType();
		}
		else
		{
			assert(isa<ArrayType>(curr_type));
			curr_type = dyn_cast<ArrayType>(curr_type)->getElementType();
		}
	}

	return curr_type;
}

struct STAAnalysis : public FunctionPass 
{
	
	static char ID;

	// unordered_map<const Argument *, size_t> arg_index_map;

	STAModel model;
	const Function *curr_func;

	STAAnalysis() : FunctionPass(ID) {}

	virtual void getAnalysisUsage(AnalysisUsage &Info) const override
	{
		Info.addRequired<DominatorTreeWrapperPass>();
	}
	
	bool runOnFunction(Function &F) override 
	{
		curr_func = &F;
		const auto &dom_tree = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

		// build bbgraph ///////////////////////////////////////////////////////
		vector<const BasicBlock *> bbmap;
		model = {};

		for (const auto &BB : F)
		{
			bbmap.push_back(&BB);
			model.bbgraph.push_back({});
			assert(bbmap.size() == model.bbgraph.size());
		}

		const auto getBbmapIndex = [&bbmap] (const BasicBlock *bb) -> size_t
		{
			auto it = find(bbmap.cbegin(), bbmap.cend(), bb);
			if (it != bbmap.cend())
			{
				return it - bbmap.cbegin();
			}
			else
			{
				// return -1;
				my_throw("Unknown BasicBlock");
			}
		};
		
		for (const auto &i : F)
		{
			for (const auto *j : predecessors(&i))
			{
				int pred_index = getBbmapIndex(j);
				model.bbgraph.at(pred_index).insert(getBbmapIndex(&i));
			}
			for (const auto *j : successors(&i))
			{
				int succ_index = getBbmapIndex(j);
				model.bbgraph.at(getBbmapIndex(&i)).insert(succ_index);
			}
		}

		// Find function argumetns and uses ////////////////////////////////
		
		// errs() << "FUNC:" << F.getName() << "\n";
		
		unordered_set<const Value *> insts_in_trees = {};

		// Sets the orig_ret_type of model.mytree_nodes.back()
		// based on the type of user
		auto get_mytype = [] (const Value *user) -> mytype
		{
			const Type *user_type = user->getType();
			if (user_type->isPointerTy() || user_type->isArrayTy())
			{
				user_type = getBaseTypeFromPointerOrArray(user_type);
				assert(user_type);
			}
			
			if (user_type->isDoubleTy())
			{
				return mytype::Double;
			}
			else if (user_type->isFloatTy())
			{
				return mytype::Float;
			}
			else
			{
				const Type *operation_type;
				if (auto storei = dyn_cast<StoreInst>(user))
				{
					operation_type = storei->getValueOperand()->getType();
				}
				else if (auto reti = dyn_cast<ReturnInst>(user))
				{
					operation_type = reti->getReturnValue()->getType();
				}
				else if (auto fcmpi = dyn_cast<FCmpInst>(user))
				{
					operation_type = fcmpi->value_op_begin()->getType();
				}
				else
				{
					// verify
					if (!isa<BranchInst>(user) && !isa<CallInst>(user) && !isa<BitCastInst>(user) &&
						!(isa<BinaryOperator>(user) && (dyn_cast<BinaryOperator>(user)->getOpcode() == Instruction::And || dyn_cast<BinaryOperator>(user)->getOpcode() == Instruction::Or)))	// exceptions for known handled types
					{
						errs() << "UNHANDLED INST:" << *user << "\n";
					}
					return mytype::Other;
					// my_throw("Unhandled instruction with non float/double type");
				}

				if (operation_type->isPointerTy() || operation_type->isArrayTy())
				{
					operation_type = getBaseTypeFromPointerOrArray(operation_type);
					assert(operation_type);
				}
		
				switch(operation_type->getTypeID())
				{
					case Type::FloatTyID:
					{
						return mytype::Float;
						// model.mytree_nodes.back().orig_ret_type = mytype::Float;
						// break;
					}
					case Type::DoubleTyID:
					{
						return mytype::Double;
						// model.mytree_nodes.back().orig_ret_type = mytype::Double;
						// break;
					}
					default:
					{
						// verify
						errs() << "Unknown operation type: " << *user << "\n";
						return mytype::Other;
						// my_throw("Dependent non float/double oprtation");
					}
				}
			}
			// assert(model.mytree_nodes.back().orig_ret_type == mytype::Double || model.mytree_nodes.back().orig_ret_type == mytype::Float);

			// // temp
			// if (isa<AllocaInst>(user))
			// {
			// 	errs() << "ALLOCA: " << *user << "\n";
			// 	errs() << "TYPE: " << (model.mytree_nodes.back().orig_ret_type == mytype::Double ? "double" : "float") << "\n";
			// }
			// else
			// {
			// 	errs() << "NON ALLOCA: " << *user << "\n";
			// }
		};

		auto assign_inst_type = [this] (const Instruction *ins)
		{
			// errs() << "__0\n";
			if (isa<BinaryOperator>(ins))
			{
				switch (dyn_cast<BinaryOperator>(ins)->getOpcode())
				{
					case Instruction::FAdd:
					{
						model.mytree_nodes.back().inst_type = "fadd";
						break;
					}
					case Instruction::FSub:
					{
						model.mytree_nodes.back().inst_type = "fsub";
						break;
					}
					case Instruction::FMul:
					{
						model.mytree_nodes.back().inst_type = "fmul";
						break;
					}
					case Instruction::FDiv:
					{
						model.mytree_nodes.back().inst_type = "fdiv";
						break;
					}
					case Instruction::FRem:
					{
						model.mytree_nodes.back().inst_type = "frem";
						break;
					}
					default:
					{
						assert(ins);
						errs() << "OTHER DEPENDENT INST: " << *ins << "\n";
					}
				}
			}
			else if (isa<AllocaInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "alloca";
			}
			else if (isa<LoadInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "load";
			}
			else if (isa<StoreInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "store";
			}
			else if (isa<FCmpInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "fcmp";
			}
			else if (isa<FPTruncInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "fptrunc";
			}
			else if (isa<FPExtInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "fpext";
			}
			else if (isa<ReturnInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "ret";
			}
			else if (isa<GetElementPtrInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "getelementptr";
			}
			else if (isa<SelectInst>(ins))
			{
				model.mytree_nodes.back().inst_type = "select";
			}
			else if (isa<PHINode>(ins))
			{
				model.mytree_nodes.back().inst_type = "phi";
			}
			else
			{
				assert(ins);
				if (!isa<CallInst>(ins) and not isa<BitCastInst>(ins))
				{
					errs() << "OTHER DEPENDENT INST: " << *ins << "\n";
				}
			}
			// errs() << "__9\n";
		};

		auto process_arg_root_node = [this, &getBbmapIndex, &insts_in_trees, &get_mytype, &assign_inst_type] (const Value &arg)
		{
			auto &root_node_trees = model.arg_trees;

			const Type *arg_type = getBaseTypeFromPointerOrArray(arg.getType());
			assert(arg_type->isDoubleTy() || arg_type->isFloatTy());

			model.mytree_nodes.emplace_back(&arg);	

			size_t arg_node_idx = model.mytree_nodes.size() - 1;
			root_node_trees.push_back(arg_node_idx);
			model.mytree_nodes.back().orig_ret_type = arg_type->isDoubleTy() ? mytype::Double : mytype::Float; // not checking for other typs because of assert check above

			deque<const Value *> remaining_users = {};

			for (const auto &user : arg.users())
			{
				assert(isa<Instruction>(user));

				// errs() << "Current user: " << *user << "\n";

				// done: check if it already exists
				const Value *curr_user = &arg;	// fixed: datatype
				const size_t curr_node_idx = model.getNodeIndex(curr_user);
				// const Instruction *ins = dyn_cast<Instruction>(user);
				if (find(insts_in_trees.cbegin(), insts_in_trees.cend(), user) != insts_in_trees.cend())
				{
					// cout << "__0: Already exists: " << curr_node_idx << "-->" << model.getNodeIndex(ins) << "\n";
					
					// fixme: StoreInst uses it's users
					if (isa<StoreInst>(curr_user) && isa<StoreInst>(user))
					{
						continue;
					}
					
					model.mytree_nodes[model.getNodeIndex(user)].parents.insert(curr_node_idx);
					model.mytree_nodes[curr_node_idx].children.insert(model.getNodeIndex(user));
					continue;
				}

				mytype user_type = get_mytype(user);
				if (user_type == mytype::Float || user_type == mytype::Double)	// fixed: store/load/cast to int, etc?
				// if (isa<StoreInst>(user) || user->getType()->isFloatTy() || user->getType()->isDoubleTy())	// fixme: store/load/cast to int, etc?
				{ 
					const Instruction *ins = dyn_cast<Instruction>(user);
					model.mytree_nodes.emplace_back(ins);
					model.mytree_nodes.back().bb = getBbmapIndex(ins->getParent());
					// model.mytree_nodes.back().parents.insert(&model.mytree_nodes[arg_node_idx]);
					model.mytree_nodes.back().parents.insert(arg_node_idx);
					
					model.mytree_nodes.back().orig_ret_type = get_mytype(user);
					
					// model.mytree_nodes[arg_node_idx].children.insert(&model.mytree_nodes.back());
					model.mytree_nodes[arg_node_idx].children.insert(model.mytree_nodes.size() - 1);

					// inst_type
					assign_inst_type(dyn_cast<Instruction>(user));

					remaining_users.push_back(user);
					insts_in_trees.insert(user);

					// todo: special handling of stores?
				}
				else
				{
					if (not isa<CallInst>(user) and not isa<BitCastInst>(user))
					{
						errs() << "OTHER USE: " << *dyn_cast<Instruction>(user) << "\n";
					}
				}
			}

			// make tree of use-chain and travers tree to get inst counts
			while (remaining_users.size())
			{
				// const Value *curr = remaining_users.front();
				const Value *curr_user = remaining_users.front();
				remaining_users.pop_front();
				const size_t curr_node_idx = model.getNodeIndex(curr_user);

				// curr_user points to instruction, curr points to the value which will be used by dependent instructions
				// same for most cases, but for stores, the stored location will be the value
				// const Value *curr = (isa<StoreInst>(curr_user) ? dyn_cast<StoreInst>(curr)->getPointerOperand() : curr_user);
				const Value *curr = curr_user;
				if (isa<StoreInst>(curr_user))
				{
					curr = dyn_cast<StoreInst>(curr)->getPointerOperand();
				}

				for (const auto &user : curr->users())
				{
					assert(isa<Instruction>(user));
					
					mytype user_type = get_mytype(user);
					if (user_type == mytype::Float || user_type == mytype::Double)
					// if (isa<StoreInst>(user) || user->getType()->isFloatTy() || user->getType()->isDoubleTy())
					{
						// const Instruction *ins = dyn_cast<Instruction>(user);

						// check if it already exists
						if (find(insts_in_trees.cbegin(), insts_in_trees.cend(), user) != insts_in_trees.cend())
						{
							// cout << "__0: Already exists: " << curr_node_idx << "-->" << model.getNodeIndex(user) << "\n";
							
							// fixme: StoreInst uses it's users
							if (isa<StoreInst>(curr_user) && isa<StoreInst>(user))
							{
								continue;
							}
							
							model.mytree_nodes[model.getNodeIndex(user)].parents.insert(curr_node_idx);
							model.mytree_nodes[curr_node_idx].children.insert(model.getNodeIndex(user));
							continue;
						}

						model.mytree_nodes.emplace_back(user);
						model.mytree_nodes.back().bb = getBbmapIndex(dyn_cast<Instruction>(user)->getParent());
						// model.mytree_nodes.back().parents.insert(&model.mytree_nodes[curr_node_idx]);
						// model.mytree_nodes[curr_node_idx].children.insert(&model.mytree_nodes.back());
						// cout << "__1: New node: " << curr_node_idx << "-->" << model.mytree_nodes.size() - 1 << "\n";
						model.mytree_nodes.back().parents.insert(curr_node_idx);

						model.mytree_nodes.back().orig_ret_type = get_mytype(user);
						
						model.mytree_nodes[curr_node_idx].children.insert(model.mytree_nodes.size() - 1);

						assign_inst_type(dyn_cast<Instruction>(user));

						remaining_users.push_back(user);
						insts_in_trees.insert(user);
					}
					else
					{
						if (!isa<BranchInst>(user) && !isa<CallInst>(user) && !isa<BitCastInst>(user) &&
						    !(isa<BinaryOperator>(user) && (dyn_cast<BinaryOperator>(user)->getOpcode() == Instruction::And || dyn_cast<BinaryOperator>(user)->getOpcode() == Instruction::Or)))	// exceptions for known handled types
						{
							errs() << "NON FLOAT/DOUBLE DEP INST: " << *user << "\n";
						}
					}
				}
			}
		};

		unordered_map<const DILocalVariable *, unordered_set<const Value *>> localvar_map;
		unordered_map<const DILocalVariable *, const Instruction *> localvar_map2;

		auto process_localver_root_node = [this, &getBbmapIndex, &insts_in_trees, &get_mytype, &assign_inst_type, &localvar_map, &localvar_map2] (const DILocalVariable &localvar)
		{
			// errs() << "Now processing: " << localvar.getName() << "\n";

			auto &root_node_trees = model.localvar_trees;

			const Type *localvar_type = getBaseTypeFromPointerOrArray((*localvar_map[&localvar].begin())->getType());
			assert(localvar_type->isDoubleTy() || localvar_type->isFloatTy());	// todo: if arg is store, etc, this assert will fail
			
			model.mytree_nodes.emplace_back(&localvar);

			// get basic block
			model.mytree_nodes.back().bb = getBbmapIndex(localvar_map2[&localvar]->getParent());

			size_t localvar_node_idx = model.mytree_nodes.size() - 1;
			root_node_trees.push_back(localvar_node_idx);
			model.mytree_nodes.back().orig_ret_type = localvar_type->isDoubleTy() ? mytype::Double : mytype::Float; // not checking for other typs because of assert check above

			deque<const Value *> remaining_users = {};

			for (const auto &user_ptr : localvar_map[&localvar])
			{
				const auto &user = user_ptr;

				if (not isa<Instruction>(user))
				{
					// errs() << "YO: " << *user << "\n";
					continue;
				}

				// errs() << "Current user: " << *user << "\n";

				// done: check if it already exists
				const DILocalVariable *curr_user = &localvar;
				const size_t curr_node_idx = model.getNodeIndex(curr_user);
				// const Instruction *ins = dyn_cast<Instruction>(user);
				if (find(insts_in_trees.cbegin(), insts_in_trees.cend(), user) != insts_in_trees.cend())
				{
					// cout << "__0: Already exists: " << curr_node_idx << "-->" << model.getNodeIndex(ins) << "\n";
					
					// verify: StoreInst uses it's users, not needed for args
					// if (isa<StoreInst>(curr_user) && isa<StoreInst>(user))
					// {
					// 	continue;
					// }
					
					model.mytree_nodes[model.getNodeIndex(user)].parents.insert(curr_node_idx);
					model.mytree_nodes[curr_node_idx].children.insert(model.getNodeIndex(user));
					continue;
				}

				mytype user_type = get_mytype(user);
				if (user_type == mytype::Float || user_type == mytype::Double)	// fixed: store/load/cast to int, etc?
				// if (isa<StoreInst>(user) || user->getType()->isFloatTy() || user->getType()->isDoubleTy())	// fixme: store/load/cast to int, etc?
				{ 
					const Instruction *ins = dyn_cast<Instruction>(user);
					model.mytree_nodes.emplace_back(ins);
					model.mytree_nodes.back().bb = getBbmapIndex(ins->getParent());
					model.mytree_nodes.back().parents.insert(localvar_node_idx);
					
					model.mytree_nodes.back().orig_ret_type = get_mytype(user);
					
					model.mytree_nodes[localvar_node_idx].children.insert(model.mytree_nodes.size() - 1);

					// inst_type
					assign_inst_type(dyn_cast<Instruction>(user));

					remaining_users.push_back(user);
					insts_in_trees.insert(user);

					// todo: special handling of stores?
				}
				else
				{
					if (not isa<CallInst>(user) and not isa<BitCastInst>(user))
					{
						errs() << "OTHER USE: " << *dyn_cast<Instruction>(user) << "\n";
					}
				}
			}

			// make tree of use-chain and travers tree to get inst counts
			while (remaining_users.size())
			{
				// const Value *curr = remaining_users.front();
				const Value *curr_user = remaining_users.front();
				remaining_users.pop_front();
				const size_t curr_node_idx = model.getNodeIndex(curr_user);

				// curr_user points to instruction, curr points to the value which will be used by dependent instructions
				// same for most cases, but for stores, the stored location will be the value
				// const Value *curr = (isa<StoreInst>(curr_user) ? dyn_cast<StoreInst>(curr)->getPointerOperand() : curr_user);
				const Value *curr = curr_user;
				if (isa<StoreInst>(curr_user))
				{
					curr = dyn_cast<StoreInst>(curr)->getPointerOperand();
				}

				for (const auto &user : curr->users())
				{
					assert(isa<Instruction>(user));
					
					mytype user_type = get_mytype(user);
					if (user_type == mytype::Float || user_type == mytype::Double)
					// if (isa<StoreInst>(user) || user->getType()->isFloatTy() || user->getType()->isDoubleTy())
					{
						// const Instruction *ins = dyn_cast<Instruction>(user);

						// check if it already exists
						if (find(insts_in_trees.cbegin(), insts_in_trees.cend(), user) != insts_in_trees.cend())
						{
							// cout << "__0: Already exists: " << curr_node_idx << "-->" << model.getNodeIndex(user) << "\n";
							
							// fixme: StoreInst uses it's users
							if (isa<StoreInst>(curr_user) && isa<StoreInst>(user))
							{
								continue;
							}
							
							model.mytree_nodes[model.getNodeIndex(user)].parents.insert(curr_node_idx);
							model.mytree_nodes[curr_node_idx].children.insert(model.getNodeIndex(user));
							continue;
						}

						model.mytree_nodes.emplace_back(user);
						model.mytree_nodes.back().bb = getBbmapIndex(dyn_cast<Instruction>(user)->getParent());
						// model.mytree_nodes.back().parents.insert(&model.mytree_nodes[curr_node_idx]);
						// model.mytree_nodes[curr_node_idx].children.insert(&model.mytree_nodes.back());
						// cout << "__1: New node: " << curr_node_idx << "-->" << model.mytree_nodes.size() - 1 << "\n";
						model.mytree_nodes.back().parents.insert(curr_node_idx);

						model.mytree_nodes.back().orig_ret_type = get_mytype(user);
						
						model.mytree_nodes[curr_node_idx].children.insert(model.mytree_nodes.size() - 1);

						assign_inst_type(dyn_cast<Instruction>(user));

						remaining_users.push_back(user);
						insts_in_trees.insert(user);
					}
					else
					{
						if (!isa<BranchInst>(user) && !isa<CallInst>(user) && !isa<BitCastInst>(user) &&
						    !(isa<BinaryOperator>(user) && (dyn_cast<BinaryOperator>(user)->getOpcode() == Instruction::And || dyn_cast<BinaryOperator>(user)->getOpcode() == Instruction::Or)))	// exceptions for known handled types
						{
							errs() << "NON FLOAT/DOUBLE DEP INST: " << *user << "\n";
						}
					}
				}
			}
		};

		for (const auto &arg : F.args())
		{
			bool process_arg = false;

			const Type *curr_type = arg.getType();

			if (arg.getType()->isPointerTy() || arg.getType()->isArrayTy())
			{
				if (getBaseTypeFromPointerOrArray(arg.getType()))
				{
					curr_type = getBaseTypeFromPointerOrArray(arg.getType());
				}
			}

			if (curr_type->isFloatTy() || curr_type->isDoubleTy())
			{
				process_arg = true;
				// errs() << "Processing arg '" << arg << "'\n";
			}

			if (process_arg)
			{
				process_arg_root_node(arg);
			}
			else
			{
				// errs() << "Skipping arg '" << arg << "' since it isn't the correct type (float/double/float*/etc)\n";
			}
		}

		// todo: __shared__ variables are treated as GlobalVariables in LLVM and don't count in this code
		const auto vars_in_function = F.getSubprogram()->getVariables();
		vector <const DILocalVariable *> localvar_order;
		for (const auto &bb : F)
		{
			for (const auto &ins : bb)
			{
				if (auto dbg_info = dyn_cast<DbgInfoIntrinsic>(&ins))
				{
					// errs() << ins << "\n";
					const DILocalVariable *var_initial;
					if (isa<DbgDeclareInst>(dbg_info))
					{
						var_initial = dyn_cast<DbgDeclareInst>(dbg_info)->getVariable();
					}
					else if (isa<DbgValueInst>(dbg_info))
					{
						var_initial = dyn_cast<DbgValueInst>(dbg_info)->getVariable();
					}
					else
					{
						my_throw("Unhandled type of DbgInfoIntrinsic (maybe DbgAddrIntrinsic?)");
					}

					if (find(vars_in_function.begin(), vars_in_function.end(), var_initial) == vars_in_function.end())
					{
						// Non local variable for *this* function
						// errs() << "Var not local to function: " << var_initial->getName() << "\n";
						continue;
					}

					// errs() << "\n" << ins << "\n";

					// Skip function args:
					const Value *localvar_val = dbg_info->getVariableLocation();

					// skipping if it is an argument
					if (isa<Argument>(localvar_val))
					{
						assert(dyn_cast<Argument>(localvar_val)->getParent() == &F);
						// errs() << "Var is function arg: " << var_initial->getName() << "\n";
						continue;
					}

					// assign type:
					const Type *localvar_type = localvar_val->getType();
					assert(localvar_type);
					bool process_localvar = false;

					if (localvar_type->isPointerTy() || localvar_type->isArrayTy())
					{
						localvar_type = getBaseTypeFromPointerOrArray(localvar_type);
						assert(localvar_type);
					}
					if (localvar_type->isDoubleTy())
					{
						process_localvar = true;
					}
					else if (localvar_type->isFloatTy())
					{
						process_localvar = true;
					}
					else
					{
						// errs() << "Var is non float/double type: " << var_initial->getName() << "\n";
						// errs() << "Ignoring non float/double local var: " << *localvar_val << "\n";
					}

					// // skipping literals
					// if (process_localvar && not isa<Instruction>(localvar_val))
					// {
					// 	continue;
					// }

					if (process_localvar)
					{
						// errs() << "Parent: " << var_initial->getName() << "\n";
						// errs() << "Children: " << *localvar_val << "\n";
						localvar_map[var_initial].insert(localvar_val);		// probably not needed unless you wanna change types of local variables
						if (localvar_map2.find(var_initial) == localvar_map2.cend())
						{
							localvar_map2[var_initial] = &ins;			// stores first instruction which maps to this variable
							localvar_order.push_back(var_initial);		// so that they are processed in order
						}
					}
				}
			}
		}

		for (const auto &i : localvar_order)
		{
			// errs() << "Localvar: " << i->getName() << "\n";
			// for (const auto &j : localvar_map[i])
			// {
			// 	errs() << "\t" << *j << "\n";
			// 	for (const auto &k : j->users())
			// 	{
			// 		errs() << "\t\t" << *k << "\n";
			// 	}
			// }
			process_localver_root_node(*i);
		}

		model.bbdata = vector<STAModel::BBData>(bbmap.size());
		assert(model.bbdata.size() == model.bbgraph.size());
		
		for (const auto &BB : F)
		{
			auto bbidx = getBbmapIndex(&BB);

			for (const auto &I : BB)
			{
				if (find(insts_in_trees.cbegin(), insts_in_trees.cend(), &I) != insts_in_trees.cend())
				{
					// implies instruction depends on arg
					continue;
				}

				using namespace llvm;
				if (isa<AllocaInst>(&I))
				{
					// // temp
					// if (I.getType()->isDoubleTy())
					// {
					// 	errs() << "ALLOCA DOUBLE\n";
					// }
					// else if (I.getType()->isFloatTy())
					// {
					// 	errs() << "ALLOCA FLOAT\n";
					// }
					// else if (I.getType()->isVoidTy())
					// {
					// 	errs() << "ALLOCA VOID\n";
					// }
					// else if (I.getType()->isPointerTy())
					// {
					// 	errs() << "ALLOCA POINTER\n";
					// }
					// else 
					// {
					// 	errs() << I << "\n";
					// 	errs() << "ALLOCA OTHER\n";
					// 	errs() << "ALLOCA TYPE: " << I.getType()->getTypeID() << "\n";
					// }
					++model.bbdata[bbidx].fixed_inst_count["alloca"];
				}
				else if (isa<LoadInst>(&I))
				{
					++model.bbdata[bbidx].fixed_inst_count["load"];
				}
				else if (isa<StoreInst>(&I))
				{
					++model.bbdata[bbidx].fixed_inst_count["store"];
				}
				else if (isa<BinaryOperator>(&I))
				{
					const BinaryOperator *bo = dyn_cast<BinaryOperator>(&I);
					// check further
					auto opcode = bo->getOpcode();
					switch (opcode)
					{
						case Instruction::Add:
						{
							++model.bbdata[bbidx].fixed_inst_count["add"];
							break;
						}
						case Instruction::Sub:
						{
							++model.bbdata[bbidx].fixed_inst_count["sub"];
							break;
						}
						case Instruction::Mul:
						{
							++model.bbdata[bbidx].fixed_inst_count["mul"];
							break;
						}
						case Instruction::UDiv:
						{
							++model.bbdata[bbidx].fixed_inst_count["udiv"];
							break;
						}
						case Instruction::SDiv:
						{
							++model.bbdata[bbidx].fixed_inst_count["sdiv"];
							break;
						}
						case Instruction::URem:
						{
							++model.bbdata[bbidx].fixed_inst_count["urem"];
							break;
						}
						case Instruction::SRem:
						{
							++model.bbdata[bbidx].fixed_inst_count["srem"];
							break;
						}
						case Instruction::And:
						{
							++model.bbdata[bbidx].fixed_inst_count["and"];
							break;
						}
						case Instruction::Or:
						{
							++model.bbdata[bbidx].fixed_inst_count["or"];
							break;
						}
						case Instruction::FAdd:
						{
							// todo
							if (I.getType()->isFloatTy())
							{
								++model.bbdata[bbidx].fixed_inst_count["f_fadd"];
							}
							else if (I.getType()->isDoubleTy())
							{
								assert(I.getType()->isDoubleTy());
								++model.bbdata[bbidx].fixed_inst_count["d_fadd"];
							}
							break;
						}
						case Instruction::FSub:
						{
							// todo
							if (I.getType()->isFloatTy())
							{
								++model.bbdata[bbidx].fixed_inst_count["f_fsub"];
							}
							else if (I.getType()->isDoubleTy())
							{
								assert(I.getType()->isDoubleTy());
								++model.bbdata[bbidx].fixed_inst_count["d_fsub"];
							}
							break;
						}
						case Instruction::FMul:
						{
							// todo
							if (I.getType()->isFloatTy())
							{
								++model.bbdata[bbidx].fixed_inst_count["f_fmul"];
							}
							else if (I.getType()->isDoubleTy())
							{
								assert(I.getType()->isDoubleTy());
								++model.bbdata[bbidx].fixed_inst_count["d_fmul"];
							}
							break;
						}
						case Instruction::FDiv:
						{
							// todo
							if (I.getType()->isFloatTy())
							{
								++model.bbdata[bbidx].fixed_inst_count["f_fdiv"];
							}
							else if (I.getType()->isDoubleTy())
							{
								assert(I.getType()->isDoubleTy());
								++model.bbdata[bbidx].fixed_inst_count["d_fdiv"];
							}
							break;
						}
						case Instruction::FRem:
						{
							// todo
							if (I.getType()->isFloatTy())
							{
								++model.bbdata[bbidx].fixed_inst_count["f_frem"];
							}
							else if (I.getType()->isDoubleTy())
							{
								assert(I.getType()->isDoubleTy());
								++model.bbdata[bbidx].fixed_inst_count["d_frem"];
							}
							break;
						}
						default:
						{
							if (!bo->isShift(bo->getOpcode()))	// fixme: const problem?
							{
								errs() << "OTHER INST: ";
								I.print(errs());
								errs() << "\n";
							}
							++model.bbdata[bbidx].fixed_inst_count["other"];
						}
					}
				}
				else if (isa<ICmpInst>(&I))
				{
					++model.bbdata[bbidx].fixed_inst_count["icmp"];
				}
				else if (isa<FCmpInst>(&I))
				{
					// todo
					if (I.getType()->isFloatTy())
					{
						++model.bbdata[bbidx].fixed_inst_count["f_fcmp"];
					}
					else if (I.getType()->isDoubleTy())
					{
						assert(I.getType()->isDoubleTy());
						++model.bbdata[bbidx].fixed_inst_count["d_fcmp"];
					}
				}
				else if (isa<BranchInst>(&I))
				{
					assert(I.getType()->isVoidTy());	// temp
					++model.bbdata[bbidx].fixed_inst_count["br"];
				}
				else if (isa<ReturnInst>(&I))
				{
					assert(I.getType()->isVoidTy());	// temp
					++model.bbdata[bbidx].fixed_inst_count["ret"];
				}
				else if (isa<SwitchInst>(&I))
				{
					++model.bbdata[bbidx].fixed_inst_count["switch"];
				}
				else if (isa<CallInst>(&I))
				{
					++model.bbdata[bbidx].fixed_inst_count["call"];
				}
				else if (isa<BitCastInst>(&I))
				{
					// ignore cos noop?
				}
				else if (isa<GetElementPtrInst>(&I))
				{
					// probably not noop at runtime
					++model.bbdata[bbidx].fixed_inst_count["getelementptr"];
				}
				else if (isa<FPTruncInst>(&I))
				{
					// todo
					++model.bbdata[bbidx].fixed_inst_count["fptrunc"];
				}
				else if (isa<FPExtInst>(&I))
				{
					// todo
					++model.bbdata[bbidx].fixed_inst_count["fpext"];
				}
				else if (isa<CastInst>(&I))
				{
					// other cast instruction
					// must be checked after fpext and fptrunc since they are cast instructions too
					assert(!isa<FPTruncInst>(&I) && !isa<FPExtInst>(&I));
					++model.bbdata[bbidx].fixed_inst_count["cast"];
				}
				else if (isa<PHINode>(&I))
				{
					++model.bbdata[bbidx].fixed_inst_count["phi"];
				}
				else
				{
					errs() << "OTHER INST: ";
					I.print(errs());
					errs() << "\n";
					++model.bbdata[bbidx].fixed_inst_count["other"];
				}
			}
		}

		// Additional check to prevent cycles:
		for (size_t i = 0; i < model.mytree_nodes.size(); ++i)
		{
			if (model.mytree_nodes[i].inst and isa<Instruction>(model.mytree_nodes[i].inst))
			{
				auto parent = dyn_cast<Instruction>(model.mytree_nodes[i].inst);

				auto children_indices = model.mytree_nodes[i].children;		// creating copy for loop iteration since loop modifies children
				for (const auto &j : children_indices)
				{
					assert(isa<Instruction>(model.mytree_nodes[j].inst));
					auto child = dyn_cast<Instruction>(model.mytree_nodes[j].inst);

					if ((not isPotentiallyReachable(parent, child, &dom_tree)) or dom_tree.dominates(child, parent))
					{
						size_t num_erased = model.mytree_nodes[i].children.erase(j);
						assert(num_erased == 1);

						num_erased = model.mytree_nodes[j].parents.erase(i);
						assert(num_erased == 1);

						// cerr << "Removed " << i << " --> " << j << "\n";
					}
				}
			}
		}
		
		return false;
	}
	
	void print(raw_ostream &O, const Module *M) const override
	{
		const auto &F = *curr_func;

		cout << "Generating output...\n";
		ofstream fout(F.getName());
		fout << model;
		// cout << model;
		fout.close();

		cout << "Reading generated output...\n";
		ifstream fin(F.getName());
		STAModel model2;
		if (fin.is_open())	// since some files aren't created due to long filenames
		{
			fin >> model2;
		}
		fin.close();

		if (true)
		{
			if (F.getName() == "func1")	// only trigger for func1
			{
				// cout << "Predict DDDD: " << model.predict(vector<mytype>({mytype::Double, mytype::Double}), vector<mytype>({mytype::Double, mytype::Double})) << "\n";
				cout << "Predict FFFF: " << model.predict(vector<mytype>({mytype::Float, mytype::Float}), vector<mytype>({mytype::Float, mytype::Float})) << "\n";
			}
			else if (F.getName() == "func2")
			{
				// cout << "Predict DF:\n";
				// model.predict(vector<mytype>({mytype::Double, mytype::Float}));
				// cout << "\n";

				// cout << "Predict FD:\n";
				// model.predict(vector<mytype>({mytype::Float, mytype::Double}));
				// cout << "\n";

				// cout << "Predict FF:\n";
				// model.predict(vector<mytype>({mytype::Float, mytype::Float}));
				// cout << "\n";

				// cout << "Predict DD:\n";
				// model.predict(vector<mytype>({mytype::Double, mytype::Double}));
				// cout << "\n";
			}
			else if (F.getName() == "func3")
			{
				// cout << "Predict F:\n";
				// model.predict(vector<mytype>({mytype::Float}));
				// cout << "\n";

				// cout << "Predict D:\n";
				// model.predict(vector<mytype>({mytype::Double}));
				// cout << "\n";
			}
			else if (F.getName() == "func4")
			{
				// cout << "Predict DDDDDD: " << model.predict(vector<mytype>({mytype::Double, mytype::Double}), vector<mytype>({mytype::Double, mytype::Double, mytype::Double, mytype::Double})) << "\n";
				cout << "Predict FFFFFF: " << model.predict(vector<mytype>({mytype::Float, mytype::Float}), vector<mytype>({mytype::Float, mytype::Float, mytype::Float, mytype::Float})) << "\n";
			}
			else if (F.getName() == LUL_FUN_NM)
			{
				auto arg = vector<mytype>(19, mytype::Double);
				auto loc = vector<mytype>(14, mytype::Double);
				cout << "Predict D: " << model.predict(arg, loc) << "\n";
				auto arg2 = {mytype::Float, mytype::Float, mytype::Double, mytype::Float, mytype::Float, mytype::Double, mytype::Double, mytype::Double, mytype::Float, mytype::Float, mytype::Double, mytype::Double, mytype::Double, mytype::Double, mytype::Double, mytype::Float, mytype::Float, mytype::Float, mytype::Float};
				auto loc2 = {mytype::Float, mytype::Double, mytype::Float, mytype::Double, mytype::Double, mytype::Float, mytype::Double, mytype::Double, mytype::Float, mytype::Float, mytype::Float, mytype::Double, mytype::Double, mytype::Float};
				cout << "Predict C: " << model.predict(arg2, loc2) << "\n";
			}
		}

		cout << "# args: " << model.arg_trees.size() << "\n";
		cout << "# local: " << model.localvar_trees.size() << "\n";

		cout << "================================================================\n";
	}
}; // end of struct STAAnalysis
}	// end of anonymous namespace

char STAAnalysis::ID = 0;
static RegisterPass<STAAnalysis> X("STA", "STAAnalysis Pass", false /* Only looks at CFG */, true /* Analysis Pass */);

#define NDEBUG OLD_NDEBUG
