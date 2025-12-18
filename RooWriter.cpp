#include "RooWriter.h"
#include "inttypes.h"
#include <algorithm>    // std::min
#include <sys/mman.h> //mprotect
#include <iostream>
#include <cmath>

using namespace std;

uint32_t gum_arm_condify(uint32_t cc)
{
    return (cc - 1) << 28;
}

RooWriter::RooWriter()
{
    //cout << "RooWriter: Default constructor called" << endl;
}

RooWriter::RooWriter(void* origin_address)
{
	_mSource = origin_address;
}

RooWriter::~RooWriter()
{
    //cout << "RooWriter: Destructor called" << endl;
}

bool RooWriter::init(void* origin_address)
{
	_mSource = origin_address;

    //do some validity checking here!

    return 1;
}

bool RooWriter::protect(void* origin, int size, int flags)
{
	if(mprotect(origin, size, flags))
    {
        return false;
    }

    return true;
}

void RooWriter::increment_pc_offset()
{
    _mPC += 4;
}

void RooWriter::add_instruction(instruction instr)
{
    _mCode.push_back(instr);
    this->increment_pc_offset();
}

void RooWriter::add_address(instruction address)
{
    _mCode.push_back(address);
}



int RooWriter::get_pc_offset() const
{
    return _mPC;
}

bool RooWriter::is_thumb_enabled() const
{
    return _mThumb;
}

void RooWriter::set_thumb_mode(bool mode)
{
    _mThumb = mode;
}


bool RooWriter::write_jump_to_address(void* origin, void* target, int offset) {
	uint8_t jmpcode[8] = 
    {
		0x04,0xf0,0x1f,0xe5, /* ldr pc, [pc, #-4] */
		0x00,0x00,0x00,0x00 /* (dummy address) */
	};

	*(uint32_t*)(jmpcode + 4) = (uint32_t)target;

    //cout << "Test1 "  << endl;

	void* page_start = (void*)((uint32_t)origin - (uint32_t)origin % 4096);

    //cout << "Page start: " << page_start << endl;

	if (!RooWriter::protect(page_start, 4096, PROT_READ | PROT_WRITE | PROT_EXEC))
    {
		printf("Protect region failed in RooWriter::write_jump_to_address, origin = %llu\n",(address)origin);
		return false;
	}

	memcpy((void*)((char*)origin + offset), jmpcode, 8);
	
    if (!RooWriter::protect(page_start, 4096, PROT_READ | PROT_EXEC))
    {
		printf("Protect region failed in RooWriter::write_jump_to_address, origin = %llu\n",(address)page_start);
		return false;
	}

    //cout << "Test3"  << endl;

	return true;

}

int RooWriter::get_current_code_buffer_size()
{
    return _mPC+_mLiteralsN*4; 
}

void RooWriter::copy_instr_to_trampoline(void* trampoline)
{
    //cout << "RooWriter::copy_instr_to_trampoline" << endl;
    int offset = 0;
    int i;
    uint32_t* ptr = (uint32_t*)trampoline;
    
    //cout << "addr ptr: " <<  ptr << endl;

    instruction instr;
    
    for(i = 0; i < _mPC/4; i++)
    {
        instr = _mCode.at(i);
        ptr[i] = instr;
    }

}

void RooWriter::push_regs(std::vector<ARM_REG> registers)
{
    int mask = 0;

    for(int i = 0; i < registers.size(); i++)
    {
        mask |= 1 << static_cast<int>(registers.at(i));
    }

    this->add_instruction(0xE92D0000 | mask);

    uint32_t instruct = 0xE92D0000 | mask;
    //cout << std::hex << "push regs: " << instruct << endl;
}

void RooWriter::push_reg(ARM_REG reg)
{
    this->add_instruction(0xE92D0000 | (1 << static_cast<int>(reg)));
}

void RooWriter::pop_regs(std::vector<ARM_REG> registers)
{
    int mask = 0;

    for(int i = 0; i < registers.size(); i++)
    {
        mask |= 1 << static_cast<int>(registers.at(i));
    }

    this->add_instruction(0xE8B00000 | (static_cast<int>(ARM_REG::SP) << 16) | mask);

    uint32_t instruct = 0xE8B00000 | (static_cast<int>(ARM_REG::SP) << 16) | mask;
    //cout << std::hex << "pop regs: " << instruct << endl;
}

