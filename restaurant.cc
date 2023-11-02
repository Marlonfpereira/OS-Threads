#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <semaphore.h>
#include <mutex>

#define ORDERS_SIZE 10

struct Order
{
    std::thread::id id;
    
    Order(){};
    Order(std::thread::id _id)
    {
        id = _id;
    }
};

std::queue<Order> orders;
std::queue<Order> kitchen_queue;
std::queue<Order> ready;

sem_t client_sem;
sem_t waiter_sem;
sem_t kitchen_sem;
sem_t ready_sem;

class Client
{
private:

public:
    void createOrder()
    {
        // Manda um novo pedido (coloca no orders[])
        Order newOrder(std::this_thread::get_id());
        // Seção Crítica
        sem_wait(&client_sem);
        orders.push(newOrder);
        sem_post(&waiter_sem);

        std::cout << "estou esperando\n";
        sem_wait(&ready_sem);

        Order readyOrder = ready.front();
        ready.pop();
        std::cout << "estou comendo:" << readyOrder.id << "\n";

        sem_post(&client_sem);

        // std::cout << "ID Pedido: " << newOrder.id << "\n";
        // post quando recebe o pedido
        // Fim Seção Crítica
    };
};

class Waiter
{
    public:
    void takeOrder() 
    {
        while(1) 
        {
            sem_wait(&waiter_sem);
            Order takenOrder = orders.front();
            orders.pop();
            std::cout << "peguei o pedido:" << takenOrder.id << "\n";
            kitchen_queue.push(takenOrder);
            sem_post(&kitchen_sem);
        }
    }
};

class Kitchen
{
    public:
    void prepareOrder() 
    {
        while(1)
        {
            sem_wait(&kitchen_sem);
            Order preparing = kitchen_queue.front();
            kitchen_queue.pop();
            std::cout << "preparando:" << preparing.id << "\n";
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
            ready.push(preparing);
            sem_post(&ready_sem);
        }
    }
};

/* Passos */
// Cliente faz um pedido
// Pedido vai pra um array de pedidos
// Garçons pegam um pedido no array
// Levam pra array entrada cozinha
// Cozinha faz o prato
// Garçom pega o prato do buffer da cozinha
// Devolve pro cliente

int main()
{
    sem_init(&client_sem, 0, ORDERS_SIZE);
    sem_init(&waiter_sem, 0, 0);
    sem_init(&kitchen_sem, 0, 0);
    sem_init(&ready_sem, 0, 0);

    std::thread clients[ORDERS_SIZE];
    std::thread waiters[2];
    std::thread kitchens[1];

    Client a;
    Waiter b;
    Kitchen c;

    for (int i = 0; i < ORDERS_SIZE; i++)
    {
        clients[i] = std::thread(&Client::createOrder, &a);
    }

    for (int i = 0; i < 2; i++)
    {
        waiters[i] = std::thread(&Waiter::takeOrder, &b);
    }

    for (int i = 0; i < 1; i++)
    {
        kitchens[i] = std::thread(&Kitchen::prepareOrder, &c);
    }

    std::cout << orders.size() << '\n';

    for (int i = 0; i < ORDERS_SIZE; i++)
    {
        clients[i].join();
    }

    for (int i = 0; i < 2; i++)
    {
        waiters[i].join();
    }

    for (int i = 0; i < 1; i++)
    {
        kitchens[i].join();
    }
}