class Foo
{
public:
    void doit() const;
};

typedef vector<Foo> FooList;
typedef shared_ptr<FooList> FooListPtr;
mutex _mutex;
FooListPtr  g_foos;

void post(const Foo& f)
{
    cout<<"post"<<endl;
    lock_guard<mutex> lock(mutex);
    if(!g_foos.unique())
    {
        g_foos.reset(new FooList(*g_foos));
        cout<<"copy the whole list"<<endl;
    }
    assert(g_foos.unique());
    g_foos->push_back(f);
}

void traverse()
{
    FooListPtr foos;
    {
        lock_guard <mutex> lock(_mutex);
        foos=g_foos;
        assert(!g_foos.unique());
    }

    for (std::vector<Foo>::const_iterator it = foos->begin();
         it != foos->end(); ++it)
    {
        it->doit();
    }
}

void Foo::doit() const
{
    Foo f;
    post(f);
}




int main()
{
    g_foos.reset(new FooList);
    Foo f;
    post(f);
    traverse();
    return 0;
}
