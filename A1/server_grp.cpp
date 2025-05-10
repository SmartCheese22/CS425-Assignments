/**
 * @file server_grp.cpp
 * @brief Chat server implementation with group chat functionality
 */

#include "server_grp.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define PORT "12345"            // Port we're listening on
#define FILENAME "users.txt"    // File to read user credentials from
constexpr int MAX_EVENTS = 100; // Maximum number of events to handle at once
constexpr int BUF_SIZE = 1024;  // Buffer size for client data
#define DEBUG 0                 // Debug flag

/**
 * handle Ctrl+C signal
 */
void sigint_handler(int signo) {
  std::cout << "\nShutting down server ..." << std::endl;
  exit(0);
}

/**
 * Strip leading and trailing spaces and newlines from a string
 */
void strip_input(std::string &str) {
  // Remove leading and trailing spaces and newlines
  str.erase(
      str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch) &&
               ch != '\n'; // Keep characters that are not space or newline
      }));

  str.erase(
      std::find_if(
          str.rbegin(), str.rend(),
          [](unsigned char ch) {
            return !std::isspace(ch) &&
                   ch != '\n'; // Keep characters that are not space or newline
          })
          .base(),
      str.end());
}

/**
 * Send message to client
 * @param client_fd: client file descriptor
 * @param message: message to send
 */
void send_server(int client_fd, std::string &message) {
  message = GREEN + message + RESET;
  send(client_fd, message.c_str(), message.size(), 0);
}

void send_server_error(int client_fd, std::string &message) {
  message = RED + "Error: " + message + RESET;
  send(client_fd, message.c_str(), message.size(), 0);
}

/**
 * Get IP address from sockaddr
 * @param sa: sockaddr
 */
void *get_in_addr(struct sockaddr *sa) {

  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/**
 * ChatServer constructor
 * Initialize the server
 */
void ChatServer::setup_listener() {
  struct addrinfo hints = {}, *ai, *p;
  hints.ai_family = AF_UNSPEC;     // Use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM; // TCP
  hints.ai_flags = AI_PASSIVE;     // use localhost

  int rv = getaddrinfo(nullptr, PORT, &hints, &ai);
  if (rv != 0) {
    throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(rv)));
  }

  for (p = ai; p != nullptr; p = p->ai_next) {
    listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener_fd == -1) {
      continue;
    }

    // Allow reusing the same port.
    int yes = 1;
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(listener_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(listener_fd);
      continue;
    }

    break;
  }

  freeaddrinfo(ai);

  if (p == nullptr) {
    throw std::runtime_error("Failed to bind listener socket");
  }

  if (listen(listener_fd, 10) == -1) {
    throw std::runtime_error("listen failed");
  }

  // Set listener_fd to non-blocking.
  int flags = fcntl(listener_fd, F_GETFL, 0);
  fcntl(listener_fd, F_SETFL, flags | O_NONBLOCK);

  std::cout << "Server is ready and waiting for connections on " << PORT
            << std::endl;

  // Create epoll instance.
  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    throw std::runtime_error("epoll_create1 failed");
  }

  struct epoll_event ev = {};
  ev.events = EPOLLIN; // Read events
  ev.data.fd = listener_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener_fd, &ev) == -1) {
    throw std::runtime_error("epoll_ctl: listener_fd failed");
  }
}

/**
 * Handle new connection
 * Accept new connection and add to epoll
 */

void ChatServer::handle_new_connection() {
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen = sizeof(remoteaddr);
  char remoteIP[INET6_ADDRSTRLEN];

  int new_fd = accept(listener_fd, (struct sockaddr *)&remoteaddr, &addrlen);
  if (new_fd == -1) {
    perror("accept");
    return;
  }

  // Print remote client info.
  std::cout << "New connection from "
            << inet_ntop(remoteaddr.ss_family,
                         get_in_addr((struct sockaddr *)&remoteaddr), remoteIP,
                         INET6_ADDRSTRLEN)
            << " on socket " << new_fd << std::endl;

  // Set new_fd to non-blocking.
  int flags = fcntl(new_fd, F_GETFL, 0);
  fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);


  ClientSession session;
  session.fd = new_fd;
  session.state = ClientState::WAITING_USERNAME;
  sessions[new_fd] = session;

  std::string prompt = "Enter the username:\n";
  send(new_fd, prompt.c_str(), prompt.size(), 0);

  // Add to epoll.
  struct epoll_event ev = {};
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = new_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1) {
    perror("epoll_ctl: add new_fd");
    close(new_fd);
    sessions.erase(new_fd);
  }
}

/**
 * Handle client message
 * @param client_fd: client file descriptor
 * Read message from client and process it
 */
