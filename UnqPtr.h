#ifndef SMARTPTR_UNQ_PTR_H
#define SMARTPTR_UNQ_PTR_H

template<class T>
class UnqPtr {
private:
    T *ptr;

    static T* CopyOf(const T* source) {
        return source != nullptr ? new T(*source) : nullptr;
    }
public:
    UnqPtr(): ptr(nullptr) {}
    explicit UnqPtr(T *item): ptr(item) {} //чтобы явно всегда задавалось
    //тк потом надо будет проворачивать подтипизацию, никаких конст ссылок

    UnqPtr(const UnqPtr &other): ptr(CopyOf(other.ptr)) {} //это глубокая копия
    UnqPtr& operator=(const UnqPtr& other) { //и это глубокая копия
        if (this == &other) { //сравниваем адрес с адресом
            return *this;
        }
        T *copy = CopyOf(other.ptr);
        delete(this->ptr);
        ptr = copy;
        return *this;
    }

    UnqPtr(UnqPtr&& other) noexcept : ptr(CopyOf(other.ptr)) {
        delete(other.ptr);
    }
    UnqPtr& operator=(UnqPtr &&other) noexcept { //нельзя передать перем с именем, только временную
        if (this == &other) { //сравниваем адрес с адресом
            return *this;
        }
        delete(this->ptr);
        ptr  = other.ptr;
        other.ptr = nullptr;
        return *this;
    }


    ~UnqPtr() {
        delete ptr;
    }

};

#endif //SMARTPTR_UNQ_PTR_H
