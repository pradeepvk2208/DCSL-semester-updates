#include "STAModel.hpp"

#include <iostream>
#include <fstream>
#define OLD_NDEBUG NDEBUG
#undef NDEBUG
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <exception>
#include <cmath>

#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace std;

#define v1 CalcKinematicsAndMonotonicQGradient_kernel
#define v1m "_Z42CalcKinematicsAndMonotonicQGradient_kerneliidPKiPKdS2_S2_S2_S2_S2_S2_S2_PdS3_S3_S3_S3_S3_S3_S3_S3_S3_S3_S3_S3_Pii"
namespace
{
    // 0   deltatime_h
    // 1   volo
    // 2   v
    // 3   x
    // 4   y   // same type as x
    // 5   z   // x
    // 6   xd  // x
    // 7   yd  // x
    // 8   zd  // x
    // 9   vnew
    // 10  delv
    // 11  arealg
    // 12  dxx
    // 13  dyy
    // 14  dzz
    // 15  vdov
    // 16  delx_zeta
    // 17  delv_zeta
    // 18  delx_xi
    // 19  delv_xi
    // 20  delx_eta
    // 21  delv_eta

    // 0	B
    // 1	x_local
    // 2	y_local
    // 3	z_local
    // 4	xd_local
    // 5	yd_local
    // 6	zd_local
    // 7	D
    // 8	volume
    // 9	relativeVolume
    // 10	dt2
    // 11	detJ
    // 12	vdovNew
    // 13	vdovthird
    // 14	vol
}
vector<string> v1_arg_types = {"deltatime_h", "volo", "v", "x", "x", "x", "x", "x", "x", "v", "delv", "arealg", "dxx", "dyy", "dzz", "vdov", "x", "x", "x", "x", "x", "x"};
vector<string> v1_local_types = {"B", "x", "x", "x", "x", "x", "x", "D", "volume", "volume", "deltatime_h", "volume", "vdov", "vdov", "volo"};

#define v2 CalcVolumeForceForElems_kernel
#define v2m "_Z30CalcVolumeForceForElems_kernelILb0EEvPKdS1_S1_S1_diiPKiS1_S1_S1_S1_S1_S1_S1_S1_PdS4_S4_Pii"
namespace
{
    // 0   volo
    // 1   v
    // 2   p
    // 3   q
    // 4   hgcoef // type: hourg
    // 5   ss
    // 6   elemMass
    // 7   x
    // 8   y   // same type as x
    // 9   z   // x
    // 10  xd  // x
    // 11  yd  // x
    // 12  zd  // x
    // 13  fx_elem
    // 14  fy_elem
    // 15  fz_elem

    // 0	xn
    // 1	yn
    // 2	zn
    // ??   xdn
    // ??   ydn
    // ??   zdn
    // 3	dvdxn
    // 4	dvdyn
    // 5	dvdzn
    // 6	hgfx
    // 7	hgfy
    // 8	hgfz
    // 9	hourgam
    // ??   coefficient
    // 10	volume
    // 11	det
    // ??   ss1
    // ??   mass1
    // 12	sigxx
    // ??   volinv
    // ??   volume13
    // 13	B
}
vector<string> v2_arg_types = {"volo", "v", "p", "q", "hourg", "ss", "elemMass", "x", "x", "x", "x", "x", "x", "fx_elem", "fy_elem", "fz_elem"};
vector<string> v2_local_types = {"x", "x", "x", /*"dn", "dn", "dn",*/ "dvd", "dvd", "dvd", "hg", "hg", "hg", "hourg", /*"coeff",*/ "volume", "volume", /*"ss", "elemMass",*/ "sig", /*"volume", "volume",*/ "B"};

