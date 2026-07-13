// START MY CODE
#pragma once
#include <string>
#include <vector>

/** Task 2: owns USERS_REGISTER.CSV — combined login/register, password reset.
 *  Row format: username,fullname,email,passwordHash */
class UserManager
{
    public:
        /** reserved testuser account (blank login skips credentials) */
        static const std::string TEST_USERNAME;

        UserManager();
        /** single entry point: blank username -> testuser TEST_USERNAME;
         *  failed login offers registration; returns username, "" on failure */
        std::string loginOrRegister();
        void resetPassword();

    private:
        /** returns the newly generated username, "" on failure */
        std::string registerUser();

        std::vector<std::vector<std::string>> readAllUsers();
        void writeAllUsers(const std::vector<std::vector<std::string>>& rows);
        std::string generateUsername(const std::vector<std::vector<std::string>>& rows);
        static std::string hashPassword(const std::string& password);

        const std::string registerFile = "USERS_REGISTER.CSV";
};
// END MY CODE
