// Order of invocation is undefined in this context according to the C++ standard.
// It's possible to leak a Foo or a Bar depending on the order of evaluation if one
// of the new statements throws an exception before their auto_ptrs can "own" it
accept_two_ptrs(std::auto_ptr<Foo>(new Foo), std::auto_ptr<Bar>(new Bar));

void MyClass::InvokeCallback(CallbackType cb)
{
    Foo* resource = new Foo;
    cb(resource); // If cb throws an exception, resource leaks
    delete resource;
}
