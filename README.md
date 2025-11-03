# CS425 Computer Networks Assignments

This repository contains assignments for the **CS425: Computer Networks** course. Each assignment demonstrates practical implementation of networking concepts through hands-on projects.

## Repository Structure

```
CS425-Assignments/
├── A1/                 # Assignment 1: Multi-Client Chat Server
├── A2/                 # Assignment 2: DNS Resolver
└── README.md           # This file
```

##  Assignments

### [Assignment 1: Multi-Client Chat Server](./A1)

A high-performance, multi-client chat server implementation using **epoll** for event-driven handling on Linux systems.

**Key Features:**
- Event-driven architecture using epoll for efficient handling of multiple clients
- Non-blocking I/O for scalable connections
- User authentication system with credentials stored in `users.txt`
- Private messaging between users
- Broadcast messaging to all connected clients
- Group chat functionality (create, join, leave groups)
- Graceful handling of client disconnections
- Prevention of duplicate logins

**Technologies:** C++, Linux epoll, Socket Programming

**Quick Start:**
```bash
cd A1
make
./server_grp
# Connect using: telnet localhost 12345
```

For detailed documentation, see [A1/README.md](./A1/README.md)

---

### [Assignment 2: DNS Resolver](./A2)

A Python-based DNS resolution system that demonstrates both iterative and recursive DNS lookup mechanisms.

**Key Features:**
- Iterative DNS lookup starting from root servers
- Recursive DNS lookup using system resolver
- Support for querying through ROOT, TLD, and authoritative name servers
- Comprehensive error handling (timeouts, NXDOMAIN, etc.)
- Detailed logging of DNS resolution process

**Technologies:** Python 3, dnspython library

**Quick Start:**
```bash
cd A2
# Iterative lookup
python3 dnsresolver.py iterative example.com

# Recursive lookup
python3 dnsresolver.py recursive example.com
```

For detailed documentation, see [A2/README.md](./A2/README.md)

---





## Getting Started

To explore any assignment:

1. Clone this repository:
   ```bash
   git clone https://github.com/SmartCheese22/CS425-Assignments.git
   cd CS425-Assignments
   ```

2. Navigate to the assignment directory:
   ```bash
   cd A1  # or A2
   ```

3. Follow the instructions in the assignment-specific README

##  Acknowledgments

- Course instructors and TAs for assignment design and guidance
- Beej's Guide to Network Programming
- dnspython documentation and community
- Linux man pages and documentation
