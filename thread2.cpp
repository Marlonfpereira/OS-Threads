#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

const int ORDER_BUFFER_SIZE = 10;
const int NUM_PRODUCERS = 3;

struct Order
{
    std::thread::id threadId;
    int orderId;

    Order(std::thread::id id, int orderId) : threadId(id), orderId(orderId) {}
};

std::queue<Order> orders;
std::mutex mutex;
std::condition_variable orderReady;
std::atomic<int> orderIdCounter(1);

void producer()
{
    std::thread::id threadId = std::this_thread::get_id();

    for (int i = 0; i < 5; ++i)
    {
        Order newOrder(threadId, orderIdCounter++);

        std::unique_lock<std::mutex> lock(mutex);
        if (orders.size() >= ORDER_BUFFER_SIZE)
        {
            // The buffer is full, wait for consumer to make space
            orderReady.wait(lock, []
                            { return orders.size() < ORDER_BUFFER_SIZE; });
        }
        orders.push(newOrder);
        std::cout << "Producer " << threadId << " produced Order " << newOrder.orderId << std::endl;
        lock.unlock();

        orderReady.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void consumer()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mutex);
        orderReady.wait(lock, []
                        { return !orders.empty(); });
        Order order = orders.front();
        orders.pop();
        lock.unlock();

        std::cout << "Consumer received Order " << order.orderId << " from Producer " << order.threadId << std::endl;

        // Signal to producers that there's space in the buffer
        orderReady.notify_all();

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

int main()
{
    std::thread consumers[NUM_PRODUCERS];
    std::thread producerThread(consumer);

    for (int i = 0; i < NUM_PRODUCERS; ++i)
    {
        consumers[i] = std::thread(producer);
    }

    producerThread.join();
    for (int i = 0; i < NUM_PRODUCERS; ++i)
    {
        consumers[i].join();
    }

    return 0;
}