void ChatServer::handle_client_message(int client_fd) {
  char buf[BUF_SIZE];
  ssize_t nbytes;

  if (clients.find(client_fd) == clients.end() &&
      sessions.find(client_fd) == sessions.end()) {
    std::cerr << "Invalid client_fd: " << client_fd << std::endl;
    return;
  }

  if ((nbytes = recv(client_fd, buf, sizeof(buf), 0)) > 0) {
    buf[nbytes] = '\0';
    std::string data(buf);

    // If we have a session waiting for authentication, use that buffer.
    if (sessions.find(client_fd) != sessions.end()) {
      ClientSession &session = sessions[client_fd];

      // Directly process the received data as a complete message.
      std::string line(data);
      strip_input(line);

      if (session.state == ClientState::WAITING_USERNAME) {
        session.usernameCandidate = line;
        std::string prompt = "Enter the password:\n";
        send(client_fd, prompt.c_str(), prompt.size(), 0);
        session.state = ClientState::WAITING_PASSWORD;

      } else if (session.state == ClientState::WAITING_PASSWORD) {
        std::string password = line;
        int auth_result = perform_authentication(session.usernameCandidate,
                                                 password, client_fd);
        if (auth_result == SUCCESS) {
          session.state = ClientState::AUTHENTICATED;

          clients.insert(client_fd);
          fdTousername[client_fd] = session.usernameCandidate;
          usernameTofd[session.usernameCandidate] = client_fd;
          activeUsernames.insert(session.usernameCandidate);

          std::string welcome = GREEN + "Welcome to the chat server!\n" + RESET;
          send(client_fd, welcome.c_str(), welcome.size(), 0);

          std::string joinMsg =
              session.usernameCandidate + " has joined the chat\n";
          broadcast_message(joinMsg.c_str(), joinMsg.size(), client_fd, true);

          sessions.erase(client_fd);
        } else {
          std::string failMsg = "Authentication failed\n";
          send(client_fd, failMsg.c_str(), failMsg.size(), 0);
          sessions.erase(client_fd);
          close(client_fd);
          return;
        }
      }
    }

    // If the client is already authenticated, process commands.
    else if (clients.find(client_fd) != clients.end()) {
      std::string message(data);
      strip_input(message);
      process_authenticated_message(client_fd, message);
    }
  }

  if (nbytes == 0) {
    std::cout << "Socket " << client_fd << " hung up" << std::endl;
  } else if (nbytes < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
    if (errno == EBADF) {
      return;
    }
    perror("recv");
  }

  // Clean up if the connection was closed.
  if (nbytes == 0 || (nbytes < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) {
    if (clients.find(client_fd) != clients.end()) {
      // Inform others that the user left.
      std::string leftMsg = fdTousername[client_fd] + " has left the chat\n";
      broadcast_message(leftMsg.c_str(), leftMsg.size(), client_fd, true);

      activeUsernames.erase(fdTousername[client_fd]);
      usernameTofd.erase(fdTousername[client_fd]);
      fdTousername.erase(client_fd);
      clients.erase(client_fd);
    }
    sessions.erase(client_fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
  }
}

/**
 * Perform authentication
 * @param username: username
 * @param password: password
 * @param client_fd: client file descriptor
 * @return: SUCCESS or FAIL
 * Check if the username and password are correct
 */
int ChatServer::perform_authentication(const std::string &username,
                                       const std::string &password,
                                       int client_fd) {
  if (activeUsernames.find(username) != activeUsernames.end()) {
    std::string msg = "User already logged in\n";
    send_server(client_fd, msg);
    return FAIL;
  }

  std::ifstream userfile(FILENAME);
  if (!userfile.is_open()) {
    std::cerr << "error opening " << FILENAME << std::endl;
    return FAIL;
  }

  std::string line;
  while (std::getline(userfile, line)) {
    size_t colon_pos = line.find(":");
    if (colon_pos != std::string::npos) {
      std::string stored_username = line.substr(0, colon_pos);
      std::string stored_password = line.substr(colon_pos + 1);
      strip_input(stored_username);
      strip_input(stored_password);

      if (DEBUG)
        std::cout << "Checked: " << stored_username << " " << stored_password
                  << std::endl;

      if (stored_username == username && stored_password == password) {
        return SUCCESS;
      }
    }
  }
  return FAIL;
}

/**
 * Process authenticated message
 * @param client_fd: client file descriptor
 * @param message: message to process
 * Process the message from the authenticated user
 * and perform the corresponding action
 */
void ChatServer::process_authenticated_message(int client_fd,
                                               const std::string &message) {
  std::stringstream ss(message);
  std::string command;
  ss >> command;

  std::string server_message;
  if (command == "/msg") {
    std::string receiver;
    ss >> receiver;
    std::string msg;
    std::getline(ss, msg);
    strip_input(receiver);
    strip_input(msg);
    msg.push_back('\n');
    if (usernameTofd.find(receiver) == usernameTofd.end()) {
      server_message = "User not found\n";
      send_server_error(client_fd, server_message);
    } else if (receiver == fdTousername[client_fd]) {
      server_message = "Cannot send message to self\n";
      send_server_error(client_fd, server_message);
    } else if (receiver.empty()) {
      server_message = "Please specify a username\n";
      send_server_error(client_fd, server_message);
    } else {
      int receiver_fd = usernameTofd[receiver];
      std::string s_message = "[ " + fdTousername[client_fd] + " ] : " + msg;
      send(receiver_fd, s_message.c_str(), s_message.size(), 0);
    }
  } else if (command == "/broadcast") {
    std::string msg;
    std::getline(ss, msg);
    strip_input(msg);
    msg.push_back('\n');
    broadcast_message(msg.c_str(), msg.size(), client_fd, false);
  } else if (command == "/group_msg") {
    std::string group;
    ss >> group;
    std::string msg;
    std::getline(ss, msg);
    strip_input(group);
    strip_input(msg);
    msg.push_back('\n');
    if (groupTofd.find(group) == groupTofd.end()) {
      server_message = "Group not found\n";
      send_server_error(client_fd, server_message);
    } else if (group.empty()) {
      server_message = "Please specify a group name\n";
      send_server_error(client_fd, server_message);
    } else {
      for (int receiver_fd : groupTofd[group]) {
        if (receiver_fd == client_fd)
          continue;
        std::string s_message =
            LIGHT_CYAN + "[ Group " + group + " ]" + RESET + " : " + msg;
        send(receiver_fd, s_message.c_str(), s_message.size(), 0);
      }
    }
  } else if (command == "/create_group") {
    std::string group;
    ss >> group;
    strip_input(group);
    if (groupTofd.find(group) != groupTofd.end()) {
      server_message = "Group already exists\n";
      send_server_error(client_fd, server_message);
    } else if (group.empty()) {
      server_message = "Please specify a group name\n";
      send_server_error(client_fd, server_message);
    } else {
      groupTofd[group] = {client_fd};
      std::string create_msg = "Group " + group + " created\n";
      send(client_fd, create_msg.c_str(), create_msg.size(), 0);
    }
  } else if (command == "/join_group") {
    std::string group;
    ss >> group;
    strip_input(group);
    if (groupTofd.find(group) == groupTofd.end()) {
      server_message = "Group not found\n";
      send_server_error(client_fd, server_message);
    } else if (group.empty()) {
      server_message = "Please specify a group name\n";
      send_server_error(client_fd, server_message);
    } else {
      if (groupTofd[group].find(client_fd) != groupTofd[group].end()) {
        server_message = "Already a member\n";
        send_server(client_fd, server_message);
      } else {
        groupTofd[group].insert(client_fd);
        std::string join_msg =
            GREEN + "You joined the group " + group + ".\n" + RESET;
        send(client_fd, join_msg.c_str(), join_msg.size(), 0);
      }
    }
  } else if (command == "/leave_group") {
    std::string group;
    ss >> group;
    if (group.empty()) {
      std::string error_msg =
          RED + "Error: Please specify a group to leave. " + RESET;
      send(client_fd, error_msg.c_str(), error_msg.size(), 0);
      return;
    }
    strip_input(group);
    if (groupTofd.find(group) == groupTofd.end()) {
      server_message = "Group not found\n";
      send_server_error(client_fd, server_message);
    } else {
      if (groupTofd[group].find(client_fd) != groupTofd[group].end()) {
        groupTofd[group].erase(client_fd);
        std::string leave_msg =
            GREEN + "You left the group " + group + ".\n" + RESET;
        send(client_fd, leave_msg.c_str(), leave_msg.size(), 0);
      } else {
        server_message = "Not a member of the group\n";
        send_server_error(client_fd, server_message);
      }
    }
  } else if (command == "CLOSE") {
    std::cout << "Connection closed on socket " << client_fd << std::endl;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    activeUsernames.erase(fdTousername[client_fd]);
    usernameTofd.erase(fdTousername[client_fd]);
    clients.erase(client_fd);
    broadcast_message(
        (fdTousername[client_fd] + " has left the chat\n").c_str(),
        fdTousername[client_fd].size() + 20, client_fd, true);
    fdTousername.erase(client_fd);
    close(client_fd);
  } else {
    send_server(client_fd, help_message);
  }
}

/**
 * Broadcast message
 * @param message: message to broadcast
 * @param length: length of message
 * @param sender_fd: sender file descriptor
 * @param server_broadcast: broadcast from server
 */
void ChatServer::broadcast_message(const char *message, size_t length,
                                   int sender_fd, bool server_broadcast) {

  for (int client_fd : clients) {
    if (client_fd != sender_fd && client_fd != listener_fd) {
      std::string s_message(message);
      if (server_broadcast)
        s_message = GREEN + s_message + RESET;
      else
        s_message = BLUE + fdTousername[sender_fd] + RESET + ": " + GREEN +
                    s_message + RESET;
      send(client_fd, s_message.c_str(), s_message.size(), 0);
    }
  }
}

void ChatServer::run() {
  setup_listener();

  std::vector<struct epoll_event> events(MAX_EVENTS);

  while (true) {
    int num_events = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
    if (num_events == -1) {
      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < num_events; ++i) {
      if (events[i].data.fd == listener_fd) {
        handle_new_connection();
      } else {
        handle_client_message(events[i].data.fd);
      }
    }
  }
}

int main() {
  signal(SIGINT, sigint_handler); // Handle Ctrl+C gracefully
  try {
    ChatServer server;
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
