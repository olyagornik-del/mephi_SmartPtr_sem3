#include <cassert> // assert
#include <cstdio> // printf
#include <type_traits> // is_constructible, is_assignable
#include <utility> // move

#include "../core/UnqPtr.h"
#include "../core/ShrdPtr.h"
#include "Animals.h"

// все ли животные удалились
static bool NoAnimalsAlive() {
    return Animal::alive == 0 && Cat::alive == 0 && Dog::alive == 0;
}

// функция обещает UnqPtr<Animal>, а возвращает UnqPtr<Cat> — сработает подтипизация
static UnqPtr<Animal> MakeCat() {
    return UnqPtr<Cat>(new Cat);
}

void testSubtypingCompile() {
    // is_constructible<A, B> — «можно ли создать A из B»,
    // is_assignable<A&, B> — «можно ли A = B»
    // всё проверяется при компиляции: если правило нарушено, файл не соберётся

    // UnqPtr: вверх по иерархии — можно, но только перемещением
    static_assert(std::is_constructible_v<UnqPtr<Animal>, UnqPtr<Cat>&&>);
    static_assert(std::is_assignable_v<UnqPtr<Animal>&, UnqPtr<Cat>&&>);
    static_assert(!std::is_constructible_v<UnqPtr<Animal>, const UnqPtr<Cat>&>); // копировать нельзя

    // UnqPtr: вниз, между «братьями» и между несвязанными типами — нельзя
    static_assert(!std::is_constructible_v<UnqPtr<Cat>, UnqPtr<Animal>&&>);
    static_assert(!std::is_assignable_v<UnqPtr<Cat>&, UnqPtr<Animal>&&>);
    static_assert(!std::is_constructible_v<UnqPtr<Dog>, UnqPtr<Cat>&&>);
    static_assert(!std::is_constructible_v<UnqPtr<Animal>, UnqPtr<int>&&>);

    // ShrdPtr: вверх по иерархии — можно и копированием, и перемещением
    static_assert(std::is_constructible_v<ShrdPtr<Animal>, const ShrdPtr<Cat>&>);
    static_assert(std::is_constructible_v<ShrdPtr<Animal>, ShrdPtr<Cat>&&>);
    static_assert(std::is_assignable_v<ShrdPtr<Animal>&, const ShrdPtr<Cat>&>);
    static_assert(std::is_assignable_v<ShrdPtr<Animal>&, ShrdPtr<Cat>&&>);

    // ShrdPtr: вниз, между «братьями» и между несвязанными типами — нельзя
    static_assert(!std::is_constructible_v<ShrdPtr<Cat>, const ShrdPtr<Animal>&>);
    static_assert(!std::is_assignable_v<ShrdPtr<Cat>&, const ShrdPtr<Animal>&>);
    static_assert(!std::is_constructible_v<ShrdPtr<Dog>, const ShrdPtr<Cat>&>);
    static_assert(!std::is_constructible_v<ShrdPtr<Animal>, const ShrdPtr<int>&>);

    printf("  [OK] testSubtypingCompile\n");
}

void testSubtypingUnqPtrVirtual() {
    {
        // указатель на родителя, а внутри наследник — вызываются методы наследника
        UnqPtr<Animal> A(new Cat);
        UnqPtr<Animal> B(new Dog);
        assert(A->Speak() == "Мяу");
        assert(B->Speak() == "Гав");
        assert(Animal::alive == 2);
    }
    // удаление через UnqPtr<Animal> вызвало ~Cat и ~Dog (благодаря virtual ~Animal)
    assert(NoAnimalsAlive());

    printf("  [OK] testSubtypingUnqPtrVirtual\n");
}

void testSubtypingUnqPtrMove() {
    {
        UnqPtr<Cat> MyCat(new Cat);
        Cat *raw = MyCat.Get();

        // UnqPtr<Cat> -> UnqPtr<Animal>: тот же объект, источник пустой
        UnqPtr<Animal> A(std::move(MyCat));
        assert(A.Get() == raw);
        assert(!MyCat);
        assert(A->Speak() == "Мяу");
        assert(Cat::alive == 1);

        // присваивание: старая кошка удаляется, теперь внутри собака
        UnqPtr<Dog> MyDog(new Dog);
        A = std::move(MyDog);
        assert(!MyDog);
        assert(A->Speak() == "Гав");
        assert(Cat::alive == 0);
        assert(Dog::alive == 1);

        // пустой наследник -> пустой родитель
        UnqPtr<Cat> EmptyCat;
        UnqPtr<Animal> FromEmpty(std::move(EmptyCat));
        assert(!FromEmpty);

        // возврат из функции
        UnqPtr<Animal> Made = MakeCat();
        assert(Made->Speak() == "Мяу");
    }
    assert(NoAnimalsAlive());

    printf("  [OK] testSubtypingUnqPtrMove\n");
}

