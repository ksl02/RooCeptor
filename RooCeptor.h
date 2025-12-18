#include "RooDefs.h"
#include "RooWriter.h"
#include <vector>

enum InterceptionStates
{
    ROO_NOT_INTERCEPTED = 0,
    ROO_REPLACED,
    ROO_ATTACHED
};

class RooCeptor
{
private:
	RooWriter _mRooWriter;
	void* _mSource = nullptr;
	void* _mTarget = nullptr;

    void* _mEntrance = nullptr;
    void* _mExit = nullptr;

    void* _mOriginal = nullptr;

    void* _mTrampoline = nullptr;

    int _mInterceptState = ROO_NOT_INTERCEPTED;

    bool _mValid = false;

    int _mCodeSize = 0;

    void insert_jump_to_address(instruction address);
    void prepare_entrance(void* entrance);
    void insert_entrance();
    void conceal_entrance();
    
    void insert_exit(void* exit);

    void execute_original();

    bool create_native_copy_src();
    void fill_native_copy_src();

    std::vector<instruction> _mOriginal_instrs;


public:

    static std::vector<address> interceptions;

    RooCeptor(void* origin_address);
	~RooCeptor();

    void print_buffer_bytes(void* address, int size);

    bool valid();

    bool replace(void* repl_func);
    bool attach(void* enter = NULL, void* leave = NULL);
    bool detach();

    bool is_attached() const;
    bool is_replaced() const;
    bool is_intercepted() const;
    int get_current_code_size() const;

    void set_current_code_size(const int size);
    int set_interception_state(const int state);

    bool set_trampoline();
};