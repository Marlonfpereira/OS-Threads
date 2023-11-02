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
std::queue<Order> kitchen_queue;
std::queue<Order> ready;

sem_t client_sem;
sem_t waiter_sem;
sem_t kitchen_sem;
sem_t ready_sem;
sem_t leave_sem;

int clients_amount;

class Client
{
private:

public:
    void createOrder()
    {
        auto tid = std::this_thread::get_id();
        Order new_order(tid); // Novo Pedido
        // Seção Crítica
        // Limita um número N de clientes. Mas não evita manipulação conjunta dos clientes
        sem_wait(&client_sem); 
        orders.push(new_order); // 2 Push no mesmo instante ocasiona problemas?
        sem_post(&waiter_sem);

        std::cout << "(C) Cliente " << tid << " aguardando.\n";

        sem_wait(&ready_sem);
        Order ready_order = ready.front();
        ready.pop();
        std::cout << "(C) Cliente recebeu o pedido " << ready_order.id << "\n";
        sem_post(&client_sem);
        sem_wait(&leave_sem);
        clients_amount--;
        sem_post(&leave_sem); // Cliente sai um de cada vez, educados :3
    };
};

class Waiter
{
    public:
    void takeOrder() 
    {
        while(clients_amount) 
        {
            sem_wait(&waiter_sem); // Waiters também podem fazerem operações ao mesmo tempo.
            // Mas não podem se o orders[] estiver vazio.
            Order taken_order = orders.front(); 
            orders.pop(); // 2 Pop no mesmo instante ocasiona problemas?
            std::cout << "(G) Entregando pedido:" << taken_order.id << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1 + std::rand() % 3));
            std::cout << "(G) Pedido entregue:" << taken_order.id << "\n";
            kitchen_queue.push(taken_order);
            sem_post(&kitchen_sem);
        }
    }
};

class Kitchen
{
    public:
    void prepareOrder() 
    {
        while(clients_amount)
        {
            sem_wait(&kitchen_sem);
            Order preparing = kitchen_queue.front();
            kitchen_queue.pop();
            std::cout << "(K) Em preparo: " << preparing.id << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1 + std::rand() % 9));
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
    std::srand(std::time(NULL));
    // Clientes Aleatórios
    clients_amount = std::rand() % 15;
    int waiters_amount = 1 + (std::rand() % 2);

    std::cout << "Número de clientes: " << clients_amount << '\n';
    std::cout << "Número de garçons: " << waiters_amount << '\n';

    sem_init(&client_sem, 0, clients_amount);
    sem_init(&waiter_sem, 0, 0);
    sem_init(&kitchen_sem, 0, 0);
    sem_init(&ready_sem, 0, 0);
    sem_init(&leave_sem, 0, 1);

    std::thread clients[clients_amount];
    std::thread waiters[waiters_amount];
    std::thread kitchens[1];

    Client a;
    Waiter b;
    Kitchen c;

    for (int i = 0; i < waiters_amount; i++)
    {
        waiters[i] = std::thread(&Waiter::takeOrder, &b);
    }

    for (int i = 0; i < 1; i++)
    {
        kitchens[i] = std::thread(&Kitchen::prepareOrder, &c);
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