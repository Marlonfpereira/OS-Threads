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
    Order(int _id)
    {
        id = _id;
    }
};

std::queue<Order> orders;
std::queue<Order> ready;
std::queue<Order> delivered;

int clients_amount, orders_taken = 0, orders_finnished_taken = 0;
int left_clients = 0;
int kitchens_amount, waiters_amount;

int active_waiter = 0, active_kitchen = 0;

class Client
{
private:
public:
    void createOrder(int id)
    {
        Order new_order(id);

        orders.push(new_order);

        while (delivered.size() == 0);
        std::this_thread::sleep_for(std::chrono::seconds(3));

        Order delivered_order = delivered.front();
        // delivered.pop();
        left_clients++;
    };
};

class Waiter
{
public:
    void takeOrder()
    {
        while (orders_finnished_taken < clients_amount)
        {

            if (ready.size()) // Tem pratos prontos? Sim -> Verifico os pratos | Não -> Verifico se já acabou
            {
                Order taken_order = ready.front();
                ready.pop();
                orders_finnished_taken++;

                active_waiter++;

                // Processamento não-crítico aqui
                std::this_thread::sleep_for(std::chrono::seconds(10));

                delivered.push(taken_order);
                active_waiter--;
            }
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

            if (orders.size()) // Tem pedidos mesmo? -> Cozinho
            {
                Order preparing = orders.front();
                orders.pop();
                orders_taken++;

                active_kitchen++;

                // Processamento não-crítico aqui
                std::this_thread::sleep_for(std::chrono::seconds(1));

                ready.push(preparing);

                active_kitchen--;
            }
        }
    }
};

class Manager
{
public:
    void seeWorking()
    {
        while (1)
        {
            std::cout << "\n Clients Iniciais: " << clients_amount;
            std::cout << "\n Clientes  = ";
            for (int i = 0; i < clients_amount - left_clients; i++)
                std::cout << "\x1b[41m " << i << " \x1b[0m" << " ";

            std::cout << "\n Pedidos   = ";
            for (int i = 0; i < orders.size(); i++)
                std::cout << "\x1b[41m" << " + " << "\x1b[0m" << " ";

            std::cout << "\n Cozinhas  = ";
            for (int i = 0; i < active_kitchen; i++)
                std::cout << "\x1b[43m " << i << " \x1b[0m" << " ";

            std::cout << "\n Prontos   = ";
            for (int i = 0; i < ready.size(); i++)
                std::cout << "\x1b[43m" << " + " << "\x1b[0m" << " ";

            std::cout << "\n Garçons   = ";
            for (int i = 0; i < active_waiter; i++)
                std::cout << "\x1b[44m " << i << " \x1b[0m" << " ";

            std::cout << "\n Entregues = ";
            for (int i = 0; i < delivered.size(); i++)
                std::cout << "\x1b[44m" << " + " << "\x1b[0m" << " ";

            if (left_clients == clients_amount)
            {
                std::cout << std::endl;
                return;
            }
            if (active_kitchen < 0 || active_waiter < 0 || left_clients > clients_amount)
            {
                std::cout << "Active Kitchens: " << active_kitchen << "\n";
                std::cout << "Active Waiters: " << active_waiter << "\n";
                std::cout << "Active Clients: " << left_clients << "\n";
                std::exit(1);
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
        clients_amount = std::atoi(argv[1]);  // 1 + (std::rand() % 14);
        kitchens_amount = std::atoi(argv[2]); // 1 + (std::rand() % 3);
        waiters_amount = std::atoi(argv[3]);  // 2 + (std::rand() % 4);
    }
    else
    {
        // Threads Aleatórias
        std::srand(std::time(NULL));
        clients_amount = 10; // 1 + (std::rand() % 14);
        kitchens_amount = 2; // 1 + (std::rand() % 3);
        waiters_amount = 5;  // 2 + (std::rand() % 4);
    }

    std::cout << "Número de clientes: " << clients_amount << '\n';
    std::cout << "Número de garçons: " << waiters_amount << '\n';
    std::cout << "Número de cozinhas: " << kitchens_amount << '\n';

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
    {
        waiters[i] = std::thread(&Waiter::takeOrder, &b);
    }

    for (int i = 0; i < kitchens_amount; i++)
    {
        kitchens[i] = std::thread(&Kitchen::prepareOrder, &c);
    }

    for (int i = 0; i < clients_amount; i++)
    {
        clients[i] = std::thread(&Client::createOrder, &a, i);
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

    manager.join();
}