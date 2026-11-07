// Заготовка кольцевого буфера. Занятие 3.1.
//
// Скопируйте файл в `src/` и доведите до рабочего состояния. С этого момента
// он ваш: правьте что угодно, включая этот комментарий.
//
// ЧТО ЗДЕСЬ ЛЕЖИТ
//
// Публичный интерфейс целиком и ни одного тела. Интерфейс менять нельзя —
// против него собраны выданные тесты и выданный `ThresholdRule`. Всё
// остальное ваше: приватные поля, как считается физический индекс, как
// устроен итератор.
//
// ЗАДАНИЕ
//
// 1. Сделать буфер шаблоном `RingBuffer<T, N>`. Сейчас он хранит `Timestamp`,
//    а вместимость задана константой — так его хватило бы одному
//    `ThresholdRule` и никому больше: окно агента хранит не отметки времени.
//    `T` — параметр-тип, `N` — non-type параметр (лекция 8): размер попадает
//    в тип, поэтому `RingBuffer<int, 4>` и `RingBuffer<int, 64>` — разные
//    типы, и перепутать их нельзя.
// 2. Написать реализацию. Тела пишутся прямо здесь: шаблон нельзя разложить
//    на заголовок и `.cpp` — компилятору нужно тело в той единице трансляции,
//    где шаблон используется.
//
// Требования к поведению и границы, которые проверяются, — в постановке
// занятия.

#ifndef NANO_EDR_RING_BUFFER_H
#define NANO_EDR_RING_BUFFER_H

#include <array>
#include <cstddef>

namespace nano_edr {

template <typename T, std::size_t N>
class RingBuffer {
 public:
    // Обход в логическом порядке — от самого старого к самому новому.
    //
    // Итератор минимальный: ровно то, что нужно range-based for. Как он
    // добирается до элементов буфера, решаете вы; приватной части здесь нет
    // намеренно.
    class Iterator {
     public:
        Iterator(const RingBuffer* ring, std::size_t index) : ring_(ring), index_(index) {}

        const T& operator*() const { return (*ring_)[index_]; }
        Iterator& operator++() {
            ++index_;
            return *this;
        }
        bool operator!=(const Iterator& other) const { return ring_ != other.ring_ || index_ != other.index_; }

     private:
        const RingBuffer* ring_ = nullptr;
        std::size_t index_ = 0;
    };

    // Дописывает элемент. Если буфер полон, самый старый затирается — окно,
    // а не архив.
    void PushBack(const T& value) {
        buffer_[head_] = value;
        head_ = (head_ + 1) % N;
        if (!full()) {
            ++size_;
        }
    }

    void Clear() {
        size_ = 0;
        head_ = 0;
    }

    std::size_t size() const { return size_; }
    static constexpr std::size_t capacity() { return N; }
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == N; }

    // По логическому индексу: 0 — самый старый.
    const T& operator[](std::size_t index) const { return (!full()) ? buffer_[index] : buffer_[(index + head_) % N]; }

    // Самый новый элемент. Вызывать на пустом буфере нельзя — это ошибка
    // в коде вызывающего, а не случай, который надо обрабатывать.
    const T& back() const { return buffer_[(head_ + N - 1) % N]; }

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, size_); }

 private:
    std::size_t size_ = 0;
    std::size_t head_ = 0;
    std::array<T, N> buffer_{};
};

}  // namespace nano_edr

#endif  // NANO_EDR_RING_BUFFER_H
