#ifndef SMARTPTR_ANIMALS_H
#define SMARTPTR_ANIMALS_H

#include <string>

// для проверки подтипизации: Animal -> Cat, Dog
// у каждого класса свой счётчик живых объектов: по нему видно, какие деструкторы сработали

class Animal {
public:
    static inline int alive = 0; // сколько животных (любых) сейчас живо

    Animal() {
        alive++;
    }
    // удаляем наследников через Animal* => деструктор обязан быть виртуальным иначе ~Cat не вызовется
    virtual ~Animal() {
        alive--;
    }

    virtual std::string Speak() const {
        return "...";
    }
};

class Cat : public Animal {
public:
    static inline int alive = 0; // сколько именно кошек живо (Cat::alive, не путать с Animal::alive)

    Cat() {
        alive++;
    }
    ~Cat() override {
        alive--;
    }

    std::string Speak() const override {
        return "Мяу";
    }
};

class Dog : public Animal {
public:
    static inline int alive = 0;

    Dog() {
        alive++;
    }
    ~Dog() override {
        alive--;
    }

    std::string Speak() const override {
        return "Гав";
    }
};

#endif //SMARTPTR_ANIMALS_H
