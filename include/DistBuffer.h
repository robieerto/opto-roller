#ifndef OPTO_DISTANCE_DISTBUFFER_H
#define OPTO_DISTANCE_DISTBUFFER_H

#include <cstdlib>
#include <cstdint>

template <class Any_Type> class DistBuffer {
public:
    explicit DistBuffer(const size_t &size);
    void addValue(const Any_Type &value);
    void addValues(const Any_Type *new_values, size_t num, Any_Type new_sum);
    void clear();
    Any_Type getAverage() const;
    Any_Type *getActual();
    Any_Type* values;
    size_t size;
    uint32_t index{};
    size_t numElems{};
    Any_Type sum;
    bool isFull;
};

template <class Any_Type> DistBuffer<Any_Type>::DistBuffer(const size_t &buff_size)
{
    size = buff_size;
    values = new Any_Type[size]();
    index = 0;
    sum = 0;
    isFull = false;
}

template <class Any_Type> void DistBuffer<Any_Type>::addValue(const Any_Type &value)
{
    if (numElems < size) {
        values[index++] = value;
        numElems++;
        sum += value;
        if (numElems == size) {
            isFull = true;
        }
    }
}

template <class Any_Type> void DistBuffer<Any_Type>::addValues(const Any_Type *new_values, size_t num, Any_Type new_sum)
{
    if (numElems + num < size) {
        memcpy(values, new_values, sizeof(double)*num);
        numElems += num;
        index += num;
        sum += new_sum;
    }
}

template <class Any_Type> Any_Type *DistBuffer<Any_Type>::getActual()
{
    return &values[index];
}

template <class Any_Type> Any_Type DistBuffer<Any_Type>::getAverage() const
{
    return (sum / numElems);
}

template <class Any_Type> void DistBuffer<Any_Type>::clear()
{
    memset(values, 0, sizeof(Any_Type)*numElems);
    numElems = 0;
    index = 0;
    sum = 0;
    isFull = false;
}

#endif //OPTO_DISTANCE_DISTBUFFER_H
