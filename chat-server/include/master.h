#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <set>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

#define BUFF_SIZE 1024

#define WORKING_DIR "/tmp/"
#define PORT 6970

#define BACKLOG_SIZE 10
#define TIMEOUT_SEC 5

#define USERS_CMD "/u"
#define DIRECT_MSG_CMD "/w"
#define BROADCAST_MSG_CMD "/s"
#define LEAVE_CMD "/q"
