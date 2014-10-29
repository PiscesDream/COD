//    Shaofan Lai, a LC2K-assembler
//    Copyright (C) 2014  Shaofan Lai

//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <fstream>

#include <stdexcept>

#include <string>
#include <vector>
#include <set>
#include <map>

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <sstream>

using std::vector;
using std::string;
using std::map;
using std::set;

using std::ostream;
using std::ofstream;
using std::ifstream;
using std::stringstream;

using std::cerr;
using std::endl;
using std::cout;
using std::cin;

// Configuration
typedef int mc_t;
const int REG_COUNT = 8;
const int MEM_MAX = 0x7fff; 
const int MEM_MIN = -0x8000;

#define DEBUG(X) cout << "Debug:" << (X) << endl;
#define DEBUGH(N) printf("Debug:%X\n", N);

class SyntaxError: public std::runtime_error
{
 public:
    explicit SyntaxError(const string &s): runtime_error(s) {}
};

class IOError: public std::runtime_error
{
 public:
    explicit IOError(const string &s): runtime_error(s) {}
};

class Assembler
{
    struct Ins
    {
        string label;
        string ope;
        vector<string> fields;
    };

 private:
    // Instruction sets
    const set<string> _ALL_INS {"add", "nand", "lw", "sw", "beq",
                                "jalr", "noop", "halt", ".fill"};
    const set<string> _R_INS {"add", "nand"};
    const set<string> _I_INS {"lw", "sw", "beq"};
    const set<string> _J_INS {"jalr"};
    const set<string> _O_INS {"halt", "noop"};
    const set<string> _DIR_INS {".fill"};
    const map<string, mc_t> _INS_MAP {{"add", 0}, {"nand", 1},
                                      {"lw", 2}, {"sw", 3},
                                      {"beq", 4}, {"jalr", 5},
                                      {"halt", 6}, {"noop", 7}};

    // Encoding functions
    inline mc_t encode_R(const Ins &);
    inline mc_t encode_I(const Ins &, int);
    inline mc_t encode_J(const Ins &);
    inline mc_t encode_O(const Ins &);
    inline mc_t encode_DIR(const Ins &);

    // Utilities functions
    mc_t get_register(const string &);
    mc_t get_offset(const string &, const int pc = -1);

    // Storage
    vector<string> _asm;
    vector<mc_t> _mc;
    vector<Ins> _ins;

    // Auxiliary data
    map<string, int> _labels;

 public:
    // Encoding procedure
    void reset();
    void import(const vector<string> &);
    void encode();

    // Code transfer
    string get_mc();  
    void loadFromFile(const string &);
    void saveToFile(const string &);
    void output2stream(ostream &s, char mode = 'D');  // D:dec H:Hex

    // Debug tools
    void test();
    void pprint(const string &str);
    string dec2bin(const mc_t code);
};

mc_t Assembler::get_register(const string &reg_name)
{
    int reg = atoi(reg_name.c_str());
    if (reg < 0 || reg >= REG_COUNT || (!reg && reg_name != "0"))
        throw SyntaxError("Invalid register: " + reg_name);
    return reg & 0x00000007;
}

mc_t Assembler::encode_R(const Ins &ins)
{
    mc_t code = _INS_MAP.at(ins.ope),
         regA = get_register(ins.fields[0]),
         regB = get_register(ins.fields[1]),
         destReg = get_register(ins.fields[2]);
    return (code << 22) | (regA << 19) | (regB << 16) | destReg;
}

mc_t Assembler::get_offset(const string &jmp, const int pc)
{
    int offset = atoi(jmp.c_str());
    //range test ?
    if(offset == 0)
        if (jmp!= "0")
            try
            {
                offset = _labels.at(jmp);
                if (pc != -1)
                    offset = offset - pc - 1; 
            }
            catch (std::runtime_error e)
            {
                throw SyntaxError("Invalid label: " + jmp);
            }
        else
            throw SyntaxError("Invalid jumping: " + jmp);
        else
            if (offset > MEM_MAX || offset < MEM_MIN)
                throw SyntaxError("Offset out of range: " + jmp);

    return offset & 0x0000ffff;
}

mc_t Assembler::encode_I(const Ins &ins, const int pc)
{
    mc_t code = _INS_MAP.at(ins.ope),
         regA = get_register(ins.fields[0]),
         regB = get_register(ins.fields[1]);
    int offset = ins.ope=="beq"?get_offset(ins.fields[2], pc):get_offset(ins.fields[2]);
    return (code << 22) | (regA << 19) | (regB << 16) | offset;
}

mc_t Assembler::encode_J(const Ins &ins)
{
    mc_t code = _INS_MAP.at(ins.ope),
         regA = get_register(ins.fields[0]),
         regB = get_register(ins.fields[1]);
    return (code << 22) | (regA << 19) | (regB << 16);
}

mc_t Assembler::encode_O(const Ins &ins)
{
    mc_t code = _INS_MAP.at(ins.ope);
    return (code << 22);
}

mc_t Assembler::encode_DIR(const Ins &ins)
{
    int data = atoi(ins.fields[0].c_str());
    if (!data && ins.fields[0] != "0")
        try
        {
            data = _labels.at(ins.fields[0]);
        }
        catch (std::runtime_error e)
        {
            throw SyntaxError("Invalid label: " + ins.fields[0]);
        }

    return data & 0xfffffffff;
}