void RooWriter::pop_reg(ARM_REG reg)
{
    this->add_instruction(0xE8B00000 | (static_cast<int>(ARM_REG::SP) << 16) | (1 << static_cast<int>(reg)));
    uint32_t instruct = 0xE8B00000 | (static_cast<int>(ARM_REG::SP) << 16) | (1 << static_cast<int>(reg));
    //cout << std::hex << "pop_reg: " << instruct << endl;
}

void RooWriter::add_reg_reg_imm(ARM_REG dest_reg, ARM_REG src_reg, uint32_t imm)
{
    uint32_t inst = 0xE2800000 | (static_cast<int>(dest_reg) << 12) | (static_cast<int>(src_reg) << 16) | (imm & INT12_MASK);
    this->add_instruction(inst);

    //cout << std::hex << "add_reg_reg_imm: " << inst << endl;
}

void RooWriter::mov_reg_reg(ARM_REG dest_reg, ARM_REG src_reg)
{
    this->add_reg_reg_imm(dest_reg,src_reg,0);
}

void RooWriter::sub_reg_reg_imm(ARM_REG dest_reg, ARM_REG src_reg, uint32_t imm)
{
    uint32_t inst = 0xE2400000 | (static_cast<int>(dest_reg) << 12) | (static_cast<int>(src_reg) << 16) | (imm & INT12_MASK);
    this->add_instruction(inst);

    //cout << std::hex << "sub_reg_reg_imm: " << inst << endl;
}

void RooWriter::bx_lr()
{
    this->add_instruction(__builtin_bswap32(0x1EFF2FE1)); //bx lr
}

void RooWriter::nop()
{
    this->add_instruction(__builtin_bswap32(0x00F020E3)); //NOP
}

void RooWriter::str_reg_reg_offset(ARM_REG dest_reg, ARM_REG src_reg, int offset)
{
    
    uint32_t cc = 15;

    bool is_positive = offset >= 0;

    uint32_t abs_dst_offset = abs(offset);
//   if (abs_dst_offset >= 4096)
//     return FALSE;

    uint32_t inst = 0x05000000 | gum_arm_condify (cc) |
      (is_positive << 23) | (static_cast<int>(src_reg) << 12) | (static_cast<int>(dest_reg) << 16) |
      abs_dst_offset;
    this->add_instruction(inst);

    //cout << std::hex << "str_reg_reg_offset: " << inst << endl;
}

void RooWriter::ldr_reg_reg_offset(ARM_REG dest_reg,ARM_REG src_reg,int offset)
{
    uint32_t cc = 15;

    bool is_positive = offset >= 0;

    uint32_t abs_src_offset = abs(offset);

    uint32_t inst = 0x05100000 | gum_arm_condify (cc) |
      (is_positive << 23) | (static_cast<int>(dest_reg) << 12) | (static_cast<int>(src_reg) << 16) |
      abs_src_offset;
    this->add_instruction(inst);

    //cout << std::hex << "ldr_reg_reg_offset: " << inst << " " << _mPC << endl;
}

void RooWriter::update_literal_references(void* trampoline)
{
    uint32_t* ptr = (uint32_t*)trampoline;

    int i;
    uint32_t offset;
    for(i = 0; i < _mLref_addresses.size(); i++)
    {
        //cout << "Add instruction: " << _mLref_addresses.at(i) << endl;
        this->add_address(_mLref_addresses.at(i));
        offset = _mLref_offsets.at(i);
        ptr[offset] = ptr[offset] | (i*4)+_mPC-(offset*4)-8;
        //cout << offset << " update_literal_references: new instruction = " << ptr[offset] << endl;

    }
    
    //cout << "addr ptr: " <<  ptr << endl;

    instruction instr;
    
    for(i = 0; i < _mLiteralsN; i++)
    {
        //cout << "i: " << _mPC/4+i << " " << _mCode.size() << endl; 
        instr = _mCode.at(_mPC/4+i);
        ptr[_mPC/4+i] = instr;
    }

}

void RooWriter::add_literal_reference(ARM_REG dest_reg,instruction address)
{
    //cout << "add_literal_reference " << _mPC << endl;
    
    _mLiteralsN++;

    _mLref_offsets.push_back(_mPC/4);
    _mLref_addresses.push_back(address);
    this->ldr_reg_reg_offset(dest_reg,ARM_REG::PC,0);
}

void RooWriter::increment_sp_padding(int num)
{
    _mSP_PADDING += num;
}

int RooWriter::get_sp_padding() const
{
    return _mSP_PADDING;
}




