#include <cassert> // assert
#include <cstdio> // printf
#include <stdexcept> // logic_error, out_of_range, invalid_argument
#include <type_traits> // is_constructible и др.
#include <utility> // move

#include "../core/UnqPtr.h"
#include "../core/ShrdPtr.h"
#include "Animals.h"
#include "Tracked.h"

void testArrayCompile() {
    // копировать UnqPtr<T[]> нельзя, перемещать можно
    static_assert(!std::is_copy_constructible_v<UnqPtr<int[]>>);
    static_assert(!std::is_copy_assignable_v<UnqPtr<int[]>>);
    static_assert(std::is_move_constructible_v<UnqPtr<int[]>>);

    // массив создаётся только по размеру: из сырого указателя нельзя,
    // и число не превращается в массив само (explicit)
    static_assert(!std::is_constructible_v<UnqPtr<int[]>, int*>);
    static_assert(!std::is_convertible_v<int, UnqPtr<int[]>>);

    // указатель на объект и указатель на массив — разные вещи, друг в друга не превращаются
    static_assert(!std::is_constructible_v<UnqPtr<int>, UnqPtr<int[]>&&>);
    static_assert(!std::is_constructible_v<UnqPtr<int[]>, UnqPtr<int>&&>);

    // подтипизации для массивов нет: массив Cat нельзя считать массивом Animal
    static_assert(!std::is_constructible_v<UnqPtr<Animal[]>, UnqPtr<Cat[]>&&>);
    static_assert(!std::is_constructible_v<ShrdPtr<Animal[]>, const ShrdPtr<Cat[]>&>);
    static_assert(!std::is_constructible_v<ShrdPtr<Animal[]>, ShrdPtr<Cat[]>&&>);

    printf("  [OK] testArrayCompile\n");
}

