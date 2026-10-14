#include <cassert> // assert
#include <cstdio> // printf
#include <stdexcept> // logic_error, runtime_error
#include <utility> // move

#include "../core/ShrdPtr.h"
#include "Tracked.h"

// возвращаем ShrdPtr из функции — должен переместиться, счётчик не растёт
static ShrdPtr<Tracked> MakeShrdTracked(int value) {
    return ShrdPtr<Tracked>(new Tracked(value));
}

void testShrdPtrCreate() {
    // конструктор по умолчанию — пустой, блока нет
    ShrdPtr<int> Default;
    assert(Default.Get() == nullptr);
    assert(!Default);
    assert(Default.GetCount() == 0);

    // из сырого указателя — один владелец
    int *raw = new int(5);
    ShrdPtr<int> FromRaw(raw);
    assert(FromRaw.Get() == raw);
    assert(FromRaw);
    assert(FromRaw.GetCount() == 1);
    assert(*FromRaw == 5);

    // из nullptr — тоже пустой
    ShrdPtr<int> FromNull(nullptr);
    assert(!FromNull);
    assert(FromNull.GetCount() == 0);

    printf("  [OK] testShrdPtrCreate\n");
}

void testShrdPtrFromUnqPtr() {
    {
        UnqPtr<Tracked> Unique(new Tracked(5));
        Tracked *raw = Unique.Get();

        ShrdPtr<Tracked> Shared(std::move(Unique));
        // объект переехал, а не скопировался
        assert(Shared.Get() == raw);
        assert(Shared.GetCount() == 1);
        assert(Tracked::alive == 1);
        // UnqPtr отдал объект и опустел
        assert(!Unique);

        // из пустого UnqPtr — пустой ShrdPtr
        UnqPtr<Tracked> Empty;
        ShrdPtr<Tracked> FromEmpty(std::move(Empty));
        assert(!FromEmpty);
        assert(FromEmpty.GetCount() == 0);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdPtrFromUnqPtr\n");
}

void testShrdPtrCopy() {
    {
        ShrdPtr<Tracked> A(new Tracked(1));
        ShrdPtr<Tracked> B(A);

        // копия смотрит на ТОТ ЖЕ объект, новый не создаётся
        assert(B.Get() == A.Get());
        assert(Tracked::alive == 1);
        assert(A.GetCount() == 2);
        assert(B.GetCount() == 2);

        // изменение через одного видно через другого — передача по ссылке
        B->value = 5;
        assert(A->value == 5);

        // копия пустого — пустая
        ShrdPtr<Tracked> Empty;
        ShrdPtr<Tracked> EmptyCopy(Empty);
        assert(!EmptyCopy);
        assert(EmptyCopy.GetCount() == 0);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdPtrCopy\n");
}

void testShrdPtrLifetime() {
    // объект живёт, пока жив хотя бы один ShrdPtr
    {
        ShrdPtr<Tracked> A(new Tracked(1));
        {
            ShrdPtr<Tracked> B(A);
            ShrdPtr<Tracked> C(A);
            assert(A.GetCount() == 3);
        }
        // B и C умерли — объект ещё жив, у него остался A
        assert(A.GetCount() == 1);
        assert(Tracked::alive == 1);
    }
    // последний владелец удалил объект
    assert(Tracked::alive == 0);

    // объект переживает того, кто его создал
    {
        ShrdPtr<Tracked> Outer;
        {
            ShrdPtr<Tracked> Inner(new Tracked(3));
            Outer = Inner;
        }
        assert(Tracked::alive == 1);
        assert(Outer->value == 3);
        assert(Outer.GetCount() == 1);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdPtrLifetime\n");
}

void testShrdPtrCopyAssign() {
    {
        ShrdPtr<Tracked> A(new Tracked(1));
        ShrdPtr<Tracked> B(new Tracked(2));

        // у Tracked(2) был единственный владелец B — он удаляется
        B = A;
        assert(B.Get() == A.Get());
        assert(A.GetCount() == 2);
        assert(Tracked::alive == 1);

        // у старого объекта есть ещё владелец — он НЕ удаляется
        ShrdPtr<Tracked> C(new Tracked(3));
        ShrdPtr<Tracked> D(C);
        C = A;
        assert(Tracked::alive == 2);
        assert(D->value == 3);
        assert(D.GetCount() == 1);
        assert(A.GetCount() == 3);

        // присваивание того, на что уже смотрим, — счётчик не меняется
        B = A;
        assert(A.GetCount() == 3);

        // самоприсваивание (через ссылку, чтобы компилятор не ругался)
        ShrdPtr<Tracked> &SameA = A;
        A = SameA;
        assert(A.GetCount() == 3);
        assert(A->value == 1);

        // присваивание пустого — B отпускает объект
        ShrdPtr<Tracked> Empty;
        B = Empty;
        assert(!B);
        assert(B.GetCount() == 0);
        assert(A.GetCount() == 2);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdPtrCopyAssign\n");
}

void testShrdPtrMove() {
    {
        ShrdPtr<Tracked> Source(new Tracked(3));
        Tracked *raw = Source.Get();

        ShrdPtr<Tracked> Target(std::move(Source));
        assert(Target.Get() == raw);
        assert(Target.GetCount() == 1);
        assert(!Source);
        assert(Source.GetCount() == 0);

        // перемещение не меняет число владельцев
        ShrdPtr<Tracked> Copy(Target);
        assert(Target.GetCount() == 2);
        ShrdPtr<Tracked> Moved(std::move(Copy));
        assert(Target.GetCount() == 2);

        // возврат из функции
        ShrdPtr<Tracked> Made = MakeShrdTracked(8);
        assert(Made->value == 8);
        assert(Made.GetCount() == 1);
        assert(Tracked::alive == 2);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdPtrMove\n");
}

void testShrdPtrMoveAssign() {
    {
        ShrdPtr<Tracked> A(new Tracked(1));
        ShrdPtr<Tracked> B(new Tracked(2));
        Tracked *raw = A.Get();

        B = std::move(A);
        assert(B.Get() == raw);
        assert(B.GetCount() == 1);
        assert(!A);
        // старый объект B удалён
        assert(Tracked::alive == 1);

        // самоперемещение — объект не должен удалиться
        ShrdPtr<Tracked> &SameB = B;
        B = std::move(SameB);
        assert(B.Get() == raw);
        assert(B.GetCount() == 1);

        // временный объект справа
        B = ShrdPtr<Tracked>(new Tracked(7));
        assert(B->value == 7);
        assert(Tracked::alive == 1);
    }
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdPtrMoveAssign\n");
}

void testShrdPtrAccess() {
    ShrdPtr<Tracked> P(new Tracked(4));
    assert((*P).value == 4);
    assert(P->value == 4);

    (*P).value = 10;
    assert(P->value == 10);
    P->value = 20;
    assert((*P).value == 20);

    // пустой указатель — исключение
    ShrdPtr<Tracked> Empty;
    bool threw = false;
    try { (void)*Empty; } catch (const std::logic_error&) { threw = true; }
    assert(threw);

    threw = false;
    try { Empty->value = 1; } catch (const std::logic_error&) { threw = true; }
    assert(threw);

    printf("  [OK] testShrdPtrAccess\n");
}

void testShrdPtrReset() {
    ShrdPtr<Tracked> A(new Tracked(1));
    ShrdPtr<Tracked> B(A);

    // A отпустил, но объект жив — у него есть B
    A.Reset();
    assert(!A);
    assert(A.GetCount() == 0);
    assert(B.GetCount() == 1);
    assert(Tracked::alive == 1);

    // последний отпустил — объект удалён
    B.Reset();
    assert(!B);
    assert(Tracked::alive == 0);

    // Reset у пустого — ничего не ломается
    B.Reset();
    assert(!B);

    printf("  [OK] testShrdPtrReset\n");
}

void testShrdPtrNoLeaks() {
    {
        ShrdPtr<Tracked> A(new Tracked(1));
        ShrdPtr<Tracked> B(A);
        ShrdPtr<Tracked> C(std::move(B));
        B = C;
        C = MakeShrdTracked(2);
        A.Reset();
        ShrdPtr<Tracked> D(UnqPtr<Tracked>(new Tracked(3)));
        D = B;
        B = std::move(C);
    }
    assert(Tracked::alive == 0);

    // исключение посреди блока — все владельцы отпускают объект (RAII)
    try {
        ShrdPtr<Tracked> P(new Tracked(1));
        ShrdPtr<Tracked> Q(P);
        throw std::runtime_error("test");
    } catch (const std::runtime_error&) {}
    assert(Tracked::alive == 0);

    printf("  [OK] testShrdPtrNoLeaks\n");
}

void testShrdPtrAll() {
    printf("=== Тесты ShrdPtr ===\n");
    testShrdPtrCreate();
    testShrdPtrFromUnqPtr();
    testShrdPtrCopy();
    testShrdPtrLifetime();
    testShrdPtrCopyAssign();
    testShrdPtrMove();
    testShrdPtrMoveAssign();
    testShrdPtrAccess();
    testShrdPtrReset();
    testShrdPtrNoLeaks();
    printf("=== Все тесты пройдены! ===\n\n");
}
