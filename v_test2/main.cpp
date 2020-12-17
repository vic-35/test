#include "../test_so_1/p_base.h"

class p_test2 : public p_base {
    int ret;
public:
    p_test2(int _ret)
    {
        ret=_ret;
        std::cout << "p_test2 -"<< ret << "\n";
    };
    ~p_test2()
    {
        std::cout << "~p_test2 -"<< ret << "\n";
    }
    virtual int test(){
        return ret;
    }
};


// the class factories
extern "C" p_base* create(int _ret) {
    return new p_test2(_ret);
}

extern "C" void destroy(p_base* p) {
    delete p;
}
