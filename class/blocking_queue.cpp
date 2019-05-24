#ifndef BLOCKING_QUEUE
#define BLOCKING_QUEUE

#include <iostream>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <condition_variable>

using namespace std;

/**
 * Blocking queue used for thread communication
 */
template <typename T>
class blocking_queue
{
private:
    mutex                d_mutex;
    condition_variable   d_condition;
    deque<T>             d_deque;

public:
    void push(T const &value)
    {
        {
            unique_lock<mutex> lock(this->d_mutex);
            this->d_deque.push_front(value);
        }
        this->d_condition.notify_one();
    }

    T pop()
    {
        unique_lock<mutex> lock(this->d_mutex);
        this->d_condition.wait(lock, [ = ] { return !this->d_deque.empty(); });

        T result(move(this->d_deque.back()));
        this->d_deque.pop_back();
        return result;
    }

};

#endif