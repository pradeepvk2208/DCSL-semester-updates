#include "STAModel.hpp"
#include <iostream>
#include <fstream>
#undef NDEBUG
#include <cassert>
#include <cstdlib>

#include <vector>
using namespace std;

#define v1m "_Z42CalcKinematicsAndMonotonicQGradient_kerneliidPKiPKdS2_S2_S2_S2_S2_S2_S2_PdS3_S3_S3_S3_S3_S3_S3_S3_S3_S3_S3_S3_Pii"
#define v2m "_Z30CalcVolumeForceForElems_kernelILb0EEvPKdS1_S1_S1_diiPKiS1_S1_S1_S1_S1_S1_S1_S1_PdS4_S4_Pii"
#define v3m "_Z45ApplyMaterialPropertiesAndUpdateVolume_kernelidddPdS_S_S_dddddPiS_S_S_S_dS_dS0_iPKiS2_i"
#define v4m "_Z29AddNodeForcesFromElems_kerneliiPKiS0_S0_PKdS2_S2_PdS3_S3_i"
#define v5m "_Z35CalcMonotonicQRegionForElems_kerneldddddiPiS_S_S_S_S_S_S_PdS0_S0_S0_S0_S0_S0_S0_S0_S0_S0_S0_S0_dS_"

// arg[1] determins kernel, 1, 2, or 3
// arg[2...] determine types
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Needs 2+ arguments\n";
        exit(EXIT_FAILURE);
    }

    int ker = atoi(argv[1]);

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
        case 4:
        {
            fin.open("../" v4m);
            num_arg = 6;
            num_local = 3;
            break;
        }
        case 5:
        {
            fin.open("../" v5m);
            num_arg = 19;
            num_local = 12;
            break;
        }
        
        default:
        {
            cerr << "Unknown kernel\n";
            exit(EXIT_FAILURE);
        }
    }

    if (argc != num_arg + num_local + 2)
    {
        cerr << "Wrong number of types. Need " << num_arg << " + "<< num_local << ", got " << argc - 2 << "\n";
        exit(EXIT_FAILURE);
    }

    vector<mytype> arg_types, localvar_types;
    
    auto create_vec = [&argv] (vector<mytype> &test_types, size_t n, size_t offset)
    {
        for (size_t i = offset; i < offset + n; ++i)
        {
            switch (argv[i][0])
            {
                case 'd':
                case 'D':
                {
                    test_types.push_back(mytype::Double);
                    break;
                }
                case 'f':
                case 'F':
                {
                    test_types.push_back(mytype::Float);
                    break;
                }
                default:
                {
                    cerr << "Unknown argument type: '" << argv[i][0] << "'\n";
                    exit(EXIT_FAILURE);
                }
            }
        }
    };

    create_vec(arg_types, num_arg, 2);
    create_vec(localvar_types, num_local, 2 + num_arg);
    assert(arg_types.size() == num_arg);
    // cout << arg_types.size() << " " << localvar_types.size() << "\n";
    assert(localvar_types.size() == num_local);

    assert(fin.is_open());
    fin >> model;
    fin.close();

    // auto test_types = vector<mytype>({mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float});

    int score = model.predict(arg_types, localvar_types);

    cout << "Score: " << score << "\n";

    exit(EXIT_SUCCESS);
}