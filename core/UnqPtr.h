#ifndef SMARTPTR_UNQPTR_H
#define SMARTPTR_UNQPTR_H

#include <stdexcept> // для логической ошибки в * и ->

template<class T>
class UnqPtr {
private:
    T *ptr;

    static T* CopyOf(const T* source) {
        return source != nullptr ? new T(*source) : nullptr;
    }

    void CheckNotEmpty() const {
        if (this->ptr == nullptr) {
            throw std::logic_error("UnqPtr: пустой указатель");
        }
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
        delete ptr;
        ptr = copy;
        return *this;
    }

    UnqPtr(UnqPtr&& other) noexcept : ptr(other.ptr) { //потому что мы перемещаем а не создаём новый адрес
        other.ptr = nullptr;
    }
    UnqPtr& operator=(UnqPtr &&other) noexcept { //нельзя передать перем с именем, только временную
        if (this == &other) { //сравниваем адрес с адресом
            return *this;
        }
        delete ptr;
        ptr  = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    T& operator*() const { //возвращаем то что лежит по адресу птр
        CheckNotEmpty();
        return *this->ptr;
    }

    T* operator->() const {
        CheckNotEmpty();
        return this->ptr;
    }

    T* Get() const {
        return this->ptr;
    }

    explicit operator bool() const { // true, если указатель НЕ пустой
        return (this->ptr != nullptr);
    }

    T* Release() { //возвращаем адрес, при этом оставляя после себя ничего
        T* old = this->ptr;
        this->ptr = nullptr;
        return old;
    }

    void Reset(T* item = nullptr) { //значение по умолчанию для reset
        if (this->ptr == item) {
            return;
        }
        delete ptr;
        ptr = item;
    }


    ~UnqPtr() {
        delete ptr;
    }

};

#endif //SMARTPTR_UNQPTR_H
