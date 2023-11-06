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

int clients_amount, kitchens_amount, waiters_amount, orders_taken = 0, orders_finnished_taken = 0;
int left_clients = 0;

int active_waiter = 0, active_kitchen = 0;

class Client
{
private:
public:
    void createOrder(int id)
    {
        Order new_order(id);
        orders.push(new_order);
    };

    void leave() {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        Order delivered_order = delivered.front();
        delivered.pop();
        left_clients++;
    }
};

class Waiter
{
public:
    void takeOrder()
    {
            if (ready.size()) // Tem pratos prontos? Sim -> Verifico os pratos | Não -> Verifico se já acabou
            {
                Order taken_order = ready.front();
                ready.pop();
                orders_finnished_taken++;

                active_waiter++;

                // Processamento não-crítico aqui
                std::this_thread::sleep_for(std::chrono::seconds(1));

                delivered.push(taken_order);
                active_waiter--;
            }
    }
};

class Kitchen
{
public:
    void prepareOrder()
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
};

class Manager
{
public:
    void seeWorking()
    {
        while (1)
        {
            std::cout << "\n Clients Iniciais: " << clients_amount;
            std::cout << "\n Cozinhas Iniciais: " << kitchens_amount;
            std::cout << "\n Garçons Iniciais: " << waiters_amount;
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

            std::cout << "\n==============================\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            printf("\033[H\033[J");
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc > 1)
        clients_amount = std::atoi(argv[1]);  // 1 + (std::rand() % 14);
    else
    {
        // Clientes Aleatórios
        std::srand(std::time(NULL));
        clients_amount = 1 + (std::rand() % 14);
    }

    std::cout << "Número de clientes: " << clients_amount << '\n';
    std::cout << "Número de garçons: " << waiters_amount << '\n';
    std::cout << "Número de cozinhas: " << kitchens_amount << '\n';

    std::thread manager;

    Client a;
    Kitchen b;
    Waiter c;
    Manager d;

    manager = std::thread(&Manager::seeWorking, &d);

    for (int i = 0; i < clients_amount; i++) {
        a.createOrder(i);
        b.prepareOrder();
        c.takeOrder();
        a.leave();
    }
    
    manager.join();
}