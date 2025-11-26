# Communication Networks Course - Project Portfolio

A comprehensive collection of networking projects demonstrating various network protocols, socket programming, and network applications built as part of a Communication Networks university course.

## 📋 Table of Contents

- [Overview](#overview)
- [Projects](#projects)
  - [Assignment 1: Network Foundations](#assignment-1-network-foundations)
  - [Assignment 2: Calculator with Proxy](#assignment-2-calculator-with-proxy)
  - [Assignment 3: File Transfer Protocol](#assignment-3-file-transfer-protocol)
  - [Assignment 4: ICMP Ping & Watchdog](#assignment-4-icmp-ping--watchdog)
  - [Assignment 5: Packet Sniffing & Spoofing](#assignment-5-packet-sniffing--spoofing)
  - [Final Project: Network Services Suite](#final-project-network-services-suite)
- [Technologies Used](#technologies-used)
- [Setup & Installation](#setup--installation)
- [Code Standards](#code-standards)
- [License](#license)

## 🎯 Overview

This repository contains a series of networking projects that explore fundamental and advanced concepts in computer networking. Each assignment builds upon previous knowledge, progressing from basic network communication to complex protocol implementation and network security concepts.

### Key Learning Outcomes

- Socket programming in Python and C
- Implementation of network protocols (TCP, UDP, RUDP, ICMP, DHCP, DNS)
- Network packet analysis and manipulation
- Client-server architecture
- Proxy server implementation
- Network security concepts (packet spoofing)
- Congestion control algorithms

## 📚 Projects

### Assignment 1: Network Foundations

**Status:** Documentation Only  
**Format:** Word Document  
**Description:** Theoretical foundation and documentation for network communication concepts.

---

### Assignment 2: Calculator with Proxy

**Language:** Python  
**Protocol:** TCP/IP  
**Components:**
- `api.py` - Protocol definitions and packet structure
- `calculator.py` - Expression parsing and evaluation engine
- `client.py` - Client application for sending calculations
- `server.py` - Calculator server with multi-threading support
- `proxy.py` - Caching proxy server with configurable policies

**Features:**
- Custom binary protocol with header structure
- Support for mathematical expressions with operators and functions
- Response caching with cache control policies
- Multi-threaded server for concurrent client connections
- Error handling for both client and server errors

**Usage:**
```bash
# Start the server
python server.py

# Start the proxy (optional)
python proxy.py

# Run client
python client.py
```

---

### Assignment 3: File Transfer Protocol

**Language:** C  
**Protocol:** TCP with custom reliability layer  
**Components:**
- `Sender.c` - File transfer server
- `Receiver.c` - File transfer client

**Features:**
- Custom file transfer protocol implementation
- Congestion control using both Reno and Cubic algorithms
- Key exchange for authentication
- Chunk-based file transmission with acknowledgments
- Support for file retransmission
- Connection state management

**Compilation:**
```bash
gcc Sender.c -o sender
gcc Receiver.c -o receiver
```

**Usage:**
```bash
# Terminal 1 - Start sender
./sender

# Terminal 2 - Start receiver
./receiver
```

---

### Assignment 4: ICMP Ping & Watchdog

**Language:** C  
**Protocol:** ICMP (Raw Sockets)  
**Components:**
- `ping.c` - Basic ICMP echo request/reply implementation
- `watchdog.c` - Connection monitoring service
- `new_ping.c` - Enhanced ping with watchdog integration

**Features:**
- Raw socket programming for ICMP
- Checksum calculation for packet integrity
- Process management with fork/exec
- Timeout detection mechanism
- Client-server communication monitoring

**Compilation:**
```bash
make all
```

**Usage:**
```bash
# Basic ping
sudo ./ping <hostname>

# Ping with watchdog
sudo ./new_ping <hostname>
```

---

### Assignment 5: Packet Sniffing & Spoofing

**Language:** C  
**Protocol:** Various (Ethernet, IP, ICMP, TCP, UDP)  
**Libraries:** libpcap  
**Components:**
- `sniffer.c` - TCP packet capture and analysis
- `spoofer.c` - ICMP packet spoofing
- `SniffAndSpoof.c` - Integrated capture and response
- `Gateway.c` - Simulated unreliable network gateway

**Features:**
- Live packet capture using libpcap
- Packet header parsing (Ethernet, IP, TCP, ICMP)
- Packet analysis and logging
- ICMP echo reply spoofing
- Automatic response to captured packets
- Network simulation with 50% packet loss

**Compilation:**
```bash
make
```

**Usage:**
```bash
# Packet sniffer
sudo ./sniffer

# ICMP spoofer
sudo ./spoofer <source_ip> <dest_ip>

# Sniff and spoof
sudo ./SniffAndSpoof
```

---

### Final Project: Network Services Suite

**Language:** Python  
**Protocols:** TCP, UDP, RUDP, DHCP, DNS, HTTP  
**Components:**
- `Client.py` - GUI-based HTTP downloader
- `TCPserver.py` - TCP-based HTTP server
- `RUDPserver.py` - RUDP-based HTTP server
- `DHCP_Server.py` - DHCP server implementation
- `DNS_Server.py` - DNS resolver with caching
- `client1.py` - Advanced client application
- `main.py` - Unified launcher interface

**Features:**
- DHCP server for dynamic IP allocation
- DNS server with caching and forwarding
- HTTP file download over TCP and RUDP
- GUI interface for user interaction
- Threaded server implementations
- Integrated service launcher

**Usage:**
```bash
# Launch the main application
sudo python3 main.py
```

**Application Features:**
1. **HTTP Downloader**
   - Support for both TCP and RUDP protocols
   - User-friendly GUI for URL input
   - Automatic file download and saving

2. **DHCP Server**
   - IP address pool management
   - DHCP discover/offer/request/ack cycle
   - IP availability checking via ICMP

3. **DNS Server**
   - Local DNS caching
   - Recursive query resolution
   - Dynamic DNS updates
   - Integration with DHCP for hostname registration

---

## 🛠️ Technologies Used

### Programming Languages
- **Python 3.x** - High-level networking applications
- **C** - Low-level protocol implementation and packet manipulation

### Libraries & Tools
- **Python:**
  - `socket` - Socket programming
  - `threading` - Multi-threaded servers
  - `tkinter` - GUI applications
  - `dnspython` - DNS protocol handling
  - `struct` - Binary data packing/unpacking
  - `pickle` - Object serialization

- **C:**
  - `socket.h` - Socket API
  - `netinet/*` - Network protocol headers
  - `pcap.h` - Packet capture library
  - Standard C libraries

### Network Protocols
- TCP (Transmission Control Protocol)
- UDP (User Datagram Protocol)
- RUDP (Reliable UDP - Custom Implementation)
- ICMP (Internet Control Message Protocol)
- DHCP (Dynamic Host Configuration Protocol)
- DNS (Domain Name System)
- HTTP (Hypertext Transfer Protocol)

---

## 🚀 Setup & Installation

### Prerequisites

**For Python Projects (Assignment 2, Final Project):**
```bash
# Python 3.7 or higher
python3 --version

# Install required packages
pip install dnspython
```

**For C Projects (Assignments 3, 4, 5):**
```bash
# GCC Compiler
gcc --version

# libpcap (for Assignment 5)
sudo apt-get install libpcap-dev  # Ubuntu/Debian
sudo yum install libpcap-devel     # RHEL/CentOS
```

### Installation

1. **Clone the Repository:**
```bash
git clone https://github.com/YourUsername/CommunicationNetwork-Course.git
cd CommunicationNetwork-Course
```

2. **Navigate to Specific Assignment:**
```bash
cd Assignment-X  # Replace X with assignment number
```

3. **Follow assignment-specific instructions** in the respective sections above.

---

## 📝 Code Standards

This repository follows professional coding standards:

### Python Code
- **PEP 8** style guidelines
- Comprehensive docstrings for all modules, classes, and functions
- Type hints for function parameters and return values
- Snake_case naming convention for variables and functions
- Proper error handling and exception management

### C Code
- **Snake_case** naming convention for all identifiers
- Comprehensive function documentation with parameter and return descriptions
- Consistent code formatting and indentation
- Clear comments explaining complex logic
- Proper error checking for system calls

### Common Standards
- Clear and descriptive variable names
- Modular code structure
- Separation of concerns
- Inline comments for complex algorithms
- Consistent naming across related projects

---

## 📂 Repository Structure

```
CommunicationNetwork-Course/
├── .gitignore
├── README.md
├── Assignment-1/
│   └── Assignment-1.docx
├── Assignment-2/
│   ├── api.py
│   ├── calculator.py
│   ├── client.py
│   ├── server.py
│   └── proxy.py
├── Assignment-3/
│   ├── Sender.c
│   ├── Receiver.c
│   ├── send.txt
│   └── makefile
├── Assignment-4/
│   ├── ping.c
│   ├── watchdog.c
│   ├── new_ping.c
│   ├── makefile
│   └── *.pcapng
├── Assignment-5/
│   ├── sniffer.c
│   ├── spoofer.c
│   ├── SniffAndSpoof.c
│   ├── Gateway.c
│   ├── makefile
│   └── *.pcapng
└── Final Project/
    ├── Code/
    │   ├── Client.py
    │   ├── TCPserver.py
    │   ├── RUDPserver.py
    │   ├── DHCP_Server.py
    │   ├── DNS_Server.py
    │   ├── client1.py
    │   └── main.py
    ├── Wireshark_Records/
    ├── Example_URLs.txt
    └── Explanations.pdf
```

---

## ⚠️ Important Notes

### Permissions
Many network applications require elevated privileges to create raw sockets:

```bash
sudo python3 <script.py>  # For Python scripts  
sudo ./<program>          # For compiled C programs
```

### Network Safety
- These tools are for **educational purposes only**
- Use only on networks you own or have explicit permission to test
- Be aware of your institution's/organization's network usage policies
- Packet sniffing and spoofing may be illegal in some jurisdictions

### Platform Compatibility
- **Linux** recommended for all assignments
- Some features (raw sockets, packet capture) require Linux/Unix
- Windows users should use WSL (Windows Subsystem for Linux) or a Linux VM

---

## 🎓 Learning Objectives Achieved

Through these projects, the following competencies were developed:

1. **Socket Programming**
   - Client-server architecture
   - TCP vs UDP communication
   - Raw socket manipulation

2. **Protocol Implementation**
   - Custom protocol design
   - Binary protocol encoding/decoding
   - State machine implementation

3. **Network Layer Understanding**
   - Packet structure and headers
   - Checksum calculation
   - TTL and routing concepts

4. **Application Layer Services**
   - DHCP for dynamic configuration
   - DNS for name resolution
   - HTTP for data transfer

5. **Network Security**
   - Packet analysis
   - Spoofing techniques
   - Authentication mechanisms

6. **Performance Optimization**
   - Congestion control algorithms (Reno, Cubic)
   - Caching strategies
   - Multi-threading for scalability

---

## 📄 License

This project is part of academic coursework. All rights reserved.

**Note:** This code is provided for educational purposes. Please respect academic integrity policies and do not use this code for your own assignments without proper attribution.

---

## 👤 Author

**Student ID:** [Your Student ID]  
**Course:** Communication Networks  
**Institution:** [Your University]  
**Year:** [Academic Year]

---

## 🙏 Acknowledgments

- Course instructor and teaching assistants
- Python Software Foundation for excellent networking libraries
- libpcap developers for packet capture capabilities
- The open-source community for tools and documentation

---

**Last Updated:** November 2025
