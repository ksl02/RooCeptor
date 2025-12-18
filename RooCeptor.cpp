#include "RooCeptor.h"
#include "inttypes.h"
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <sys/mman.h> //mprotect

using namespace std;

vector<ARM_REG> regs_r0_to_r3{ARM_REG::R0,ARM_REG::R1,ARM_REG::R2,ARM_REG::R3,ARM_REG::R6,ARM_REG::R7,ARM_REG::R8};
vector<ARM_REG> regs_r4_r12{ARM_REG::R4,ARM_REG::R12};

RooCeptor::RooCeptor(void* origin_address) 
{
    _mSource = origin_address;

    _mRooWriter.init(origin_address);
    _mValid = true; //validity check here
}

RooCeptor::~RooCeptor()
{
    cout << "RooCeptor: Destructor called" << endl;
} 

void RooCeptor::print_buffer_bytes(void* address, int size)
{
    instruction* addr = (instruction*)address;
    cout << "Start display all buffer bytes" << endl;
    // int size = _mRooWriter.get_current_code_buffer_size()/4;
    for(int i = 0; i < size; i++)
    {
        cout << addr[i] << endl;
    }
    cout << "End display all buffer bytes" << endl;
}

bool RooCeptor::attach(void* entrance, void* exit)
{
    if(!this->valid())
    {
        return false;
    }
    cout << "RooCeptor::attach called" << endl;

    this->create_native_copy_src();

    _mRooWriter.push_reg(ARM_REG::R4); //push {r4,r12}
    cout << "Decrement stack by 4 in push" << endl;

    this->prepare_entrance(entrance);

    cout << "Increment stack by " << _mRooWriter.get_sp_padding()*4 << endl;
    _mRooWriter.add_reg_reg_imm(ARM_REG::SP,ARM_REG::SP,_mRooWriter.get_sp_padding()*4);
    _mRooWriter.add_reg_reg_imm(ARM_REG::R4,ARM_REG::LR,0); //mov r4, lr
    cout << "Insert entrance" << endl;
    this->insert_entrance();
    cout << "Execute orig" << endl;
    this->execute_original();
    cout << "Executed orig" << endl;

    _mRooWriter.sub_reg_reg_imm(ARM_REG::SP,ARM_REG::SP,4);
    cout << "Decrement stack by 4" << endl;
    _mRooWriter.add_reg_reg_imm(ARM_REG::LR,ARM_REG::R4,0); //mov lr, r4
    _mRooWriter.pop_reg(ARM_REG::R4); //pop {r4}
    cout << "Increment stack by 4" << endl;

    this->insert_exit(exit);
    // _mRooWriter.add_instruction((instruction)_mTarget); //editMe with jump address
    // *(address*)(jumpCodeCave + 20) = (address)target;

    this->set_trampoline();

    _mRooWriter.copy_instr_to_trampoline(_mTrampoline);
    _mRooWriter.update_literal_references(_mTrampoline);

    // this->print_buffer_bytes(_mTrampoline,_mRooWriter.get_current_code_buffer_size()/4);

    instruction* addr = (instruction*)_mSource;
    _mOriginal_instrs.push_back(addr[0]);
    _mOriginal_instrs.push_back(addr[1]);
    
    this->fill_native_copy_src();

    _mRooWriter.write_jump_to_address(_mSource,_mTrampoline,0);

    // cout << "Calling test" << endl;
    // ((int (*)(int,int,int,int,int,int,int,int,int,int))_mTrampoline)(1,2,3,4,5,6,7,8,9,10);
    // cout << "End calling test" << endl;

    this->set_interception_state(ROO_ATTACHED);
    return true;
}

bool RooCeptor::replace(void* repl_func)
{
    if(!this->valid())
    {
        return false;
    }
    cout << "RooCeptor::replace called" << endl;
    this->set_interception_state(ROO_REPLACED);
    return true;
}

bool RooCeptor::detach()
{
    cout << "RooCeptor::detach called" << endl;
    this->set_interception_state(ROO_NOT_INTERCEPTED);
    return true;
}

bool RooCeptor::valid()
{    
    return _mValid;
}

bool RooCeptor::is_attached() const
{
    return _mInterceptState == ROO_ATTACHED;
}

bool RooCeptor::is_replaced() const
{
    return _mInterceptState == ROO_REPLACED;
}

bool RooCeptor::is_intercepted() const
{   
    return _mInterceptState != ROO_NOT_INTERCEPTED;
}

