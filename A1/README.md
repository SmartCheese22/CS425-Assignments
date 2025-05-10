# README: Multi-Client Chat Server

## How to Run
1. Navigate to the directory where the source files are located.
2. Compile the server using:
   ```bash
   make
   ```
3. Start the server:
   ```bash
   ./server_grp
   ```
You can also connect to the server using (PORT = 12345):
   ```bash
   telnet localhost PORT
   ```
5. Follow the prompts to log in using credentials from `users.txt`.
6. Use the available commands to interact with other users.

---

## Features
### Implemented Features
- Multi-client chat server using epoll for efficient event-driven handling.
- Basic authentication via `users.txt`.
- Private messaging between users.
- Broadcast messaging.
- Group chat functionality:
  - Creating groups.
  - Joining and leaving groups.
  - Sending messages to a group.
- Graceful handling of client disconnections.
- Server shutdown handling with `SIGINT`.
- Non-blocking I/O to handle multiple clients efficiently.
- Prevent Duplicate logins, Groups
- Basic Error Handling

### Not Implemented Features
- Getting information about active Users, active Groups, members of a particular group .etc
- Encrypted communication.
- Cannot dynamically add and remove users from the database.
---

## Design Decisions
### Choice of epoll over Multithreading
Instead of creating a new thread for each connection, we chose an **event-driven approach using epoll** because:
- **Scalability**: Threads introduce significant overhead. Epoll allows handling thousands of connections efficiently in a single thread.
- **Performance**: Unlike blocking calls in multi-threaded models, epoll uses edge-triggered (EPOLLET) notifications, minimizing CPU wake-ups. Epoll is further optimised for linux systems.
- **Simplicity**: Managing concurrency with threads often requires synchronization (mutexes, condition variables), increasing complexity. With epoll, no explicit locking is required.

### Non-blocking I/O
- All sockets (listener and client connections) are set to **non-blocking mode** using `fcntl()`.
- This prevents the server from getting stuck waiting for data and allows immediate processing of available input.
- This allows multiple users to log in simultaneously even though epoll handles the events sequentially. 

### Authentication Handling
- Usernames and passwords are stored in `users.txt`.
- Upon connection, a user must provide credentials.
- **Duplicate logins** are prevented by tracking active usernames.

### Synchronization Considerations
Since we use an **event-driven model instead of threads**, explicit synchronization mechanisms are not required. 

### Message Handling
- **strip_input()** is used to sanitize incoming data.
- If invalid command is send, available commands are displayed.

---

## Implementation
### High-Level Overview of Key Functions
- `setup_listener()`: Initializes the listener socket, sets it to non-blocking, binds it to the port, and registers it with epoll.
- `handle_new_connection()`: Accepts new client connections, sets them to non-blocking, and adds them to epoll.
- `handle_client_message()`: Reads client messages, processes commands, and manages authentication.
- `perform_authentication()`: Verifies login credentials and prevents duplicate logins.
- `process_authenticated_message()`: Parses and executes user commands like `/msg`, `/broadcast`, `/group_msg`, etc.
- `broadcast_message()`: Sends messages to all clients except the sender.
- `run()`: The main event loop that processes incoming connections and messages using `epoll_wait()`.

We have used classes to structure the code properly, also making helper functions and placing all the declarations in the header file. Appropriate helper functions are also created and appropriate comments have been added wherever necessary.

### How the Code Works

1. **Server Initialization and Listening:**
   - **Socket Creation and Binding:**  
     The server creates a listener socket using `socket()`, binds it to the defined port (12345), and sets it to non-blocking mode.
   - **Epoll Setup:**  
     An epoll instance is created (`epoll_create1()`), and the listener socket is added to the epoll watch list. This allows the server to efficiently monitor multiple sockets for events.

2. **Handling New Connections:**
   - **Accepting Connections:**  
     When the listener socket becomes active (indicating a new connection), the `handle_new_connection()` function is called.  
   - **Client Setup:**  
     The server accepts the connection with `accept()`, sets the new socket to non-blocking mode, and sends a combined message containing the username prompt.
   - **Session Initialization:**  
     A `ClientSession` is created for the new connection, with its state set to `WAITING_USERNAME`, and the session is stored in a map keyed by the client’s file descriptor. State is stored as the socket are non-blocking and we do not wait for the user to enter the password immediately allowing for 'concurrency' in this epoll setup. 

3. **Authentication:**
   - **Receiving and Processing Data:**  
     Once the client sends input (username followed by password), the server reads the data in `handle_client_message()`. The code directly uses the received data.
   - **State Transitions:**  
     Depending on the session state (`WAITING_USERNAME` or `WAITING_PASSWORD`), the server either prompts for a password or verifies the credentials against a file (`users.txt`).
   - **Successful Authentication:**  
     Upon a successful login, the client is marked as authenticated, and its information is added to the active user maps. A welcome message is sent, and a broadcast informs other clients of the new connection.

4. **Message Handling:**
   - **Post-Authentication:**  
     After authentication, incoming messages are processed in `process_authenticated_message()`, which handles private messages, broadcasts, and group chat commands.
   - **Non-blocking I/O with Epoll:**  
     The server uses the epoll event loop to continuously check for new data on any active socket. This design avoids blocking on any single client and efficiently manages multiple connections.



### Code Flow (Diagram Representation)
1. **Server Initialization**
   - Create socket → Bind → Listen → Setup epoll
2. **Event Loop (epoll_wait)**
   - If **new connection**: Accept and add to epoll.
   - If **client message**: Read, authenticate, process command.
   - If **client disconnects**: Remove from epoll and close socket.

---

## Testing
### Correctness Testing
- Verified authentication by providing correct/incorrect credentials.
- Tested message formatting and delivery for private and broadcast messages.
- Checked handling of special cases (e.g., sending messages to non-existent users).

### Stress Testing
- Used multiple telnet connections via python script to test scalability.
- Sent large messages to check buffer handling. Messges size is upper-bounded by the Buffer size (1024 bytes).
- Simulated abrupt client disconnections to ensure robustness.

---

## Challenges Faced
1. **Initial Design with Threads**: 
   - We originally considered a multi-threaded server but then switched to epoll after considering the load multiple clients would put on the multi-threading server due to large connections.

2. **Dealing with Non-Blocking I/O**:
   - Some syscalls (`recv()`, `send()`) returned `EAGAIN` due to non-blocking mode.
   - Sign in was sequential as we had used epoll.
   - This made us switch to using non-blocking sockets with state management for authentication to handle concurrent authentications (as it appears).
   - Ensured proper state transitions.

---

## Restrictions
- Maximum clients: 100 (as defined by `MAX_EVENTS`).
- Maximum message size: 1024 bytes. Single message should be smaller than this (which includes usernames and passwords as well).
- Users must be predefined in `users.txt`.
- As epoll is linux specific the code is not portable across different operating systems. Their variants like poll(), select() can be used on Unix systems as well.
---

## Individual Contributions
| Member | Roll Number | Contribution | Percentage |
|--------|-------------|-----------|---------|
| Prathamesh Baviskar | 220285 | Designed and Implemented the server | 33.33 |
| Mayank Gupta | 220638 | Handled Testing and preparing     README | 33.33 | 
| Ayushmaan Jay Singh | 220276  | Handled Testing and preparing README  | 33.33 |

---

## Sources
- **Beej’s Guide to Network Programming**
- **Linux man pages** (`epoll`, `fcntl`, `socket`)
- **Online blogs on event-driven programming**

---

## Declaration
We declare that this project was implemented independently and did not involve plagiarism.

---

## Feedback
- Assignment was well-structured and challenging.
