#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <semaphore.h>
#include <mutex>

struct Order
{
    int id;

    Order(){};
    Order(std::thread::id _id)
    {
        // Por que?
        // Aparentemente thread ids são magia negra e não podem ser int, char, string nem nada
        // hash funciona ._.
        // Solução (passar id única como parâmetro pra thread do cliente)
        size_t e = std::hash<std::thread::id>()(_id);
        id = e;
    }
};

std::queue<Order> orders;
sem_t orders_mutex;
std::queue<Order> kitchen_queue;
sem_t kitchen_mutex;
std::queue<Order> ready;
sem_t ready_mutex;
std::queue<Order> delivered;
sem_t delivered_mutex;

sem_t client_sem;
sem_t waiter_sem;
sem_t kitchen_sem;
sem_t ready_sem;
sem_t leave_sem;

int clients_amount, orders_taken = 0, orders_finnished_taken = 0;

class Client
{
private:
public:
    void createOrder()
    {

        auto tid = std::this_thread::get_id();
        Order new_order(tid);

        sem_wait(&client_sem);
        sem_wait(&orders_mutex);
        orders.push(new_order);
        sem_post(&orders_mutex);
        sem_post(&kitchen_sem);

        std::cout << "(C) Cliente " << tid << " aguardando.\n";

        sem_wait(&ready_sem);
        sem_wait(&ready_mutex);
        Order delivered_order = delivered.front();
        delivered.pop();
        sem_post(&ready_mutex);
        std::cout << "(C) Cliente recebeu o pedido " << delivered_order.id << "\n";
        sem_post(&client_sem);
        std::cout << "(C) Cliente saiu\n";
    };
};

class Waiter
{
public:
    void takeOrder(int clients_amount)
    {
        while (orders_finnished_taken != clients_amount)
        {
            sem_wait(&waiter_sem);
            if (orders_finnished_taken != clients_amount)
            {
                sem_wait(&ready_mutex);
                Order taken_order = ready.front();
                ready.pop();
                orders_finnished_taken++;
                sem_post(&waiter_sem);
                sem_post(&ready_mutex);
                std::cout << "(G) Entregando pedido: " << taken_order.id << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1 + std::rand() % 3));
                sem_wait(&delivered_mutex);
                Order delivered_order = delivered.front();
                delivered.pop();
                std::cout << "(G) Pedido entregue: " << taken_order.id << "\n";
                sem_post(&delivered_mutex);
                sem_post(&ready_sem);
            }
        }
    }
};

class Kitchen
{
public:
    void prepareOrder(int clients_amount)
    {
        while (orders_taken != clients_amount)
        {
            sem_wait(&kitchen_sem);
            if (orders_taken != clients_amount)
            {
                sem_wait(&kitchen_mutex);
                Order preparing = orders.front();
                orders.pop();
                orders_taken++;
                sem_post(&kitchen_sem);
                sem_post(&kitchen_mutex);
                std::cout << "(K) Em preparo: " << preparing.id << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1 + std::rand() % 3));
                sem_wait(&ready_mutex);
                ready.push(preparing);
                sem_post(&ready_mutex);
                sem_post(&waiter_sem);
            }
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
    std::srand(std::time(NULL));
    // Clientes Aleatórios
    clients_amount = 1 + (std::rand() % 14);
    int waiters_amount = 1 + (std::rand() % 2);

    std::cout << "Número de clientes: " << clients_amount << '\n';
    std::cout << "Número de garçons: " << waiters_amount << '\n';

    sem_init(&client_sem, 0, clients_amount);
    sem_init(&waiter_sem, 0, 0);
    sem_init(&kitchen_sem, 0, 0);
    sem_init(&ready_sem, 0, 0);
    sem_init(&leave_sem, 0, 1);

    sem_init(&orders_mutex, 0, 1);
    sem_init(&kitchen_mutex, 0, 1);
    sem_init(&ready_mutex, 0, 1);
    sem_init(&delivered_mutex, 0, 1);

    std::thread clients[clients_amount];
    std::thread waiters[waiters_amount];
    std::thread kitchens[1];

    Client a;
    Waiter b;
    Kitchen c;

    for (int i = 0; i < waiters_amount; i++)
    {
        waiters[i] = std::thread(&Waiter::takeOrder, &b, clients_amount);
    }

    for (int i = 0; i < 1; i++)
    {
        kitchens[i] = std::thread(&Kitchen::prepareOrder, &c, clients_amount);
    }

    for (int i = 0; i < clients_amount; i++)
    {
        clients[i] = std::thread(&Client::createOrder, &a);
    }

    for (int i = 0; i < clients_amount; i++)
    {
        clients[i].join();
    }

    for (int i = 0; i < waiters_amount; i++)
    {
        waiters[i].join();
    }

    for (int i = 0; i < 1; i++)
    {
        kitchens[i].join();
    }
}