#include "shared/online_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Online_Server_Config {
    const char* lobbyName;
    const char* lobbyPassword;
    const char* server;
    unsigned short port;
};

static void Print_Usage(const char* executable_name) {
    printf("usage: %s [--name lobby] [--password password] [--port port] [--server address/url]\n", executable_name);
}

static int Parse_Args(Online_Server_Config* config, int argc, char** argv) {
    for (int index = 1; index < argc; index++) {
        const char* argument = argv[index];

        if (strcmp(argument, "--name") == 0 && index + 1 < argc) {
            config->lobbyName = argv[++index];
        }
        else if (strcmp(argument, "--password") == 0 && index + 1 < argc) {
            config->lobbyPassword = argv[++index];
        }
        else if (strcmp(argument, "--port") == 0 && index + 1 < argc) {
            unsigned long port = strtoul(argv[++index], nullptr, 10);
            if (port == 0 || port > 65535) {
                return 0;
            }
            config->port = static_cast<unsigned short>(port);
        }
        else if (strcmp(argument, "--server") == 0 && index + 1 < argc) {
            config->server = argv[++index];
        }
        else if (strcmp(argument, "--help") == 0) {
            return 0;
        }
        else {
            return 0;
        }
    }

    return 1;
}

int main(int argc, char** argv) {
    Online_Server_Config config = {};
    config.lobbyName = "BigTiddyGothGF";
    config.lobbyPassword = "";
    config.server = "https://dusklight.modloader64.com";
    config.port = 32123;

    if (!Parse_Args(&config, argc, argv)) {
        Print_Usage(argv[0]);
        return 1;
    }

    printf("Dusklight Online server\n");
    printf("protocol=%u\n", ONLINE_PROTOCOL_VERSION);
    printf("lobby=%s server=%s:%s\n", config.lobbyName, config.server, config.port);
    printf("network runtime is not started yet\n");
    return 0;
}
