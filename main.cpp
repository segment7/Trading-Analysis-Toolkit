#include <iostream>
#include "OrderBook.h"
#include "UserManager.h"
#include "MerkelMain.h"
#include "Input.h"

int main()
// START MY CODE
{
    // load market data ONCE; every login session shares it by reference
    OrderBook orderBook{"20200601.csv"};
    UserManager userManager;

    while (true)
    {
        std::cout << "\n=========================" << std::endl;
        std::cout << "=== MerkelRex Trading ===" << std::endl;
        std::cout << "=========================" << std::endl;
        std::cout << "\n1: Login / Register" << std::endl;
        std::cout << "2: Reset password" << std::endl;
        std::cout << "0: Exit\n" << std::endl;
        int choice = Input::promptChoice(0, 2);

        if (choice == 0)
        {
            std::cout << "Goodbye." << std::endl;
            break;
        }
        if (choice == 1)
        {
            std::string username = userManager.loginOrRegister();
            if (username != "")
            {
                // fresh session per login — per-user state, destroyed on logout
                MerkelMain session{username, orderBook};
                session.run();
            }
        }
        if (choice == 2) userManager.resetPassword();
    }
    return 0;
// END MY CODE
}
