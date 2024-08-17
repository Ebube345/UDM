#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 3555
#define BUFFER_LEN 100
#define FALSE 0
#define SERVER_HOSTNAME "ServerHostName"
#define NETDB_MAX_HOST_NAME_LENGTH 100

int main(int argc, char **argv) {
  int sd = -1, rc = 0;
  char server[NETDB_MAX_HOST_NAME_LENGTH];
  char buffer[BUFFER_LEN];
  struct sockaddr_in serveraddr;
  socklen_t serveraddrlen = sizeof(serveraddrlen);
  struct addrinfo hints, *res;

  do {
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
      perror("socket() failed");
      break;
    }

    if (argc > 1) {
      strcpy(server, argv[1]);
    } else
      strcpy(server, SERVER_HOSTNAME);

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVER_PORT);

    rc = inet_pton(AF_INET, server, &serveraddr.sin_addr.s_addr);

    if (rc != 1) {
      // convert the hostname string to an actual ip address using getaddrinfo()
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_INET;
      hints.ai_flags = AI_V4MAPPED;

      rc = getaddrinfo(server, NULL, &hints, &res);

      if (rc != 0) {
        printf("Host not found! (%s)\n", server);
        break;
      }

      memcpy(&serveraddr.sin_addr,
             (&((struct sockaddr_in *)(res->ai_addr))->sin_addr),
             sizeof(serveraddr.sin_addr));

      freeaddrinfo(res);
    }

    // init data block to be sent to server
    memset(buffer, 0, sizeof(buffer));
    if (argc > 2)
      strcpy(buffer, argv[2]);
    else
      strcpy(buffer, "A CLIENT REQUEST");

    rc = sendto(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&serveraddr,
                sizeof(serveraddr));

    if (rc < 0) {
      perror("sendto() failed");
      break;
    }

    rc = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&serveraddr,
                  &serveraddrlen);

    if (rc < 0) {
      perror("recvfrom() failed");
      break;
    }

    printf("client received the following <%s>\n", buffer);
    inet_ntop(AF_INET, &serveraddr.sin_addr.s_addr, buffer, sizeof(buffer));
    printf("from port %d, from address %s\n", ntohs(serveraddr.sin_port),
           buffer);

  } while (FALSE);

  if (sd != -1)
    close(sd);
}
