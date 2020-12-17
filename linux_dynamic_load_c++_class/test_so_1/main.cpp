#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include<vector>
#include <memory>

#include "p_base.h"

using namespace std;

class p_lib_handle
{
    void *lib_handle;
    string _name;
    bool _is_open;
    destroy_t* destroy_plug;
    p_base* _p;

public:
    p_lib_handle(string __name,int __id):lib_handle(nullptr),_name(__name),_is_open(false),destroy_plug(nullptr),_p(nullptr)
    {
         lib_handle = dlopen(_name.c_str(), RTLD_LAZY);
         if(!lib_handle) return;

         create_t* create_plug = reinterpret_cast<create_t*>(dlsym(lib_handle, "create"));
         destroy_plug = reinterpret_cast<destroy_t*>(dlsym(lib_handle, "destroy"));

         if (!create_plug || !destroy_plug) return;

         _p = create_plug(__id);
         if(!_p) return;

         _is_open=true;
    }
    ~p_lib_handle(){


        if(destroy_plug)
        {
            destroy_plug(_p);
        }
        if(lib_handle)
        {
            cout << "dlclose -" << _name << "\n";
            dlclose(lib_handle);
        }

        _is_open=false;

    }
    string name()
    {
        return _name;
    }
    bool is_open()
    {
        return _is_open;
    }
    p_base* p()
    {
        return _p;
    }


};


int main()//int argc, char **argv)
{
    vector< shared_ptr<p_lib_handle> > plugin;

    try {

        plugin.push_back(  make_shared<p_lib_handle>("../v_test/v_test.so.1.0",1));
        plugin.push_back(  make_shared<p_lib_handle>("../v_test/v_test.so.1.0",2333));
        plugin.push_back(  make_shared<p_lib_handle>("../v_test2/v_test2.so.1.0",3));
        plugin.push_back(  make_shared<p_lib_handle>("../v_test2/v_test2.so.1.0",3767));




        for(auto p_tmp:plugin) {

           if(p_tmp->is_open() )
           {
               cout << p_tmp->name() << "-" << p_tmp->p()->test() << '\n';
           }
           else
           {
               cout << p_tmp->name() << " not open " << '\n';
           }
        }
        // удалить last вызов деструктора и выгрузка
        plugin.pop_back();
        //удалить

       plugin.push_back(  make_shared<p_lib_handle>("../v_test/v_test.so.1.not_open",2)); // not found !+ except

       for(auto p_tmp:plugin) {

          if(p_tmp->is_open() )
          {
              cout << p_tmp->name() << "-" << p_tmp->p()->test() << '\n';
          }
          else
          {
              cout << p_tmp->name() << " not open " << '\n';
              //throw("errr");
          }
       }
    }
    catch (...)
    {
        cout <<  " except " << '\n';
    }


   return 0;
}