void testUnqArrCreate() {
    // по умолчанию — пустой
    UnqPtr<int[]> Default;
    assert(!Default);
    assert(Default.Get() == nullptr);
    assert(Default.GetSize() == 0);

    // с размером — элементы заполнены нулями
    UnqPtr<int[]> Array(5);
    assert(Array);
    assert(Array.GetSize() == 5);
    for (int i = 0; i < 5; i++) {
        assert(Array[i] == 0);
    }

    // размер 0 — пустой, память не выделяется
    UnqPtr<int[]> Zero(0);
    assert(!Zero);
    assert(Zero.GetSize() == 0);

    // отрицательный размер — исключение
    bool threw = false;
    try { UnqPtr<int[]> Bad(-1); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    printf("  [OK] testUnqArrCreate\n");
}

void testUnqArrIndex() {
    UnqPtr<int[]> Array(3);
    Array[0] = 10;
    Array[2] = 30;
    assert(Array[0] == 10);
    assert(Array[1] == 0);
    assert(Array[2] == 30);

    // выход за границы — с обеих сторон
    bool threw = false;
    try { Array[3] = 1; } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    threw = false;
    try { Array[-1] = 1; } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    // у пустого — logic_error, как * у пустого UnqPtr
    UnqPtr<int[]> Empty;
    threw = false;
    try { Empty[0] = 1; } catch (const std::logic_error&) { threw = true; }
    assert(threw);

    printf("  [OK] testUnqArrIndex\n");
}

void testUnqArrDelete() {
    // если бы удаление шло через delete, а не delete[], уничтожился бы только один элемент
    {
        UnqPtr<Tracked[]> Array(5);
        assert(Tracked::alive == 5);
    }
    assert(Tracked::alive == 0);

    // Reset — удалить массив и стать пустым
    UnqPtr<Tracked[]> Array(3);
    Array.Reset();
    assert(!Array);
    assert(Array.GetSize() == 0);
    assert(Tracked::alive == 0);

    printf("  [OK] testUnqArrDelete\n");
}

void testUnqArrMove() {
    {
        UnqPtr<Tracked[]> Source(4);
        Tracked *raw = Source.Get();

        // массив переезжает вместе с размером
        UnqPtr<Tracked[]> Target(std::move(Source));
        assert(Target.Get() == raw);
        assert(Target.GetSize() == 4);
        assert(!Source);
        assert(Source.GetSize() == 0);
        assert(Tracked::alive == 4);

        // присваивание: старый массив удаляется
        UnqPtr<Tracked[]> Other(2);
        Target = std::move(Other);
        assert(Target.GetSize() == 2);
        assert(Tracked::alive == 2);

        // самоперемещение — массив не удаляется
        UnqPtr<Tracked[]> &SameTarget = Target;
        Target = std::move(SameTarget);
        assert(Target.GetSize() == 2);
        assert(Tracked::alive == 2);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testUnqArrMove\n");
}

void testShrdArrCreate() {
    {
        ShrdPtr<int[]> Default;
        assert(!Default);
        assert(Default.GetSize() == 0);
        assert(Default.GetCount() == 0);

        ShrdPtr<Tracked[]> Array(3);
        assert(Array.GetSize() == 3);
        assert(Array.GetCount() == 1);
        assert(Tracked::alive == 3);

        // из UnqPtr<T[]> — массив переезжает, не копируется
        UnqPtr<Tracked[]> Unique(2);
        Tracked *raw = Unique.Get();
        ShrdPtr<Tracked[]> FromUnique(std::move(Unique));
        assert(!Unique);
        assert(FromUnique.Get() == raw);
        assert(FromUnique.GetSize() == 2);
        assert(Tracked::alive == 5);

        bool threw = false;
        try { ShrdPtr<int[]> Bad(-1); } catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdArrCreate\n");
}

void testShrdArrShare() {
    {
        ShrdPtr<int[]> A(4);
        ShrdPtr<int[]> B(A);

        // копия — тот же массив: изменение через B видно через A
        assert(B.Get() == A.Get());
        assert(B.GetSize() == 4);
        assert(A.GetCount() == 2);
        B[2] = 7;
        assert(A[2] == 7);
    }

    // массив живёт, пока жив последний владелец
    {
        ShrdPtr<Tracked[]> Outer;
        {
            ShrdPtr<Tracked[]> Inner(3);
            Outer = Inner;
            assert(Outer.GetCount() == 2);
        }
        assert(Tracked::alive == 3);
        assert(Outer.GetCount() == 1);
        assert(Outer.GetSize() == 3);

        // Reset последнего — массив удаляется целиком
        Outer.Reset();
        assert(Tracked::alive == 0);
        assert(Outer.GetSize() == 0);
    }

    printf("  [OK] testShrdArrShare\n");
}

void testShrdArrMove() {
    {
        ShrdPtr<Tracked[]> A(2);
        ShrdPtr<Tracked[]> B(A);

        // перемещение — владельцев столько же
        ShrdPtr<Tracked[]> C(std::move(B));
        assert(!B);
        assert(B.GetSize() == 0);
        assert(A.GetCount() == 2);
        assert(C.GetSize() == 2);

        // перемещающее присваивание: у D был свой массив — он удаляется
        ShrdPtr<Tracked[]> D(5);
        assert(Tracked::alive == 7);
        D = std::move(C);
        assert(Tracked::alive == 2);
        assert(D.GetSize() == 2);
        assert(A.GetCount() == 2);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdArrMove\n");
}

void testShrdArrIndex() {
    ShrdPtr<int[]> Array(2);
    Array[1] = 5;
    assert(Array[1] == 5);

    bool threw = false;
    try { Array[2] = 1; } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    threw = false;
    try { Array[-1] = 1; } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    ShrdPtr<int[]> Empty;
    threw = false;
    try { Empty[0] = 1; } catch (const std::logic_error&) { threw = true; }
    assert(threw);

    printf("  [OK] testShrdArrIndex\n");
}

void testArrayAll() {
    printf("=== Тесты массивов ===\n");
    testArrayCompile();
    testUnqArrCreate();
    testUnqArrIndex();
    testUnqArrDelete();
    testUnqArrMove();
    testShrdArrCreate();
    testShrdArrShare();
    testShrdArrMove();
    testShrdArrIndex();
    printf("=== Все тесты пройдены! ===\n\n");
}
