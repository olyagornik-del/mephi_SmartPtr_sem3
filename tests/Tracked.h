#ifndef SMARTPTR_TRACKED_H
#define SMARTPTR_TRACKED_H

// тестовый класс: считает, сколько его объектов сейчас живо
// по alive видно, удалился ли объект, и нет ли утечек
class Tracked {
public:
    static inline int alive = 0; // общий для всех объектов Tracked
    int value;

    explicit Tracked(int value = 0): value(value) {
        alive++;
    }
    // копия — это новый объект, его тоже считаем
    Tracked(const Tracked &other): value(other.value) {
        alive++;
    }
    // присваивание новый объект не создаёт, счётчик не меняем
    Tracked& operator=(const Tracked &other) = default;

    ~Tracked() {
        alive--;
    }
};

#endif //SMARTPTR_TRACKED_H