#define v3 ApplyMaterialPropertiesAndUpdateVolume_kernel
#define v3m "_Z45ApplyMaterialPropertiesAndUpdateVolume_kernelidddPdS_S_S_dddddPiS_S_S_S_dS_dS0_iPKiS2_i"
namespace
{
    // 0   refdens          f
    // 1   e_cut            f
    // 2   emin             d
    // 3   ql               f
    // 4   qq               f
    // 5   vnew             d
    // 6   v                d
    // 7   pmin             d
    // 8   p_cut            f
    // 9   q_cut            f
    // 10  eosvmin          d
    // 11  eosvmax          d
    // 12  e                d
    // 13  delv             d
    // 14  p                d
    // 15  q                f
    // 16  ss4o3            f
    // 17  ss               f
    // 18  v_cut            f

    // 0	work            f
    // 1	vnewc           d
    // 2	compression     f
    // 3	delvc           d
    // 4	vchalf          d
    // 5	compHalfStep    f
    // 6	p_old           d
    // 7	e_old           d
    // 8	q_old           f
    // 9	qq_old          f
    // 10	ql_old          f
    // 11	p_new           d
    // 12	e_new           d
    // 13	q_new           f
    // ??   p_temp
    // ??   e_temp
    // ??   q_temp
    // ??   qq_temp
    // ??   ql_temp
    // ??   delvc_temp
    // ??   bvc
    // ??   pbvc
}
vector<string> v3_arg_types = {"refdens", "e_cut", "emin", "ql", "qq", "v", "v", "pmin", "p_cut", "q_cut", "eosvmin", "eosvmax", "e", "delv", "p", "q", "ss4o3", "ss", "v_cut"};
vector<string> v3_local_types = {"work", "v", "comp", "delv", "v", "comp", "p", "e", "q", "qq", "ql", "p", "e", "q"/*, "p", "e", "q", "qq", "ql", "delv", "v", "v"*/};

// combined
vector<string> v_types = {"work", "sig", "hg", "dvd", "v_cut", "ss4o3", "pmin", "qq", "e_cut", "eosvmin", "refdens", "fz_elem", "fy_elem", "D", "elemMass", "delv_xi", "hourg", "p_cut", "q", "delv_eta", "volo", "delx_xi", "ss", "q_cut", "v", "delx_eta", "B", "delv_zeta", "vdov", "delx_zeta", "volume", "emin", "dyy", "comp", "fx_elem", "dxx", "dzz", "x", "e", "arealg", "eosvmax", "delv", "vnew", "ql", "p", "deltatime_h",
"dn", "coeff", "ssc", "qtilde"};

#define CORRECT_ENERGY_VAL 5.124778e+05     // verify

using hash_type = uint64_t;

unordered_map<hash_type, double> lut = {};

template <typename T>   // T should be container of strings
hash_type getHash(const T &val, const vector<string> &ref)
{
    hash_type hash = 0;
    for (const string &i : val)
    {
        auto it = find(ref.cbegin(), ref.cend(), i);
        if (it != ref.cend())
        {
            size_t idx = it - ref.cbegin();
            if (idx > 63)
            {
                my_throw("Hash can only work with upto 64 bits");
            }
            hash |= (1 << idx);
        }
        else
        {
            cerr << "String: \"" << i << "\"\n";
            my_throw("Searched value doesn't exist in reference");
        }
    }
    return hash;
}

void parse_csv(ifstream &csv)
{
    string record;
    while (getline(csv, record))
    {
        // cerr << record << "\n";

        assert(count(record.cbegin(), record.cend(), ',') == 3);
        // auto it1 = find(record.cbegin(), record.cend(), ',');
        auto it1 = record.find_first_of(',');
        // string var_types = record.substr(0, it1 - record.cbegin());
        string var_types = record.substr(0, it1);

        unordered_set<string> hash_set = {};

        // converting var_types to unordered_set
        // auto it2 = var_types.cbegin();
        size_t it2 = 0;
        do
        {
            auto it3 = var_types.find_first_of(' ', it2);

            string var_type;
            if (it3 != string::npos)
            {
                var_type = var_types.substr(it2, it3 - it2);
            }
            else
            {
                var_type = var_types.substr(it2);
            }

            // cerr << var_type << "\n";

            if (var_type != "NULL")
            {
                assert(var_type.find_first_of("Real_t_") == 0);
                hash_set.insert(var_type.substr(7));    // Skipping first 7 characters: "Real_t_"
                // cerr << var_type.substr(7) << "\n";
            }

            it2 = ++it3;
        }
        while (it2 < var_types.size());

        assert(hash_set.size() == 1);

        // cerr << "getHash in parse_csv():\n";
        hash_type hash = getHash(hash_set, v_types);    // todo: set reference set later

        auto it4 = record.find_last_of(','); // verify
        ++it4;

        string energy_str = record.substr(it4);
        double energy = stod(energy_str);
        // cerr << energy << "\n";

        lut[hash] = abs((energy - CORRECT_ENERGY_VAL) / CORRECT_ENERGY_VAL);
        // cerr << hash << ": " << lut[hash] << "\n";
    }
}

