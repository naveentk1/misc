#include <iostream>

using namespace std;


template<typename T>
class UniquePtr
{
    private:
    T* ptr;

    public:
    explicit UniquePtr(T* p = nullptr) : ptr(p)
    {
        cout<<"UniquePtr created , managing:" <<ptr <<'\n';
    }

    ~UniquePtr()
    {
        cout<<"UniquePtr destroying , deleting: "<<ptr <<endl;
        delete ptr;
    }

    UniquePtr(const UniquePtr&) = delete;

    UniquePtr& operator = (const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
        cout<<"UniquePtr moved"<<"\n";
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept 
    {
        if (this != &other) 
        {
            delete ptr;           
            ptr = other.ptr;      
            other.ptr = nullptr;  
        }
        return *this;
    }

    T& operator*() const
    {
        return *ptr;
    }

    T* operator->() const
    {
    return ptr; 
    }

     T* get() const
     {
        return ptr;
     }

     T* Release()
     {
        T* temp = ptr;
        ptr = nullptr;
        return temp;
     }

     explicit operator bool() const {
        return ptr != nullptr;

     }

};


struct Person 
{
    string name;
    int age;
    
    Person(string n, int a) : name(n), age(a) 
    {
        cout << "Person created: " << name << endl;
    }
    
    ~Person() 
    {
        cout << "Person destroyed: " << name << endl;
    }
    
    void greet() 
    {
        cout << "Hi, I'm " << name << ", age " << age << endl;
    }
};


int main() 
{
    cout << "=== Creating unique pointer ===" << endl;
    UniquePtr<Person> p1(new Person("Alice", 25));
    
    cout << "\n=== Using the pointer ===" << endl;
    p1->greet();          
    (*p1).age = 26;       
    cout << "Age: " << p1->age << endl;
    
    cout << "\n=== Moving ownership ===" << endl;
    UniquePtr<Person> p2 = move(p1); 
    
    if (!p1)
    {
        cout << "p1 is now empty" << endl;
    }
    if (p2) 
    {
        cout << "p2 now owns the object" << endl;
        p2->greet();
    }
    
    cout << "\n=== Automatic cleanup ===" << endl;

    
    return 0;
}