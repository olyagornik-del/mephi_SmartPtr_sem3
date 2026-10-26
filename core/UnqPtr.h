#ifndef SMARTPTR_UNQPTR_H
#define SMARTPTR_UNQPTR_H

#include <concepts> // derived_from
#include <stdexcept> // logic_error, out_of_range, invalid_argument

template<class T>
class UnqPtr {
private:
    T *ptr;

    void CheckNotEmpty() const {
        if (this->ptr == nullptr) {
            throw std::logic_error("UnqPtr: пустой указатель");
        }
    }
public:
    UnqPtr(): ptr(nullptr) {}
    explicit UnqPtr(T *item): ptr(item) {} //чтобы явно всегда задавалось
    //тк потом надо будет проворачивать подтипизацию, никаких конст ссылок

    // копирование запрещено
    UnqPtr(const UnqPtr &other) = delete;
    UnqPtr& operator=(const UnqPtr &other) = delete;

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

    // подтипизация: UnqPtr<Cat> -> UnqPtr<Animal>, только перемещением
    template <class U>
        requires std::derived_from<U, T> // условие: U - наследник Т
    UnqPtr(UnqPtr<U> &&other) noexcept: ptr(other.Release()) {} // U* -> T* само

    template <class U>
        requires std::derived_from<U, T>
    UnqPtr& operator=(UnqPtr<U> &&other) noexcept {
        Reset(other.Release()); // старый объект удалится внутри Reset
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

// версия для массивов: UnqPtr<int[]> arr(10)
// специализация: если T — массив, компилятор берёт этот класс, а не тот, что выше
// отличия: delete[] вместо delete, operator[] с проверкой границ, хранится размер,
// нет *, -> и подтипизации (массив Cat нельзя считать массивом Animal)
template <class T>
class UnqPtr<T[]> {
private:
    T *ptr;
    int size;

    void CheckIndex(int index) const {
        if (ptr == nullptr) {
            throw std::logic_error("UnqPtr<T[]>: пустой указатель");
        }
        if (index < 0 || index >= size) {
            throw std::out_of_range("UnqPtr<T[]>: индекс вне массива");
        }
    }

public:
    UnqPtr(): ptr(nullptr), size(0) {}

    // массив выделяем сами — пользователь не пишет new[] и не может ошибиться с размером
    explicit UnqPtr(int size): ptr(nullptr), size(0) {
        if (size < 0) {
            throw std::invalid_argument("UnqPtr<T[]>: отрицательный размер");
        }
        if (size == 0) {
            return; // пустой массив — память не выделяем
        }
        ptr = new T[size](); // () — элементы по умолчанию: для int нули, для классов — конструктор без аргументов
        this->size = size;
    }

    // копировать нельзя, как и обычный UnqPtr
    UnqPtr(const UnqPtr &other) = delete;
    UnqPtr& operator=(const UnqPtr &other) = delete;

    // перемещение — забираем массив вместе с размером
    UnqPtr(UnqPtr &&other) noexcept: ptr(other.ptr), size(other.size) {
        other.ptr = nullptr;
        other.size = 0;
    }

    UnqPtr& operator=(UnqPtr &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        delete[] ptr;
        ptr = other.ptr;
        size = other.size;
        other.ptr = nullptr;
        other.size = 0;
        return *this;
    }

    // элемент по индексу; ссылка — чтобы можно было и читать, и менять: arr[3] = 5
    T& operator[](int index) const {
        CheckIndex(index);
        return ptr[index];
    }

    T* Get() const {
        return ptr;
    }

    int GetSize() const {
        return size;
    }

    explicit operator bool() const {
        return ptr != nullptr;
    }

    // удалить массив и стать пустым
    void Reset() {
        delete[] ptr;
        ptr = nullptr;
        size = 0;
    }

    ~UnqPtr() {
        delete[] ptr;
    }
};

#endif //SMARTPTR_UNQPTR_H