double error_pred(const vector<mytype> &types_arg, const vector<mytype> &types_local, const vector<string> *vX_types_arg, const vector<string> *vX_types_local)
{
    assert(types_arg.size() == vX_types_arg->size());
    assert(types_local.size() == vX_types_local->size());

    unordered_set<string> changed_types;

    // give val
    // hash_type hash = 0;
    double err = 0;
    for (size_t i = 0; i < types_arg.size(); ++i)
    {
        if (types_arg[i] == mytype::Float)
        {
            changed_types.insert((*vX_types_arg)[i]);
        }
    }

    for (size_t i = 0; i < types_local.size(); ++i)
    {
        if (types_local[i] == mytype::Float)
        {
            changed_types.insert((*vX_types_local)[i]);
        }
    }

    for (const auto &i : changed_types)
    {
        vector<string> t0 = {};
        t0.push_back(i);
        // cerr << "getHash in error_pred():\n";
        err += lut[getHash(t0, v_types)];
    }

    // use hash for table lookup?

    return err;
}

void generate_v_types_combined()
{
    unordered_set<string> temp;
    temp.insert(v1_arg_types.cbegin(), v1_arg_types.cend());
    temp.insert(v2_arg_types.cbegin(), v2_arg_types.cend());
    temp.insert(v3_arg_types.cbegin(), v3_arg_types.cend());
    temp.insert(v1_local_types.cbegin(), v1_local_types.cend());
    temp.insert(v2_local_types.cbegin(), v2_local_types.cend());
    temp.insert(v3_local_types.cbegin(), v3_local_types.cend());
    for (const auto &i : temp)
    {
        cerr << i << "\n";
    }
}

// arg[1] determins kernel, 1, 2, or 3
// arg[2...] determine types
int main(int argc, char *argv[])
{
    // generate_v_types_combined();

    if (argc < 2)
    {
        cerr << "Needs 2+ arguments\n";
        exit(EXIT_FAILURE);
    }

    int ker = atoi(argv[1]);    // potentially unneeded?

    size_t num_arg, num_local;

    vector<string> *vX_types_arg;
    vector<string> *vX_types_local;
    switch (ker)
    {
        case 1:
        {
            vX_types_arg = &v1_arg_types;
            vX_types_local = &v1_local_types;
            num_arg = 22;
            num_local = 15;
            break;
        }
        case 2:
        {
            vX_types_arg = &v2_arg_types;
            vX_types_local = &v2_local_types;
            num_arg = 16;
            num_local = 14;
            break;
        }
        case 3:
        {
            vX_types_arg = &v3_arg_types;
            vX_types_local = &v3_local_types;
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

    // auto test_types = vector<mytype>({mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float, mytype::Float});

    ifstream csv("../Opt_results.csv");

    parse_csv(csv);
    
    // cerr << "Arg: " << arg_types.size() << " (read), " << vX_types_arg->size() << " (expected)\n";
    // cerr << "Local: " << localvar_types.size() << " (read), " << vX_types_local->size() << " (expected)\n";
    cout << "Error: " << scientific << error_pred(arg_types, localvar_types, vX_types_arg, vX_types_local) << "\n";

    exit(EXIT_SUCCESS);
}

#define NDEBUG OLD_NDEBUG