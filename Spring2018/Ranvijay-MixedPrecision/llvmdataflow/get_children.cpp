#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unordered_set>
#include <cassert>
#include <algorithm>
#include <queue>

#include "STAModel.hpp"

#define v1m "_Z42CalcKinematicsAndMonotonicQGradient_kerneliidPKiPKdS2_S2_S2_S2_S2_S2_S2_PdS3_S3_S3_S3_S3_S3_S3_S3_S3_S3_S3_S3_Pii"
#define v2m "_Z30CalcVolumeForceForElems_kernelILb0EEvPKdS1_S1_S1_diiPKiS1_S1_S1_S1_S1_S1_S1_S1_PdS4_S4_Pii"
#define v3m "_Z45ApplyMaterialPropertiesAndUpdateVolume_kernelidddPdS_S_S_dddddPiS_S_S_S_dS_dS0_iPKiS2_i"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Needs 2 arguments: [ker] [root]\n";
        exit(EXIT_FAILURE);
    }

    int ker = atoi(argv[1]);
    size_t root = atoi(argv[2]);

    STAModel model;
    ifstream fin;
    size_t num_arg, num_local;

    switch (ker)
    {
        case 1:
        {
            fin.open("../" v1m);
            num_arg = 22;
            num_local = 15;
            break;
        }
        case 2:
        {
            fin.open("../" v2m);
            num_arg = 16;
            num_local = 14;
            break;
        }
        case 3:
        {
            fin.open("../" v3m);
            num_arg = 19;
            num_local = 14;
            break;
        }
        default:
        {
            cerr << "Unknown kernel\n";
            exit(EXIT_FAILURE);
        }
    }

    fin >> model;

    vector<size_t> &vec = root < num_arg ? model.arg_trees : model.localvar_trees;
    if (root < num_arg)
    {
        // vec = model.arg_trees;
    }
    else
    {
        assert(root - num_arg < num_local);
        // vec = model.localvar_trees;
        root -= num_arg;
    }

    assert(root < vec.size());

    // if (find(model.arg_trees.cbegin(), model.arg_trees.cend(), root) == model.arg_trees.cend() and find(model.localvar_trees.cbegin(), model.localvar_trees.cend(), root) == model.localvar_trees.cend())
    // {
    //     cerr << "[root] isn't root node\n";
    //     exit(EXIT_FAILURE);
    // }

    root = vec[root];

    auto children = [&model] (size_t root)
    {
        unordered_set<size_t> chil;

        queue<size_t> to_process;
        to_process.push(root);

        while (not to_process.empty())
        {
            size_t curr_idx = to_process.front();
            to_process.pop();
            mytree_node &node = model.mytree_nodes[curr_idx];
            for (const auto &child_idx : node.children)
            {
                chil.insert(child_idx);
                to_process.push(child_idx);
            }
        }

        return chil;
    };

    for (const auto &i : children(root))
    {
        cout << i << " ";
    }
    cout << "\n";

    auto 
    
    exit(EXIT_SUCCESS);
}