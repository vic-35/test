#ifndef P_BASE_H
#define P_BASE_H
#include<iostream>

class p_base {
public:
    p_base(){};
    virtual ~p_base()
    {
        std::cout << "~p_base\n";
    };
    virtual int test() = 0;
};


typedef p_base* create_t(int _ret);
typedef void destroy_t(p_base*);


#endif // P_BASE_H