void testSubtypingShrdPtrCopy() {
    {
        ShrdPtr<Cat> MyCat(new Cat);

        // ShrdPtr<Cat> -> ShrdPtr<Animal>: тот же объект, общий счётчик
        ShrdPtr<Animal> A(MyCat);
        assert(A.Get() == MyCat.Get());
        assert(A.GetCount() == 2);
        assert(MyCat.GetCount() == 2);
        assert(A->Speak() == "Мяу");
        assert(Cat::alive == 1);

        // кошка живёт, пока на неё смотрит хоть кто-то — даже если это ShrdPtr<Animal>
        MyCat.Reset();
        assert(Cat::alive == 1);
        assert(A.GetCount() == 1);

        // присваивание: собака была только у B — удаляется
        ShrdPtr<Animal> B(new Dog);
        ShrdPtr<Cat> OtherCat(new Cat);
        B = OtherCat;
        assert(Dog::alive == 0);
        assert(B->Speak() == "Мяу");
        assert(OtherCat.GetCount() == 2);
    }
    // последним владельцем был ShrdPtr<Animal>, а удалилась именно кошка
    assert(NoAnimalsAlive());

    printf("  [OK] testSubtypingShrdPtrCopy\n");
}

void testSubtypingShrdPtrMove() {
    {
        ShrdPtr<Cat> MyCat(new Cat);
        Cat *raw = MyCat.Get();

        // перемещение вверх: владельцев столько же
        ShrdPtr<Animal> A(std::move(MyCat));
        assert(A.Get() == raw);
        assert(A.GetCount() == 1);
        assert(!MyCat);

        // перемещающее присваивание: старая кошка удаляется
        ShrdPtr<Dog> MyDog(new Dog);
        A = std::move(MyDog);
        assert(!MyDog);
        assert(A->Speak() == "Гав");
        assert(Cat::alive == 0);
        assert(A.GetCount() == 1);
    }
    assert(NoAnimalsAlive());

    printf("  [OK] testSubtypingShrdPtrMove\n");
}

void testSubtypingShrdPtrFromUnqPtr() {
    {
        // UnqPtr<Cat> -> ShrdPtr<Animal>: сразу и смена владения, и подтипизация
        UnqPtr<Cat> MyCat(new Cat);
        ShrdPtr<Animal> A(std::move(MyCat));
        assert(!MyCat);
        assert(A->Speak() == "Мяу");
        assert(A.GetCount() == 1);
    }
    assert(NoAnimalsAlive());

    printf("  [OK] testSubtypingShrdPtrFromUnqPtr\n");
}

void testSubtypingZoo() {
    {
        // главное, зачем нужна подтипизация: разные животные в одном массиве
        ShrdPtr<Animal> Zoo[] = {ShrdPtr<Cat>(new Cat), ShrdPtr<Dog>(new Dog), ShrdPtr<Cat>(new Cat)};
        assert(Zoo[0]->Speak() == "Мяу");
        assert(Zoo[1]->Speak() == "Гав");
        assert(Zoo[2]->Speak() == "Мяу");
        assert(Cat::alive == 2);
        assert(Dog::alive == 1);
    }
    assert(NoAnimalsAlive());

    printf("  [OK] testSubtypingZoo\n");
}

void testSubtypingAll() {
    printf("=== Тесты подтипизации ===\n");
    testSubtypingCompile();
    testSubtypingUnqPtrVirtual();
    testSubtypingUnqPtrMove();
    testSubtypingShrdPtrCopy();
    testSubtypingShrdPtrMove();
    testSubtypingShrdPtrFromUnqPtr();
    testSubtypingZoo();
    printf("=== Все тесты пройдены! ===\n\n");
}
