#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <unordered_set>
#include <unordered_map>

enum
{
    FAIL,
    SUCCESS
};

enum class ClientState { WAITING_USERNAME, WAITING_PASSWORD, AUTHENTICATED };

const std::string BLUE = "\033[34m";        // Light Blue for usernames
const std::string GREEN = "\033[32m";       // Light Blue for usernames
const std::string LIGHT_GREEN = "\033[92m"; // Light Green for messages
const std::string LIGHT_CYAN = "\033[96m";  // Light Cyan for server messages
const std::string RED = "\033[31m";         // Red for error messages
const std::string RESET = "\033[0m";

std::string help_message = "\n" +
                                 LIGHT_CYAN + "Available commands" + RESET + ":\n" +
                                 LIGHT_GREEN + "/msg <username> <message>" + RESET + " : Send a message to a user\n" +
                                 LIGHT_GREEN + "/broadcast <message>" + RESET + " : Send a message to all users\n" +
                                 LIGHT_GREEN + "/create_group <groupname>" + RESET + " : Create a group\n" +
                                 LIGHT_GREEN + "/join_group <groupname>" + RESET + " : Join a group\n" +
                                 LIGHT_GREEN + "/leave_group <groupname>" + RESET + " : Leave a group\n" +
                                 LIGHT_GREEN + "/group_msg <groupname> <message>" + RESET + " : Send a message to a group\n" +
                                 LIGHT_GREEN + "CLOSE" + RESET + " : Close the connection\n";

struct ClientSession {
    int fd;                         // file descriptor of the client
    ClientState state;              // state of the client
    std::string usernameCandidate;  // store the username entered
};


class ChatServer
{
public:
    ChatServer() : listener_fd(-1), epoll_fd(-1) {}

    void run();

private:
    int listener_fd;
    int epoll_fd;
    std::unordered_set<int> clients;
    std::unordered_set<std::string> activeUsernames;
    std::unordered_map<std::string, int> usernameTofd;                  //? username -> clientfd
    std::unordered_map<int, std::string> fdTousername;                  //? clientfd -> username
    std::unordered_map<std::string, std::unordered_set<int>> groupTofd; //? groupname -> set of clientfds
    std::unordered_map<int, ClientSession> sessions;
    void process_authenticated_message(int client_fd, const std::string &message);

    int perform_authentication(const std::string &username, const std::string &password, int client_fd);
    void setup_listener();
    void handle_new_connection();
    void handle_client_message(int client_fd);
    void broadcast_message(const char *message, size_t length, int sender_fd, bool server_broadcast = true);
};

#endif
