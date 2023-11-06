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
std::queue<Order> ready;
sem_t ready_mutex;
std::queue<Order> delivered;
sem_t delivered_mutex;

sem_t client_sem;
sem_t ready_sem;
sem_t leave_sem;

int clients_amount, orders_taken = 0, orders_finnished_taken = 0;
int left_clients = 0;

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

        std::cout << "(C) Cliente " << tid << " aguardando.\n";

        sem_wait(&ready_sem);
        sem_wait(&delivered_mutex);
        Order delivered_order = delivered.front();
        delivered.pop();
        sem_post(&delivered_mutex);
        std::cout << "(C) Cliente recebeu o pedido " << delivered_order.id << "\n";
        sem_post(&client_sem);
        sem_wait(&leave_sem);
        left_clients++;
        std::cout << "(C) Cliente " << left_clients << " saiu\n";
        sem_post(&leave_sem);
    };
};

class Waiter
{
public:
    void takeOrder(int clients_amount)
    {
        while (1)
        {
            // Tem pratos prontos? Sim -> Verifico os pratos | Não -> Espero
            if (ready.size())
            {
                sem_wait(&ready_mutex); // Minha vez de verificar os pratos
                if (ready.size()) // Tem pratos prontos mesmo? -> Pego para entregar
                {
                    Order taken_order = ready.front();
                    ready.pop();
                    orders_finnished_taken++;
                    sem_post(&ready_mutex); // Já peguei minha entrega, libero a verificação
                    // Processamento não-crítico aqui
                    std::cout << "(G) Entregando pedido: " << taken_order.id << "\n";
                    sem_wait(&delivered_mutex);
                    Order delivered_order = delivered.front();
                    delivered.pop();
                    sem_post(&delivered_mutex);
                    std::cout << "(G) Pedido entregue: " << taken_order.id << "\n";
                    sem_post(&ready_sem);
                }
                else // Não tem pratos prontos. -> Libero a verificação e espero alguma mudança
                    sem_post(&ready_mutex);
            }
            // Todos os pratos foram entregues -> Para
            else if (orders_finnished_taken == clients_amount)
                return;
        }
    }
};

class Kitchen
{
public:
    void prepareOrder(int clients_amount)
    {
        while (1)
        {
            // Pedidos vazio? Sim -> Verifico os pedidos | Não -> Espero
            if (orders.size())
            {
                sem_wait(&orders_mutex); // Minha vez de verificar pedidos
                if (orders.size()) // Tem pedidos mesmo? -> Cozinho
                {
                    Order preparing = orders.front();
                    orders.pop();
                    orders_taken++;
                    sem_post(&orders_mutex); // Já peguei meu pedido, libero a verificação
                    std::cout << "(K) Em preparo: " << preparing.id << "\n";
                    // Processamento não-crítico aqui
                    sem_wait(&ready_mutex);
                    ready.push(preparing);
                    sem_post(&ready_mutex);
                    std::cout << "(K) Preparado: " << preparing.id << "\n";
                }
                else // Não tem pedidos. -> Libero a verificação e espero alguma mudança
                    sem_post(&orders_mutex);
            }
            // Todos pedidos dos clientes foram pegos? -> Para
            else if (orders_taken == clients_amount)
                return;
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
    int waiters_amount = 2 + (std::rand() % 4);
    int kitchens_amount = 1 + (std::rand() % 3);

    std::cout << "Número de clientes: " << clients_amount << '\n';
    std::cout << "Número de garçons: " << waiters_amount << '\n';

    sem_init(&client_sem, 0, clients_amount);
    sem_init(&ready_sem, 0, 0);
    sem_init(&leave_sem, 0, 1);

    sem_init(&orders_mutex, 0, 1);
    sem_init(&ready_mutex, 0, 1);
    sem_init(&delivered_mutex, 0, 1);

    std::thread clients[clients_amount];
    std::thread waiters[waiters_amount];
    std::thread kitchens[kitchens_amount];

    Client a;
    Waiter b;
    Kitchen c;

    for (int i = 0; i < waiters_amount; i++)
    {
        waiters[i] = std::thread(&Waiter::takeOrder, &b, clients_amount);
    }

    for (int i = 0; i < kitchens_amount; i++)
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

    for (int i = 0; i < kitchens_amount; i++)
    {
        kitchens[i].join();
    }
}