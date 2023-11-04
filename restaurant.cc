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
std::queue<Order> finnished;
sem_t finnished_mutex;

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
        sem_wait(&orders_mutex); // acredito que todos os acessos simultâneos à listas podem ser controlados pelo mutex, tal qual o exemplo de semáforo de EM do professor. Por isso eu criei 3 mutex, um para cada fila (acho q o leave_sem também pode ser considerado um mutex)
        orders.push(new_order);  // 2 Push no mesmo instante ocasiona problemas?
        sem_post(&orders_mutex);
        sem_post(&waiter_sem);

        std::cout << "(C) Cliente " << tid << " aguardando.\n";

        sem_wait(&ready_sem);
        sem_wait(&ready_mutex);
        Order ready_order = ready.front();
        ready.pop();
        sem_post(&ready_mutex);
        std::cout << "(C) Cliente recebeu o pedido " << ready_order.id << "\n";
        sem_post(&client_sem);
        sem_wait(&leave_sem);
        clients_amount--;
        sem_post(&leave_sem); // Cliente sai um de cada vez, educados :3
        std::cout << "Cliente saiu\n";
    };
};

class Waiter
{
public:
    void takeOrder()
    {
        while (clients_amount) // issue: programa nunca encerra, a thread espera pelo semáforo antes que o cliente possa decrementar a variável de controle do laço, o mesmo acontece com a cozinha
        {
            sem_wait(&waiter_sem); // Waiters também podem fazerem operações ao mesmo tempo.
            // Mas não podem se o orders[] estiver vazio.

            sem_wait(&ready_mutex); // acredito que esse if seja capaz de fazer com que o garçom tenha a função tanto de levar quando trazer os pratos aos clientes (respeitando o acesso à lista) (eu fiz essa parte com um pouco de sono, devo admitir)
            if (ready.size())
            {
                Order taken_order = ready.front();
                ready.pop(); // 2 Pop no mesmo instante ocasiona problemas?
                sem_post(&ready_mutex);
                std::cout << "(G) Retirando pedido:" << taken_order.id << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1 + std::rand() % 1));
                std::cout << "(G) Pedido finalizado:" << taken_order.id << "\n";
                sem_wait(&finnished_mutex);
                finnished.push(taken_order);
                sem_post(&finnished_mutex);
                sem_post(&ready_sem);
            }
            else
            {
                sem_post(&ready_mutex);

                sem_wait(&orders_mutex);
                Order taken_order = orders.front();
                orders.pop(); // 2 Pop no mesmo instante ocasiona problemas?
                sem_post(&orders_mutex);
                std::cout << "(G) Entregando pedido:" << taken_order.id << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1 + std::rand() % 3));
                std::cout << "(G) Pedido entregue:" << taken_order.id << "\n";
                sem_wait(&kitchen_mutex);
                kitchen_queue.push(taken_order);
                sem_post(&kitchen_mutex);
                sem_post(&kitchen_sem);
            }
        }
    }
};

class Kitchen
{
public:
    void prepareOrder()
    {
        while (clients_amount)
        {
            sem_wait(&kitchen_sem);
            sem_wait(&kitchen_mutex);
            Order preparing = kitchen_queue.front();
            kitchen_queue.pop();
            sem_post(&kitchen_mutex);
            std::cout << "(K) Em preparo: " << preparing.id << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1 + std::rand() % 5));
            sem_wait(&ready_mutex);
            ready.push(preparing);
            sem_post(&ready_mutex);
            sem_post(&waiter_sem);
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

    sem_init(&orders_mutex, 0, 1);
    sem_init(&kitchen_mutex, 0, 1);
    sem_init(&ready_mutex, 0, 1);
    sem_init(&finnished_mutex, 0, 1);

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