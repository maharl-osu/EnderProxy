EnderProxy takes the pain out of managing multiple Minecraft servers. Using subdomain routing, it seamlessly forwards players to the correct destination.


# Overview

This application is a reverse proxy for Minecraft that works similarly to nginx but is designed specifically for the Minecraft protocol. It allows you to host multiple Minecraft servers behind a single IP and external port, eliminating the need to manage complex port assignments.

With built-in subdomain-based routing, players can connect to different servers using clean, memorable addresses like:

`survival.example.com → localhost:25565`

`creative.example.com → localhost:25566`

`skyblock.example.com → localhost:25567`

Key Features

🔹 Single Port, Multiple Servers — No need to expose dozens of ports.

🔹 Subdomain Routing — Seamlessly forward players based on hostname.

🔹 Lightweight & Fast — Built for high performance with minimal overhead.

🔹 Nginx-Inspired Design — Simple configuration for server admins.

Whether you’re running a small home server or managing a large network, this reverse proxy gives you a cleaner, more scalable way to manage Minecraft server infrastructure.

### Note: Due to how reverse proxies work, you should NOT ip-ban players. 

# Example Config

```toml
port = 25565     # Port that the reverse proxy will run on.

[skyblock.example.com]
ip = 192.168.0.2 # IP address to forward requests to skyblock.example.com to
port = 25565     # Port that the server is running on
type = tcp       # WIP, will support TCP as well as UDP for voice chat

[creative.example.com]
ip = 192.168.0.3
port = 25565
type = tcp
```
