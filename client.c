#include<sys/types.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>
int
main (int argc, char *argv[])
{

  int sfd, n, flag = 0;
  fd_set rset;

  char buff[1024] = " ";

  struct sockaddr_in server;

  sfd = socket (AF_INET, SOCK_STREAM, 0);

  if (sfd < 0)

    {

      printf ("not created\n");

    }

  bzero (&server, sizeof (struct sockaddr_in));

  server.sin_family = AF_INET;

  server.sin_port = htons (1503);
  //inet_aton ("127.0.0.1", &server.sin_addr);
inet_pton(AF_INET,argv[1],&server.sin_addr);

  if (connect (sfd, (struct sockaddr *) &server, sizeof (server)) == -1)
    {
      printf ("no server online\n");
      exit (0);
    }
  for (;;)

    {

      FD_ZERO (&rset);

      FD_SET (0, &rset);

      FD_SET (sfd, &rset);

      select (sfd + 1, &rset, NULL, NULL, NULL);

      if (FD_ISSET (0, &rset))

	{

//printf (">>");

	  gets (buff);
	  if (flag != 0)
	    strcat (buff, "\r\n");
	  write (sfd, buff, strlen (buff) + 2);
	  flag = 1;
	  bzero (buff, sizeof (buff));
	}

      if (FD_ISSET (sfd, &rset))

	{
	  bzero (buff, sizeof (buff));
	  n = read (sfd, buff, 1024);
	  if (n == 0)
	    {
	      printf ("connection closed by host\n");
	      exit (0);
	    }
	  buff[n] = '\0';
	  printf ("%s", buff);

	}

    }

  close (sfd);

}
