#pragma once
#include "RooDefs.h"
#include <vector>

//TODO: change public functions eventually to private

class RooWriter
{
public:
	static bool protect(void* origin, int size, int flags);
	static bool write_jump_to_address(void* origin, void* target, int offset = 0);
	
	void add_instruction(instruction inst);
    void add_address(instruction address);


    void increment_pc_offset();
    void increment_sp_padding(int num);

    void set_thumb_mode(bool mode);

	int get_pc_offset() const;
    int get_sp_padding() const;
	int get_current_code_buffer_size();
	
	bool is_thumb_enabled() const;
	
	bool init(void* origin_address);

	//change these to vector types later
	//eventually if wanting x86 support, 
	//overload all of these instead of template
	void push_regs(std::vector<ARM_REG> registers);
    void push_reg(ARM_REG reg);
	void pop_regs(std::vector<ARM_REG> registers);
    void pop_reg(ARM_REG reg);

	void add_reg_reg_imm(ARM_REG dest, ARM_REG src, uint32_t imm);
	void sub_reg_reg_imm(ARM_REG dest, ARM_REG src, uint32_t imm);
    void str_reg_reg_offset(ARM_REG dest_reg, ARM_REG src_reg, int offset);
    
    void ldr_reg_reg_offset(ARM_REG dest_reg,ARM_REG src_reg,int offset);

    void mov_reg_reg(ARM_REG dest,ARM_REG src);
	void bx_lr();
    void nop();

    void update_literal_references(void* trampoline);
    void add_literal_reference(ARM_REG dest_reg,instruction address);

	void copy_instr_to_trampoline(void* trampoline);

    RooWriter();
	RooWriter(void* origin_address);
	~RooWriter();

private:
	void* _mSource = 0;

	bool _mThumb = false;
    
    int _mPC = 0;
    int _mLiteralsN = 0;
    uint16_t _mSP_PADDING = 1;

    std::vector<instruction> _mCode;

    std::vector<instruction> _mLref_offsets;
    std::vector<instruction> _mLref_addresses;


};

