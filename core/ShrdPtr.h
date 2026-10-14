#ifndef SMARTPTR_SHRDPTR_H
#define SMARTPTR_SHRDPTR_H

#include <cstddef> // size_t
#include <stdexcept> // logic_error
#include <utility> // move

#include "UnqPtr.h"

// общая часть блока — одинаковая для любого T => ShrdPtr<Cat> и ShrdPtr<Animal> будут на одно и тоже указывать
struct ShrdBlockBase {
    size_t count = 1; // сколько ShrdPtr смотрят на блок; блок создаётся сразу с одним владельцем

    // деструктор виртуальный чтобы удалить блок через ShrdBlockBase*
    virtual ~ShrdBlockBase() = default;
};

// конкретный блок — хранит ведущий указатель на объект
template <class T>
struct ShrdBlock : ShrdBlockBase {
    UnqPtr<T> master; // единственный, кто владеет объектом и удаляет его

    explicit ShrdBlock(T *item): master(item) {}
    explicit ShrdBlock(UnqPtr<T> &&source): master(std::move(source)) {}
};

template <class T>
class ShrdPtr {
private:
    ShrdBlockBase *block; // счётчик + владение
    T *window;

    // отцепиться от блока: если мы были последними — удалить блок вместе с объектом
    void Detach() {
        ShrdBlockBase *old = block;
        // чтобы не смотреть на удалённый блок
        block = nullptr;
        window = nullptr;
        if (old != nullptr) {
            old->count--;
            if (old->count == 0) {
                delete old; // виртуальный деструктор -> ~ShrdBlock<T> -> ~UnqPtr<T> -> delete объекта
            }
        }
    }

    void CheckNotEmpty() const {
        if (window == nullptr) {
            throw std::logic_error("ShrdPtr: пустой указатель");
        }
    }

public:
    // пустой указатель
    ShrdPtr(): block(nullptr), window(nullptr) {}

    // берём во владение уже созданный объект
    explicit ShrdPtr(T *item): block(nullptr), window(nullptr) {
        if (item == nullptr) {
            return; // на пустоту блок не заводим
        }
        try {
            block = new ShrdBlock<T>(item);
        } catch (...) {
            // нет памяти => объект нам уже отдали, удаляем его, чтобы не утёк
            delete item;
            throw;
        }
        window = item;
    }

    // забираем объект у UnqPtr: один владелец -> общее, сам объект не копируется
    explicit ShrdPtr(UnqPtr<T> &&source): block(nullptr), window(nullptr) {
        if (!source) {
            return;
        }
        window = source.Get();
        // у параметра source есть имя, поэтому нужен std::move ещё раз, чтобы глубокой копии не было
        block = new ShrdBlock<T>(std::move(source));
    }

    // копия => + один владелец
    ShrdPtr(const ShrdPtr &other): block(other.block), window(other.window) {
        if (block != nullptr) {
            block->count++;
        }
    }

    //копирующее =
    ShrdPtr& operator=(const ShrdPtr &other) {
        if (this == &other) {
            return *this;
        }
        Detach(); // отпускаем старый объект (возможно, он удалится)
        block = other.block;
        window = other.window;
        if (block != nullptr) {
            block->count++;
        }
        return *this;
    }

    // перемещение — забираем блок, счётчик не меняется: владельцев столько же
    ShrdPtr(ShrdPtr &&other) noexcept: block(other.block), window(other.window) {
        other.block = nullptr;
        other.window = nullptr;
    }

    //перемещающие =
    ShrdPtr& operator=(ShrdPtr &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        Detach();
        block = other.block;
        window = other.window;
        other.block = nullptr;
        other.window = nullptr;
        return *this;
    }

    // объект по адресу
    T& operator*() const {
        CheckNotEmpty();
        return *window;
    }

    T* operator->() const {
        CheckNotEmpty();
        return window;
    }

    T* Get() const {
        return window;
    }

    // true, если указатель НЕ пустой
    explicit operator bool() const {
        return window != nullptr;
    }

    // сколько ShrdPtr сейчас делят объект (у пустого — 0)
    size_t GetCount() const {
        return block != nullptr ? block->count : 0;
    }

    // отпустить объект и стать пустым
    void Reset() {
        Detach();
    }

    ~ShrdPtr() {
        Detach();
    }
};

#endif //SMARTPTR_SHRDPTR_H
