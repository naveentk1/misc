#include <iostream>

using namespace std;

namespace differentUseForCout{
  void cout(){
    std::cout<<"fuck you";
  }
}


int i = 10;

int main(){
  // reference implementation
  
    int i = 11;
    cout<< "original: " << i << '\n';
    int& _i = i;

    _i = 12 ;

   cout<<"reference to i after modification: "<<_i<<'\n';
   

   //pointer implementation

   int* i_ptr = &i;

   cout<<"value of i before i_ptr modifies it: "<<*i_ptr<<'\n';

   *i_ptr = 13;

   cout<<"i value modified using ptr: " << *i_ptr <<'\n';
   cout<<"final value of i: " <<i<<'\n';

   cout<<"global namespace i variable: "<<::i<<'\n';


   differentUseForCout::cout();


}