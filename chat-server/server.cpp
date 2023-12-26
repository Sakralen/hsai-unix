#include "master.h"
#include "server.h"

int init_listener_socket()
{
    struct sockaddr_in addr;

    int listener = socket(AF_INET, SOCK_STREAM, 0);

    if (listener < 0)
    {
        syslog(LOG_ERR, "Failed to create listener socket.");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        syslog(LOG_ERR, "Failed to bind listener.");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "Chat-server started successfully.");

    return listener;
}

string clear_string(string s)
{
    string res;
    std::remove_copy_if(s.begin(), s.end(), back_inserter(res), [](unsigned char c)
                        { return c < 32; });
    return res;
}

bool is_user_exist(Clients &clients, string nickname)
{
    for (auto it = clients.nicknames.begin(); it != clients.nicknames.end(); it++)
    {
        if ((*it).second == nickname)
        {
            return true;
        }
    }
    return false;
}

bool is_user_exist(Clients &clients, int sock)
{
    for (auto it = clients.nicknames.begin(); it != clients.nicknames.end(); it++)
    {
        if ((*it).first == sock)
        {
            return true;
        }
    }
    return false;
}

int get_user_sock_by_nn(Clients &clients, string nickname)
{
    for (auto it = clients.nicknames.begin(); it != clients.nicknames.end(); it++)
    {
        if ((*it).second == nickname)
        {
            return (*it).first;
        }
    }
    return -1;
}

string get_user_nn_by_sock(Clients &clients, int sock)
{
    for (auto it = clients.nicknames.begin(); it != clients.nicknames.end(); it++)
    {
        if ((*it).first == sock)
        {
            return (*it).second;
        }
    }

    // TODO: add Optional.
    return "";
}

void handle_message(Clients &clients, vector<int> &socks_to_erase, int sock, string message)
{
    istringstream iss(message);
    string command;

    iss >> command;

    syslog(LOG_ERR, "Command content: [%s]", command.c_str());

    if (command == USERS_CMD)
    {
        string server_message_str = "Users:\n";
        for (auto it = clients.nicknames.begin(); it != clients.nicknames.end(); it++)
        {
            server_message_str += (*it).second + "\n";
        }

        const char *server_message = server_message_str.c_str();
        send(sock, server_message, strlen(server_message), 0);

        return;
    }

    if (command == DIRECT_MSG_CMD)
    {
        string user_nn, message;

        iss >> user_nn;
        getline(iss, message);

        if (message.length() != 0)
        {
            message = message.substr(1);
        }

        int user_sock = get_user_sock_by_nn(clients, user_nn);
        if (user_sock == -1)
        {
            const char *server_message = "Invalid nickname.\n";
            send(sock, server_message, strlen(server_message), 0);
            return;
        }

        string server_message_str = "Message from " + get_user_nn_by_sock(clients, sock) + ":\n" + message + "\n";
        const char *server_message = server_message_str.c_str();
        send(user_sock, server_message, strlen(server_message), 0);

        return;
    }

    if (command == BROADCAST_MSG_CMD)
    {
        string message;

        getline(iss, message);

        if (message.length() != 0)
        {
            message = message.substr(1);
        }

        string server_message_str = "Message from " + get_user_nn_by_sock(clients, sock) + ":\n" + message + "\n";
        const char *server_message = server_message_str.c_str();

        for (auto it = clients.nicknames.begin(); it != clients.nicknames.end(); it++)
        {
            if ((*it).first != sock)
            {
                send((*it).first, server_message, strlen(server_message), 0);
            }
        }

        return;
    }

    if (command == LEAVE_CMD)
    {
        const char *server_message = "Disconnected.\n";
        send(sock, server_message, strlen(server_message), 0);

        socks_to_erase.push_back(sock);

        return;
    }

    // const char *server_message = "Unknown command.\n";
    // send(sock, server_message, strlen(server_message), 0);

    string bad_msg = string(USERS_CMD) + " to list users, " + string(DIRECT_MSG_CMD) + " to send direct message, " + string(BROADCAST_MSG_CMD) + " to broadcast message, " + string(LEAVE_CMD) + " to leave.\n";
    send(sock, bad_msg.c_str(), strlen(bad_msg.c_str()), 0);
}

void serve(int listener, Clients &clients, vector<int> &socks_to_erase, timeval &timeout)
{
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(listener, &readset);
    for (auto it = clients.socks.begin(); it != clients.socks.end(); it++)
    {
        FD_SET(*it, &readset);
    }

    int mx;
    if (clients.socks.empty())
    {
        mx = listener;
    }
    else
    {
        mx = -1;
        for (auto it = clients.socks.begin(); it != clients.socks.end(); it++)
        {
            mx = max(mx, *it);
        }
    }

    if (select(mx + 1, &readset, NULL, NULL, &timeout) < 0)
    {
        syslog(LOG_ERR, "Select error.");
        exit(EXIT_FAILURE);
    }

    if (FD_ISSET(listener, &readset))
    {
        int sock = accept(listener, NULL, NULL);
        if (sock < 0)
        {
            syslog(LOG_ERR, "Accept error.");
            exit(EXIT_FAILURE);
        }
        fcntl(sock, F_SETFL, O_NONBLOCK);

        syslog(LOG_INFO, "Client accepted.");

        const char *server_message = "Enter your nickname:\n";
        send(sock, server_message, strlen(server_message), 0);
        clients.socks.insert(sock);
    }

    for (auto it = clients.socks.begin(); it != clients.socks.end(); it++)
    {
        if (FD_ISSET(*it, &readset))
        {
            char buff[BUFF_SIZE];
            memset(buff, 0, sizeof(buff));

            int recvd_num = recv(*it, buff, sizeof(buff), 0);

            if (buff[0] == '\0')
            {
                syslog(LOG_INFO, "Recieved \\0 from %d.", *it);
                socks_to_erase.push_back(*it);
                continue;
            }

            if (recvd_num < 0)
            {
                // close(*it);
                syslog(LOG_INFO, "Connection from socket %d refused.", *it);
                socks_to_erase.push_back(*it);
                continue;
            }

            string message = clear_string(string(buff));
            if (!is_user_exist(clients, *it))
            {
                syslog(LOG_INFO, "User does not exist.");

                if (!is_user_exist(clients, message))
                {
                    clients.nicknames[*it] = message;
                    syslog(LOG_INFO, "User added.");
                }
                else
                {
                    const char *server_message = "User already exists.\n";
                    send(*it, server_message, strlen(server_message), 0);

                    socks_to_erase.push_back(*it);
                    syslog(LOG_INFO, "User disconnected.");
                }
                continue;
            }

            handle_message(clients, socks_to_erase, *it, message);
            memset(buff, 0, sizeof(buff));
        }
    }

    for (auto it = socks_to_erase.begin(); it != socks_to_erase.end(); it++)
    {
        clients.nicknames.erase(*it);
        clients.socks.erase(*it);
        close(*it);
    }
    socks_to_erase.clear();
}
