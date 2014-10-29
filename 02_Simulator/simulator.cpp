#include <iostream>
#include <cstdio>
#include <fstream>

#include <vector>
#include <string>

#include <stdexcept>
#include <cstring>
#include <cstdlib>

using namespace std;

typedef unsigned int mc_t;
typedef int word_t;

class Simulator
{
 private:
    static const int NUMMEMORY = 65536;
    static const int NUMREGS = 8;

    word_t _reg[NUMREGS];
    word_t * _mem;

    int _mem_c, _pc;
    bool _ready;
    bool _end;

    inline void runAdd(mc_t );
    inline void runNand(mc_t );
    inline void runLw(mc_t );
    inline void runSw(mc_t );
    inline void runBeq(mc_t );
    inline void runJalr(mc_t );
    inline void runHalt(mc_t );
    inline void runNoop(mc_t );

    inline word_t getOffset(mc_t );
 public:
    Simulator(): _mem(new word_t [NUMMEMORY]), _ready(false) {}
    ~Simulator() { if (_mem) delete [] _mem; }

    void loadFromFile(string filename);
    void setMC(const vector<mc_t> &mc);
    void printInit(ostream & os = cout);
    void printState(ostream & os = cout);
    bool next();
};

void Simulator::runAdd(mc_t mc)
{
    mc_t regA = (mc >> 19) & 0x7;
    mc_t regB = (mc >> 16) & 0x7;
    mc_t regC = (mc >> 0) & 0x7;

    _reg[regC] = _reg[regA]+_reg[regB];
    ++_pc;
}

void Simulator::runNand(mc_t mc)
{
    mc_t regA = (mc >> 19) & 0x7;
    mc_t regB = (mc >> 16) & 0x7;
    mc_t regC = (mc >> 0) & 0x7;


    _reg[regC] = ~(_reg[regA] & _reg[regB]);
    ++_pc;
}

word_t Simulator::getOffset(mc_t mc)
{
    mc_t offset = mc & ((1 << 16) - 1);
    if (offset & (1 << 15))
        return int(offset - (1 << 16));
    else
        return offset;
}

void Simulator::runLw(mc_t mc)
{
    mc_t regA = (mc >> 19) & 0x7;
    mc_t regB = (mc >> 16) & 0x7;
    int basic_addr = _reg[regA],
        shifted = getOffset(mc);
    int addr = basic_addr + shifted;
    if (addr < 0 || addr >= NUMMEMORY)
        throw runtime_error("Invalid memory access!");
    _reg[regB] = _mem[addr];
    ++_pc;
}

void Simulator::runSw(mc_t mc)
{
    mc_t regA = (mc >> 19) & 0x7;
    mc_t regB = (mc >> 16) & 0x7;
    int basic_addr = _reg[regA],
        shifted = getOffset(mc);
    int addr = basic_addr + shifted;
    if (addr < 0 || addr >= NUMMEMORY)
        throw runtime_error("Invalid memory access!");
    _mem[addr] = _reg[regB];
    ++_pc;
}

void Simulator::runBeq(mc_t mc)
{
    mc_t regA = (mc >> 19) & 0x7;
    mc_t regB = (mc >> 16) & 0x7;

    if (_reg[regA] == _reg[regB])
        _pc += getOffset(mc);
    ++_pc;
}

void Simulator::runJalr(mc_t mc)
{
    mc_t regA = (mc >> 19) & 0x7;
    mc_t regB = (mc >> 16) & 0x7;

    _reg[regB] = _pc + 1;
    _pc = _reg[regA];
}

void Simulator::runHalt(mc_t mc)
{
    _ready = false;
    _end = true;
}

void Simulator::runNoop(mc_t mc)
{
    ++_pc;
}


bool Simulator::next()
{
    if (_pc >= NUMMEMORY)
        throw runtime_error("Invalid memory access!");

    if (_end || !_ready)
        return false;

    mc_t cur = _mem[_pc]; 
    mc_t opcode = (cur >> 22) & (0x7);
    switch (opcode)
    {
        case 0: runAdd(cur); break;
        case 1: runNand(cur); break;
        case 2: runLw(cur); break;
        case 3: runSw(cur); break;
        case 4: runBeq(cur); break;
        case 5: runJalr(cur); break;
        case 6: runHalt(cur); break;
        case 7: runNoop(cur); break;
    }
    return true;
}

void Simulator::setMC(const vector<mc_t> & mc)
{
    if (_ready)
        throw runtime_error("The code is executing!");

    memset(_reg, 0, sizeof(word_t)*NUMREGS);
    memset(_mem, 0, sizeof(word_t)*NUMMEMORY);

    _mem_c = mc.size();
    copy(mc.begin(), mc.end(), _mem);
    _pc = 0;
    _ready = true;
    _end = false;
}

void Simulator::printInit(ostream & os)
{
    for (int i = 0; i < _mem_c; ++i)
//        printf("memory[%d] = %d\n", i, _mem[i]);
        os << "memory[" << i << "]=" << _mem[i] << endl;
    os.flush();
}

void Simulator::printState(ostream & os)
{
    os << endl
       << "@@@" << endl
       << "state:" << endl
       << "\tpc " << _pc << endl
       << "\tmemory: " << endl;
    for (int i = 0; i < _mem_c; ++i)
        os << "\t\tmem[ " << i << " ] " << _mem[i] << endl;
    os << "\tregisters:" << endl;
    for (int i = 0; i < NUMREGS; ++i)
        os << "\t\treg[ " << i << " ] " << _reg[i] << endl;
    os << "end state" << endl;
    os.flush();
}

void Simulator::loadFromFile(string filename)
{
    ifstream ifs(filename.c_str());
    if (!ifs)
        throw runtime_error("Invalid filename: "+filename);

    vector<mc_t> mc;
    int tmp;
    while (ifs >> tmp)
        mc.push_back(tmp);
    setMC(mc);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>" << endl;
        return EXIT_FAILURE;
    }

    int count = 0;
    Simulator simulator;
    try
    {
        simulator.loadFromFile(argv[1]);
        simulator.printInit();
        simulator.printState();

        while (simulator.next())
        {
            simulator.printState();
            count++;
        }
        cout << "machine halted\ntotal of "<< count <<" instructions executed\nfinal state of machine:\n";
        simulator.printState();
    }
    catch (runtime_error e)
    {
        cerr << e.what(); 
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
