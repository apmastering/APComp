#pragma once

#include <vector>
#include <mutex>


class CircularBuffer {
public:
    CircularBuffer(size_t size) : size(size), buffer(size), head(0), tail(0), isFull(false) {}

    void add(double value) {
        
        std::lock_guard<std::mutex> lock(mutex);
        
        buffer[head] = value;
        head = (head + 1) % size;

        if (isFull) {
            tail = (tail + 1) % size;
        }

        isFull = head == tail;
    }

    double average() const {
        
        std::lock_guard<std::mutex> lock(mutex);

        if (head == tail && !isFull) {
            return 0.0;
        }

        double sum = 0.0;
        size_t count = 0;

        size_t index = tail;
        while (true) {
            sum += buffer[index];
            count++;
            if (index == head) break;
            index = (index + 1) % size;
        }

        return sum / count;
    }

private:
    size_t size;
    std::vector<double> buffer;
    size_t head;
    size_t tail;
    bool isFull;
    mutable std::mutex mutex; // Mutex for synchronizing access
};
