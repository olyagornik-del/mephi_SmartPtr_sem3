#include <cassert> // assert
#include <cstdio> // printf
#include <stdexcept> // logic_error, runtime_error
#include <utility> // move

#include "../core/UnqPtr.h"
#include "Tracked.h"

// возвращаем UnqPtr из функции — должен переместиться, а не скопироваться
static UnqPtr<Tracked> MakeTracked(int value) {
    return UnqPtr<Tracked>(new Tracked(value));
}

void testUnqPtrCreate() {
    // конструктор по умолчанию — пустой
    UnqPtr<int> Default;
    assert(Default.Get() == nullptr);
    assert(!Default);

    // из сырого указателя — владеем именно этим объектом
    int *raw = new int(5);
    UnqPtr<int> FromRaw(raw);
    assert(FromRaw.Get() == raw);
    assert(FromRaw);
    assert(*FromRaw == 5);

    printf("  [OK] testUnqPtrCreate\n");
}

void testUnqPtrCopy() {
    {
        UnqPtr<Tracked> Original(new Tracked(7));
        UnqPtr<Tracked> Copy(Original);

        // глубокая копия — другой объект с тем же значением
        assert(Copy.Get() != Original.Get());
        assert(Copy->value == 7);
        assert(Tracked::alive == 2);

        // копии независимы
        Copy->value = 99;
        assert(Original->value == 7);

        // копия пустого — тоже пустая, новый объект не создаётся
        UnqPtr<Tracked> Empty;
        UnqPtr<Tracked> EmptyCopy(Empty);
        assert(!EmptyCopy);
        assert(Tracked::alive == 2);
    }
    // оба объекта удалились
    assert(Tracked::alive == 0);

    printf("  [OK] testUnqPtrCopy\n");
}

void testUnqPtrCopyAssign() {
    {
        UnqPtr<Tracked> A(new Tracked(1));
        UnqPtr<Tracked> B(new Tracked(2));

        B = A;
        assert(B->value == 1);
        assert(B.Get() != A.Get());
        // старый объект B удалён, иначе было бы 3
        assert(Tracked::alive == 2);

        // самоприсваивание — ничего не меняется
        // (через ссылку, чтобы компилятор не ругался на явное A = A)
        Tracked *before = A.Get();
        UnqPtr<Tracked> &SameA = A;
        A = SameA;
        assert(A.Get() == before);
        assert(A->value == 1);

        // присваивание пустого — B становится пустым, его объект удаляется
        UnqPtr<Tracked> Empty;
        B = Empty;
        assert(!B);
        assert(Tracked::alive == 1);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testUnqPtrCopyAssign\n");
}

void testUnqPtrMove() {
    {
        UnqPtr<Tracked> Source(new Tracked(3));
        Tracked *raw = Source.Get();

        UnqPtr<Tracked> Target(std::move(Source));
        // тот же самый объект, новый не создан
        assert(Target.Get() == raw);
        assert(Tracked::alive == 1);
        // источник опустел
        assert(!Source);

        // перемещение пустого
        UnqPtr<Tracked> Empty;
        UnqPtr<Tracked> FromEmpty(std::move(Empty));
        assert(!FromEmpty);

        // возврат из функции
        UnqPtr<Tracked> Made = MakeTracked(8);
        assert(Made->value == 8);
        assert(Tracked::alive == 2);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testUnqPtrMove\n");
}

void testUnqPtrMoveAssign() {
    {
        UnqPtr<Tracked> A(new Tracked(1));
        UnqPtr<Tracked> B(new Tracked(2));
        Tracked *raw = A.Get();

        B = std::move(A);
        assert(B.Get() == raw);
        assert(!A);
        // старый объект B удалён
        assert(Tracked::alive == 1);

        // самоперемещение — объект не должен удалиться
        UnqPtr<Tracked> &SameB = B;
        B = std::move(SameB);
        assert(B.Get() == raw);
        assert(B->value == 1);

        // временный объект справа
        B = UnqPtr<Tracked>(new Tracked(5));
        assert(B->value == 5);
        assert(Tracked::alive == 1);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testUnqPtrMoveAssign\n");
}

void testUnqPtrAccess() {
    UnqPtr<Tracked> P(new Tracked(4));
    assert((*P).value == 4);
    assert(P->value == 4);

    // меняем через * и через -> — меняется сам объект, а не копия
    (*P).value = 10;
    assert(P->value == 10);
    P->value = 20;
    assert((*P).value == 20);

    // пустой указатель — исключение, а не падение программы
    UnqPtr<Tracked> Empty;
    bool threw = false;
    try { (void)*Empty; } catch (const std::logic_error&) { threw = true; }
    assert(threw);

    threw = false;
    try { Empty->value = 1; } catch (const std::logic_error&) { threw = true; }
    assert(threw);

    printf("  [OK] testUnqPtrAccess\n");
}

void testUnqPtrRelease() {
    UnqPtr<Tracked> P(new Tracked(6));
    Tracked *raw = P.Get();

    Tracked *released = P.Release();
    assert(released == raw);
    assert(!P);
    // объект НЕ удалён — теперь за него отвечаем мы
    assert(Tracked::alive == 1);
    delete released;
    assert(Tracked::alive == 0);

    // Release у пустого
    UnqPtr<Tracked> Empty;
    assert(Empty.Release() == nullptr);

    printf("  [OK] testUnqPtrRelease\n");
}

void testUnqPtrReset() {
    UnqPtr<Tracked> P(new Tracked(1));

    // старый удаляется, берём новый
    P.Reset(new Tracked(2));
    assert(P->value == 2);
    assert(Tracked::alive == 1);

    // свой же адрес — объект не должен удалиться
    P.Reset(P.Get());
    assert(P->value == 2);
    assert(Tracked::alive == 1);

    // без аргумента — удалить и стать пустым
    P.Reset();
    assert(!P);
    assert(Tracked::alive == 0);

    // Reset у пустого — ничего не ломается
    P.Reset();
    assert(!P);

    printf("  [OK] testUnqPtrReset\n");
}

void testUnqPtrNoLeaks() {
    // много операций подряд — в конце всё должно удалиться
    {
        UnqPtr<Tracked> A(new Tracked(1));
        UnqPtr<Tracked> B(A);
        UnqPtr<Tracked> C(std::move(B));
        B = C;
        C = std::move(A);
        A.Reset(new Tracked(5));
        B.Reset();
        UnqPtr<Tracked> D = MakeTracked(9);
        D = MakeTracked(10);
    }
    assert(Tracked::alive == 0);

    // исключение посреди блока — деструктор всё равно вызывается (RAII)
    try {
        UnqPtr<Tracked> P(new Tracked(1));
        throw std::runtime_error("test");
    } catch (const std::runtime_error&) {}
    assert(Tracked::alive == 0);

    printf("  [OK] testUnqPtrNoLeaks\n");
}

void testUnqPtrAll() {
    printf("=== Тесты UnqPtr ===\n");
    testUnqPtrCreate();
    testUnqPtrCopy();
    testUnqPtrCopyAssign();
    testUnqPtrMove();
    testUnqPtrMoveAssign();
    testUnqPtrAccess();
    testUnqPtrRelease();
    testUnqPtrReset();
    testUnqPtrNoLeaks();
    printf("=== Все тесты пройдены! ===\n\n");
}