int RooCeptor::set_interception_state(const int state){
    if(state == ROO_NOT_INTERCEPTED || state == ROO_REPLACED || state == ROO_ATTACHED)
    {
        _mInterceptState = state;
    }
    else
    {
       _mInterceptState = ROO_NOT_INTERCEPTED; 
    }

    return _mInterceptState;
}

int RooCeptor::get_current_code_size() const
{
    return _mCodeSize;
}

void RooCeptor::set_current_code_size(const int size)
{
    _mCodeSize = size;
}

bool RooCeptor::create_native_copy_src()
{
    if(_mOriginal != nullptr) //native copy already exists
    {
        return false;
    }
    
    _mOriginal = mmap(NULL, 16, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);     

    return true;
}

void RooCeptor::fill_native_copy_src()
{
    uint32_t* addr = (uint32_t*)_mOriginal;

    //Don't even check for error here because if this vector is empty
    //everything will break. It should let you know when you compile
    addr[0] = _mOriginal_instrs.at(0);
    addr[1] = _mOriginal_instrs.at(1);
    addr[2] = __builtin_bswap32(0x04F01FE5); //ldr pc, [pc, #-4]
    addr[3] = (instruction)_mSource+8; //jump to original after patch
}

bool RooCeptor::set_trampoline()
{
    cout << "RooCeptor::set_trampoline" << endl;

    //TODO: this is more complicated for multiple attaches
    //because realistically this ptr should be a static variable 
    //within map for address 
    if(this->is_intercepted())
    {
        cout << "WARNING: Called munmap!" << endl;
        munmap(_mTrampoline,this->get_current_code_size());
    }

    cout << "Getting func size!" << endl;
    int size = _mRooWriter.get_current_code_buffer_size();
    cout << "Got size: " << size << endl;

    this->set_current_code_size(size);

    cout << "Set size: " << size << endl;
    
    _mTrampoline = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
    
    cout << "Mmapped region: " << _mTrampoline << endl;

    return true;
}

void RooCeptor::insert_jump_to_address(instruction address)
{
    vector<ARM_REG> regs_r0_pc{ARM_REG::R0,ARM_REG::PC};
    _mRooWriter.add_reg_reg_imm(ARM_REG::LR,ARM_REG::PC,12);
    _mRooWriter.push_regs(regs_r0_pc); //push {r0,pc}
    cout << "Decrement stack by 8" << endl;
    _mRooWriter.add_literal_reference(ARM_REG::R0, address);
    _mRooWriter.str_reg_reg_offset(ARM_REG::SP,ARM_REG::R0,4); //str r0, [sp, #4]
    _mRooWriter.pop_regs(regs_r0_pc);  //pop {r0,pc}
    cout << "Increment stack by 8" << endl;
}

void RooCeptor::prepare_entrance(void* entrance)
{

    if(entrance == nullptr)
    {
        return;
    }

    _mEntrance = entrance;

    _mRooWriter.push_regs(regs_r0_to_r3);
    cout << "Decrement stack by 16" << endl;
    _mRooWriter.increment_sp_padding(regs_r0_to_r3.size());
}

void RooCeptor::insert_entrance()
{
    if(_mEntrance == nullptr)
    {
        return;
    }

    this->insert_jump_to_address((instruction)_mEntrance);
    this->conceal_entrance();

}

void RooCeptor::conceal_entrance()
{
    _mRooWriter.sub_reg_reg_imm(ARM_REG::SP,ARM_REG::SP,(_mRooWriter.get_sp_padding()+1)*4);
    cout << "Decrement stack by " << (_mRooWriter.get_sp_padding()+1)*4 << endl;

    _mRooWriter.pop_reg(ARM_REG::R3);
    _mRooWriter.pop_reg(ARM_REG::R2); 
    _mRooWriter.pop_reg(ARM_REG::R1); 
    _mRooWriter.pop_reg(ARM_REG::R0); 
    cout << "Increment stack by 16" << endl;

    _mRooWriter.add_reg_reg_imm(ARM_REG::SP,ARM_REG::SP,(_mRooWriter.get_sp_padding()+1-4)*4);
    cout << "Increment stack by " << (_mRooWriter.get_sp_padding()+1-4)*4 << endl;
}

void RooCeptor::execute_original()
{
    this->insert_jump_to_address((instruction)_mOriginal);
}

void RooCeptor::insert_exit(void* exit)
{

    if(exit == nullptr)
    {
        _mRooWriter.bx_lr();
        return;
    }

    _mRooWriter.ldr_reg_reg_offset(ARM_REG::PC,ARM_REG::PC,-4);
    _mRooWriter.add_instruction((instruction)exit);

    //insert exit here

}