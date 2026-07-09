#include "UserManager.h"
#include "CSVReader.h"
#include "Input.h"
#include <iostream>
#include <fstream>
#include <functional>
#include <random>

const std::string UserManager::TEST_USERNAME = "0000000000";

UserManager::UserManager()
{
}

std::vector<std::vector<std::string>> UserManager::readAllUsers()
{
    std::vector<std::vector<std::string>> rows;
    std::ifstream file{registerFile};
    std::string line;
    while (std::getline(file, line))
    {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if (tokens.size() == 4) rows.push_back(tokens);
    }
    return rows;
}

void UserManager::writeAllUsers(const std::vector<std::vector<std::string>>& rows)
{
    std::ofstream file{registerFile, std::ios::trunc};
    for (const std::vector<std::string>& r : rows)
    {
        file << r[0] << "," << r[1] << "," << r[2] << "," << r[3] << std::endl;
    }
}

std::string UserManager::hashPassword(const std::string& password)
{
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

std::string UserManager::generateUsername(const std::vector<std::vector<std::string>>& rows)
{
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<long long> dist{1000000000LL, 9999999999LL};
    while (true)
    {
        std::string candidate = std::to_string(dist(rng));   // always 10 digits
        bool taken = false;
        for (const std::vector<std::string>& r : rows)
        {
            if (r[0] == candidate) { taken = true; break; }
        }
        if (!taken) return candidate;
    }
}

std::string UserManager::registerUser()
{
    std::string fullname = Input::readLine("Full name: ");
    std::string email    = Input::readLine("Email address: ");
    std::string password = Input::readLine("Password: ");

    if (fullname.empty() || email.empty() || password.empty())
    {
        std::cout << "All fields are required." << std::endl;
        return "";
    }
    // commas would corrupt the CSV row; '@' check keeps emails plausible
    if (fullname.find(',') != std::string::npos || email.find(',') != std::string::npos)
    {
        std::cout << "Fields must not contain commas." << std::endl;
        return "";
    }
    if (email.find('@') == std::string::npos)
    {
        std::cout << "Invalid email address." << std::endl;
        return "";
    }

    std::vector<std::vector<std::string>> rows = readAllUsers();
    for (const std::vector<std::string>& r : rows)
    {
        if (r[1] == fullname && r[2] == email)   // Task 2.5: duplicate prevention
        {
            std::cout << "User already registered." << std::endl;
            return "";
        }
    }

    std::string username = generateUsername(rows);

    std::ofstream file{registerFile, std::ios::app};
    file << username << "," << fullname << "," << email << ","
         << hashPassword(password) << std::endl;

    // Task 2.1.6: signup bonus is granted on first login,
    // in one shared code path for registered users and the testuser alike
    std::cout << "Registered! Your username is: " << username << std::endl;
    std::cout << "(Remember it for next login. Signup bonus on first login: 200 BTC, 200 ETH, 200 USDT)" << std::endl;
    return username;
}

std::string UserManager::loginOrRegister()
{
    std::string username = Input::readLine("Username (leave blank for testuser): ");
    if (username.empty())
    {
        std::cout << "\nContinuing as testuser " << TEST_USERNAME
                  << " (login bypassed for quick testing)." << std::endl;
        return TEST_USERNAME;
    }

    // unknown usernames skip straight to the registration prompt
    std::vector<std::vector<std::string>> rows = readAllUsers();
    const std::vector<std::string>* found = nullptr;
    for (const std::vector<std::string>& r : rows)
    {
        if (r[0] == username) { found = &r; break; }
    }

    if (found == nullptr)
    {
        std::cout << "Username not found." << std::endl;
        std::string answer = Input::readLine("Register as a new user? (y/n): ");
        if (answer == "y" || answer == "Y")
        {
            return registerUser();   // logs the new user straight in
        }
        return "";
    }

    std::string password = Input::readLine("Password: ");
    if ((*found)[3] == hashPassword(password))
    {
        std::cout << "Welcome, " << (*found)[1] << "!" << std::endl;
        return username;
    }
    std::cout << "Login failed." << std::endl;
    return "";
}

void UserManager::resetPassword()
{
    // identity check: all three details must match the stored record
    std::string username = Input::readLine("Username: ");
    std::string fullname = Input::readLine("Full name: ");
    std::string email    = Input::readLine("Email address: ");

    std::vector<std::vector<std::string>> rows = readAllUsers();
    for (std::vector<std::string>& r : rows)
    {
        if (r[0] == username && r[1] == fullname && r[2] == email)
        {
            std::string newPassword = Input::readLine("New password: ");
            if (newPassword.empty())
            {
                std::cout << "Password must not be empty." << std::endl;
                return;
            }
            r[3] = hashPassword(newPassword);
            writeAllUsers(rows);
            std::cout << "Password updated." << std::endl;
            return;
        }
    }
    std::cout << "Reset failed." << std::endl;
}