void Assembler::reset()
{
    _asm.clear();
    _mc.clear();
    _ins.clear();
    _labels.clear();
}

void Assembler::loadFromFile(const string &filename)
{
    reset();
    ifstream input(filename.c_str());
    if (!input)
        throw IOError("Can not open file: " + filename);

    string tmp;
    while (input)
    {
        getline(input, tmp);
        if (tmp == "") continue;
        _asm.push_back(tmp);
    }
    cout << _asm.size() << endl;
}

void Assembler::import(const vector<string> &new_codes)
{
    reset();
    _asm = new_codes;
}

void Assembler::encode()
{
    //first scan    
    string temp;
    int pc = 0x00000000;
    for (auto &s: _asm)
        try
        {
            Ins ins;

            stringstream buffer(s);
            buffer.exceptions(std::stringstream::failbit);

            buffer >> temp; 
            if (_ALL_INS.count(temp))
                ins.ope = temp;
            else
            {
                ins.label = temp;
                buffer >> ins.ope;
            }

            if (_R_INS.count(ins.ope) || _I_INS.count(ins.ope))
                for (int i = 0; i < 3; ++i)
                {
                    buffer >> temp; 
                    ins.fields.push_back(temp);
                }
            else if (_J_INS.count(ins.ope))
                for (int i = 0; i < 2; ++i)
                {
                    buffer >> temp; 
                    ins.fields.push_back(temp);
                }
            else if (_DIR_INS.count(ins.ope))
                {
                    buffer >> temp;
                    ins.fields.push_back(temp);
                }
            else if (!_O_INS.count(ins.ope))
                throw SyntaxError("Invalid opearator: " + ins.ope);

            _ins.push_back(ins);
            if (ins.label.size())
                if (_labels.count(ins.label))
                    throw SyntaxError("Duplicated label: " + ins.label);
                else
                    _labels[ins.label] = pc;

            pc += 1;
        }
        catch (stringstream::failure e)
        {
            stringstream expbuffer;
            expbuffer << "  error on line: " << _ins.size()+1 << '\n'
                      << "     " << s << '\n'
                      << "  Failed to recognize the structure of the operator\n";
            throw SyntaxError(expbuffer.str());
        }
        catch (SyntaxError e)
        {
            stringstream expbuffer;
            expbuffer << "  error on line: " << _ins.size()+1 << '\n'
                      << "     " << s << '\n'
                      << "  " << e.what() << '\n';
            throw SyntaxError(expbuffer.str());
        }

    //second scan
    pc = 0x00000000;
    for (auto ins: _ins)
        try
        {
            if (_R_INS.count(ins.ope))
                _mc.push_back(encode_R(ins));
            else if (_I_INS.count(ins.ope))
                _mc.push_back(encode_I(ins, pc));
            else if (_J_INS.count(ins.ope))
                _mc.push_back(encode_J(ins));
            else if (_O_INS.count(ins.ope))
                _mc.push_back(encode_O(ins));
            else
                _mc.push_back(encode_DIR(ins));
            pc += 1;
        }
        catch (SyntaxError e)
        {
            stringstream expbuffer;
            expbuffer << "  error on line: " << pc+1 << '\n'
                      << "     " << _asm[pc] << '\n'
                      << "  " << e.what() << '\n';
            throw SyntaxError(expbuffer.str());
        }

}

void Assembler::output2stream(ostream &s, char mode)
{
    if (mode == 'H')
        for_each(_mc.begin(), _mc.end(), [&s](mc_t &mc){printf("%X\n", mc);});
    else
        for_each(_mc.begin(), _mc.end(), [&s](mc_t &mc){s << mc << endl;});
}

void Assembler::saveToFile(const string &filename)
{
    ofstream output;
    
    try
    {
        output.open(filename);

        for (auto &mc: _mc)
            output << mc << '\n';
        output << std::flush;

        output.close();
    }
    catch (ofstream::failure)
    {
        throw IOError("Failed when write to file: " + filename);
    }
}








//debug code
void Assembler::pprint(const string &str)
{
    cout << '|';
    for (int i = 31; i >= 0; --i)
        printf("%02d|", i);
    cout << endl << '|';
    for_each(str.begin(), str.end(), [](char c){cout << ' ' << c << '|';});
    cout << endl;
}

string Assembler::dec2bin(mc_t code)
{
    string s = "";
    while (code)
    {
        s.push_back('0' + code % 2);
        code /= 2;
    }
    s.resize(32, '0');
    reverse(s.begin(), s.end());
    return s;
}

void Assembler::test()
{
    Ins ins;
    ins.ope = "lw";
    ins.fields = vector<string>{"1", "2", "3"};

    pprint( dec2bin(encode_I(ins, 0)) );

    encode();
    output2stream(cout, 'H');
}

int main(int argc, char* argv[])
{
    if (argc != 3 && argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <asm file> <machine code file (optional)> " << endl;
        return EXIT_FAILURE;
    }

    try
    {
        Assembler asmer;
        asmer.loadFromFile(argv[1]);
        asmer.encode();
        if (argc == 2)
            asmer.output2stream(cout);
        else
            asmer.saveToFile(argv[2]);
    }
    catch (IOError e)
    {
        cerr << "IOError occured: " << endl << e.what() << endl;
        return EXIT_FAILURE;
    }
    catch (SyntaxError e)
    {
        cerr << "SyntaxError occured: " << endl << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//q1 how to turn all the functions into static functions temporarily while debuging
