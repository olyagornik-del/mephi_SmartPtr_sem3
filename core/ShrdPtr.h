#ifndef SMARTPTR_SHRDPTR_H
#define SMARTPTR_SHRDPTR_H

#include <concepts> // derived_from
#include <cstddef> // size_t
#include <stdexcept> // logic_error, out_of_range
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

// отпустить блок: уменьшить счётчик, и если владельцев не осталось — удалить блок вместе с объектом
// общая функция для ShrdPtr<T> и ShrdPtr<T[]>
inline void ReleaseShrdBlock(ShrdBlockBase *block) {
    if (block == nullptr) {
        return;
    }
    block->count--;
    if (block->count == 0) {
        delete block; // виртуальный деструктор -> ~ShrdBlock<T> -> ~UnqPtr<T> -> delete (или delete[])
    }
}

template <class T>
class ShrdPtr {
private:
    ShrdBlockBase *block; // счётчик + владение
    T *window;

    // ShrdPtr<Cat> и ShrdPtr<Animal> — разные классы и не видят приватные поля друг друга;
    // объявляем все ShrdPtr друзьями, чтобы при подтипизации можно было взять block и window у ShrdPtr<U>
    template <class U>
    friend class ShrdPtr;

    // отцепиться от блока: если мы были последними — удалить блок вместе с объектом
    void Detach() {
        ShrdBlockBase *old = block;
        // чтобы не смотреть на удалённый блок
        block = nullptr;
        window = nullptr;
        ReleaseShrdBlock(old);
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

    // подтипизация: ShrdPtr<Cat> -> ShrdPtr<Animal>, копированием и перемещением
    // requires: эти методы существуют, только если U — наследник T
    template <class U>
        requires std::derived_from<U, T>
    ShrdPtr(const ShrdPtr<U> &other): block(other.block), window(other.window) { // U* -> T* само
        if (block != nullptr) {
            block->count++;
        }
    }

    template <class U>
        requires std::derived_from<U, T>
    ShrdPtr(ShrdPtr<U> &&other) noexcept: block(other.block), window(other.window) {
        other.block = nullptr;
        other.window = nullptr;
    }

    // other другого типа => это точно другой объект ShrdPtr, проверка на самоприсваивание не нужна
    template <class U>
        requires std::derived_from<U, T>
    ShrdPtr& operator=(const ShrdPtr<U> &other) {
        Detach();
        block = other.block;
        window = other.window;
        if (block != nullptr) {
            block->count++;
        }
        return *this;
    }

    template <class U>
        requires std::derived_from<U, T>
    ShrdPtr& operator=(ShrdPtr<U> &&other) noexcept {
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

// версия для массивов: ShrdPtr<int[]> arr(10)
// блоки те же: ShrdBlock<T[]> хранит UnqPtr<T[]>, а он сам удалит массив через delete[]
// подтипизации нет — массив Cat нельзя считать массивом Animal
template <class T>
class ShrdPtr<T[]> {
private:
    ShrdBlockBase *block; // счётчик + владение
    T *window; // на нулевой элемент: только доступ, не владение
    int size; // хранится в каждом ShrdPtr: общий ShrdBlockBase про размер не знает

    void Detach() {
        ShrdBlockBase *old = block;
        block = nullptr;
        window = nullptr;
        size = 0;
        ReleaseShrdBlock(old);
    }

    void CheckIndex(int index) const {
        if (window == nullptr) {
            throw std::logic_error("ShrdPtr<T[]>: пустой указатель");
        }
        if (index < 0 || index >= size) {
            throw std::out_of_range("ShrdPtr<T[]>: индекс вне массива");
        }
    }

public:
    ShrdPtr(): block(nullptr), window(nullptr), size(0) {}

    // создаём массив через UnqPtr<T[]> и сразу отдаём его конструктору ниже
    explicit ShrdPtr(int size): ShrdPtr(UnqPtr<T[]>(size)) {}

    // забираем массив у UnqPtr<T[]> вместе с размером
    explicit ShrdPtr(UnqPtr<T[]> &&source): block(nullptr), window(nullptr), size(0) {
        if (!source) {
            return;
        }
        window = source.Get();
        size = source.GetSize();
        // если new не сработает, source ещё владеет массивом и сам его удалит
        block = new ShrdBlock<T[]>(std::move(source));
    }

    // копия — тот же массив, ещё один владелец
    ShrdPtr(const ShrdPtr &other): block(other.block), window(other.window), size(other.size) {
        if (block != nullptr) {
            block->count++;
        }
    }

    ShrdPtr& operator=(const ShrdPtr &other) {
        if (this == &other) {
            return *this;
        }
        Detach();
        block = other.block;
        window = other.window;
        size = other.size;
        if (block != nullptr) {
            block->count++;
        }
        return *this;
    }

    // перемещение — счётчик не меняется
    ShrdPtr(ShrdPtr &&other) noexcept: block(other.block), window(other.window), size(other.size) {
        other.block = nullptr;
        other.window = nullptr;
        other.size = 0;
    }

    ShrdPtr& operator=(ShrdPtr &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        Detach();
        block = other.block;
        window = other.window;
        size = other.size;
        other.block = nullptr;
        other.window = nullptr;
        other.size = 0;
        return *this;
    }

    T& operator[](int index) const {
        CheckIndex(index);
        return window[index];
    }

    T* Get() const {
        return window;
    }

    int GetSize() const {
        return size;
    }

    explicit operator bool() const {
        return window != nullptr;
    }

    size_t GetCount() const {
        return block != nullptr ? block->count : 0;
    }

    void Reset() {
        Detach();
    }

    ~ShrdPtr() {
        Detach();
    }
};

#endif //SMARTPTR_SHRDPTR_H
