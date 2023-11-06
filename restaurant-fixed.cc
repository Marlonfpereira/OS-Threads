#include <iostream>
#include <thread>
#include <queue>
#include <semaphore.h>
#include <mutex>


struct Order
{
    int id;

    Order(){};
    Order(int _id)
    {
        id = _id;
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

int clients_amount, orders_taken = 0, orders_finnished_taken = 0;
int left_clients = 0;
int kitchens_amount, waiters_amount;

int active_waiter = 0, active_kitchen = 0;
sem_t active_waiter_sem, active_kitchen_sem;

class Client
{
private:
public:
    void createOrder(int id)
    {
        Order new_order(id);

        sem_wait(&client_sem);

        sem_wait(&orders_mutex);
        orders.push(new_order);
        sem_post(&orders_mutex);


        sem_wait(&ready_sem);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sem_wait(&delivered_mutex);
        Order delivered_order = delivered.front();
        delivered.pop();
        left_clients++;
        sem_post(&delivered_mutex);
        
        sem_post(&client_sem);
    };
};

class Waiter
{
public:
    void takeOrder()
    {
        while (orders_finnished_taken < clients_amount)
        {
            sem_wait(&ready_mutex); // Minha vez de verificar os pratos
            if (ready.size())       // Tem pratos prontos? Sim -> Verifico os pratos | Não -> Verifico se já acabou
            {
                Order taken_order = ready.front();
                ready.pop();
                orders_finnished_taken++;
                sem_post(&ready_mutex); // Já peguei minha entrega, libero a verificação

                sem_wait(&active_waiter_sem);
                active_waiter++;
                sem_post(&active_waiter_sem);

                // Processamento não-crítico aqui
                std::this_thread::sleep_for(std::chrono::seconds(1));
                sem_wait(&delivered_mutex);
                delivered.push(taken_order);
                sem_post(&delivered_mutex);

                sem_post(&ready_sem);
                
                sem_wait(&active_waiter_sem);
                active_waiter--;
                sem_post(&active_waiter_sem);
            }
            else // Não tem pratos prontos. -> Libero a verificação e espero alguma mudança
                sem_post(&ready_mutex);
        }
    }
};



class Kitchen
{
public:
    void prepareOrder()
    {
        while (orders_taken < clients_amount)
        {
            sem_wait(&orders_mutex);
            if (orders.size()) // Tem pedidos mesmo? -> Cozinho
            {
                Order preparing = orders.front();
                orders.pop();
                orders_taken++;
                sem_post(&orders_mutex); // Já peguei meu pedido, libero a verificação

                sem_wait(&active_kitchen_sem);
                active_kitchen++;
                sem_post(&active_kitchen_sem);

                // Processamento não-crítico aqui
                std::this_thread::sleep_for(std::chrono::seconds(1));
                sem_wait(&ready_mutex);
                ready.push(preparing);
                sem_post(&ready_mutex);

                sem_wait(&active_kitchen_sem);
                active_kitchen--;
                sem_post(&active_kitchen_sem);
            }
            else // Não tem pedidos. -> Libero a verificação e espero alguma mudança
                sem_post(&orders_mutex);
        }
    }
};

class Manager 
{
    public:
    void seeWorking() {
        while(1) {
            std::cout << "\n Clientes  = ";
            for (int i = 0; i < clients_amount - left_clients; i++)
                std::cout << "\x1b[41m " << i << " \x1b[0m" << " ";

            std::cout << "\n Pedidos   = ";
            for (int i = 0; i < orders.size(); i++)
                std::cout << "\x1b[41m" <<" + " << "\x1b[0m" << " ";
            
            std::cout << "\n Cozinhas  = ";
            for (int i = 0; i < active_kitchen; i++)
                std::cout << "\x1b[43m " << i << " \x1b[0m" << " ";

            std::cout << "\n Prontos   = ";
            for (int i = 0; i < ready.size(); i++)
                std::cout << "\x1b[43m" <<" + " << "\x1b[0m" << " ";
            
            std::cout << "\n Garçons   = ";
            for (int i = 0; i < active_waiter; i++)
                std::cout << "\x1b[44m " << i << " \x1b[0m" << " ";
            
            std::cout << "\n Entregues = ";
            for (int i = 0; i < delivered.size(); i++)
                std::cout << "\x1b[44m" <<" + " << "\x1b[0m" << " ";
            
            if (left_clients == clients_amount)
            {
                std::cout << std::endl;
                return;
            }
            
            std::cout << "\n==============================\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            printf("\033[H\033[J");
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        clients_amount = std::atoi(argv[1]);
        kitchens_amount = std::atoi(argv[2]);
        waiters_amount = std::atoi(argv[3]);
    }
    else
    {
        // Threads Aleatórias
        std::srand(std::time(NULL));
        clients_amount = 1 + (std::rand() % 50);
        kitchens_amount = 1 + (std::rand() % 10);
        waiters_amount = 2 + (std::rand() % 10);
    }


    std::cout << "Número de clientes: " << clients_amount << '\n';
    std::cout << "Número de garçons: " << waiters_amount << '\n';
    std::cout << "Número de cozinhas: " << kitchens_amount << '\n';

    sem_init(&client_sem, 0, clients_amount); // (Mesas) Limita quantidade de clientes no restaurante
    sem_init(&ready_sem, 0, 0); // Prato na mesa

    sem_init(&orders_mutex, 0, 1);
    sem_init(&ready_mutex, 0, 1);
    sem_init(&delivered_mutex, 0, 1);
    
    sem_init(&active_waiter_sem, 0, 1);
    sem_init(&active_kitchen_sem, 0, 1);

    std::thread clients[clients_amount];
    std::thread waiters[waiters_amount];
    std::thread kitchens[kitchens_amount];
    std::thread manager;

    Client a;
    Waiter b;
    Kitchen c;
    Manager d;

    manager = std::thread(&Manager::seeWorking, &d);
    
    for (int i = 0; i < waiters_amount; i++)
        waiters[i] = std::thread(&Waiter::takeOrder, &b);

    for (int i = 0; i < kitchens_amount; i++)
        kitchens[i] = std::thread(&Kitchen::prepareOrder, &c);

    for (int i = 0; i < clients_amount; i++)
        clients[i] = std::thread(&Client::createOrder, &a, i);

    for (int i = 0; i < clients_amount; i++)
        clients[i].join();

    for (int i = 0; i < waiters_amount; i++)
        waiters[i].join();

    for (int i = 0; i < kitchens_amount; i++)
        kitchens[i].join();

    manager.join();
}