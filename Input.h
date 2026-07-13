// START MY CODE
#pragma once
#include <iostream>
#include <string>

/** Shared input helpers — the one place all menu/user input is read and validated (Task 5) */
namespace Input
{
    /** print prompt, read a whole line; returns "" on EOF */
    inline std::string readLine(const std::string& prompt)
    {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) return "";   // EOF
        return line;
    }

    /** menu choice in [min,max]; re-prompts until valid; EOF returns 0 = exit/back */
    inline int promptChoice(int min, int max)
    {
        while (true)
        {
            std::cout << "Type in " << min << "-" << max << ": ";
            std::string line;
            if (!std::getline(std::cin, line)) return 0;  // EOF exits cleanly
            try
            {
                int choice = std::stoi(line);
                if (choice >= min && choice <= max) return choice;
            }
            catch (const std::exception& e) { }
            std::cout << "Invalid choice." << std::endl;
        }
    }
}
// END MY CODE
