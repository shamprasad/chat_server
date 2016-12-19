#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 1503		// port we're listening on
#define SHOW 1			// command show
#define MESSAGE 2
#define HELP 3
#define NICK 4
#define NEWLINE 5
#define VALIDATE 6
void write_to_socket (int target, char *buff_to_send, int len);
int check_for_command (char *buffer, int *p, int, int);
int is_fd_valid (int destination);
void type_writer (char *buf, int len, int fd);


typedef struct
{
  int id;
  char name[20];
  int vlid;			/* socket descripter */
} USER;
USER user[20];
int usercount = 4;


int
main (void)
{
  fd_set master;		// master file descriptor list
  fd_set read_fds;		// temp file descriptor list for select()
  struct sockaddr_in myaddr;	// server address
  struct sockaddr_in remoteaddr;	// client address
  int fdmax;			// maximum file descriptor number
  int listener;			// listening socket descriptor
  int newfd;			// newly accept()ed socket descriptor
  char buf[1024], buf2[20];	// buffer for client data
  int nbytes;
  int yes = 1;			// for setsockopt() SO_REUSEADDR, below
  int addrlen;
  int i, j, k, tmp, destination = 0, l;
  int str_len;
  char c;
  char *tmp_ptr;

  bzero (user, sizeof (USER));

  FD_ZERO (&master);		// clear the master and temp sets
  FD_ZERO (&read_fds);

  // get the listener
  if ((listener = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      //perror ("socket");
      exit (1);
    }

  // lose the  "address already in use" error message
  if (setsockopt (listener, SOL_SOCKET, SO_REUSEADDR, &yes,
		  sizeof (int)) == -1)
    {
      //perror ("setsockopt");
      exit (1);
    }
  // bind
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = INADDR_ANY;
  myaddr.sin_port = htons (PORT);
  memset (&(myaddr.sin_zero), '\0', 8);
  if (bind (listener, (struct sockaddr *) &myaddr, sizeof (myaddr)) == -1)
    {
      //perror ("bind");
      exit (1);
    }
  // listen
  if (listen (listener, 10) == -1)
    {
      //perror ("listen");
      exit (1);
    }

  // add the listener to the master set
  FD_SET (listener, &master);

  // keep track of the biggest file descriptor
  fdmax = listener;		// so far, it's this one

  // main loop
  for (;;)
    {
      read_fds = master;	// copy it
      if (select (fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
	{
	  //perror ("select");
	  exit (1);
	}

      // run through the existing connections looking for data to read
      for (i = 0; i <= fdmax; i++)
	{
	  if (FD_ISSET (i, &read_fds))
	    {			// we got one!!
	      if (i == listener)
		{
		  // handle new connections
		  addrlen = sizeof (remoteaddr);
		  if ((newfd =
		       accept (listener, (struct sockaddr *) &remoteaddr,
			       &addrlen)) == -1)
		    {
		      // //perror ("accept");
		    }
		  else
		    {
		      FD_SET (newfd, &master);	// add to master set
		      if (newfd > fdmax)
			{	// keep track of the maximum
			  fdmax = newfd;
			}
		      l = 4;
		      while (user[l].id != 0)
			l++;
		      user[l].id = newfd;
		      ++usercount;

		      printf ("selectserver: new connection from %s on "
			      "socket %d\n", inet_ntoa (remoteaddr.sin_addr),
			      newfd);
		      bzero (buf, sizeof (buf));
		      strcpy (buf,
			      "\n\n\n\n\n\n\n\n\n\n\n********************************************************************************\n");
		      str_len = strlen (buf);
		      write_to_socket (newfd, buf, str_len);


		      bzero (buf, sizeof (buf));
		      strcpy (buf,
			      "\n\n\t\t\tWelcome to sham and sunil chat server\n\n\n");
		      str_len = strlen (buf);
		      //  type_writer (buf,str_len,newfd);
		      write_to_socket (newfd, buf, str_len);
		      bzero (buf, sizeof (buf));
		      strcpy (buf,
			      "********************************************************************************\n\n"
			      "\t\t\t~Type /SHOW/ to list all online users\n\n"
			      "\t~ To send a message to everyone in the room just type the message and press enter\n\n"
			      "\t\t~ Type /numeric-user/ to send a message to particular user only\n\n"
			      "\t\t ex: /6/ Hi number 6 how r u\n\n"
			      "\t\t\t~ Type /HELP/ to see server commands\n\n\n"
			      "********************************************************************************\n\n\n\n>>");
		      str_len = strlen (buf);
		      write_to_socket (newfd, buf, str_len);
		      bzero (buf, sizeof (buf));
		      strcpy (buf, "enter username\n");
		      str_len = strlen (buf);
		      write_to_socket (newfd, buf, str_len);
		      //strcpy(buff,"\tWelecome to  chat server\n");
		    }
		}
	      else
		{
		  // handle data from a client
		  bzero (buf, sizeof (buf));
		  if ((nbytes = recv (i, buf, sizeof (buf), 0)) <= 0)
		    {
		      // got error or connection closed by client
		      if (nbytes == 0)
			{
			  // connection closed
			  printf ("selectserver: socket %d hung up\n", i);
			  user[i].id = 0;
			  user[i].vlid = 0;
			  bzero (user[i].name, sizeof (user[i].name));
			  --usercount;
			}
		      else
			{
			  //perror ("recv");
			}
		      close (i);	// bye!
		      FD_CLR (i, &master);	// remove from master set
		    }
		  else
		    {
		      // we got some data from a client

		      for (j = 0; j <= fdmax; j++)
			{
			  // send to everyone!
			  if (FD_ISSET (j, &master))
			    {
			      // except the listener and ourselves
			      if (j != listener && j != i)
				{
				  switch (check_for_command
					  (buf, &destination, i, nbytes - 2))
				    {
				    case SHOW:

				      k = usercount - 1;
				      bzero (buf, sizeof (buf));
				      sprintf (buf, ">>");
				      //str_len=strlen(buf);
				      write_to_socket (i, buf, strlen (buf));
				      bzero (buf, sizeof (buf));
				      while (k >= 4)
					{

					  sprintf (buf, "<%s-%d>\n>>",
						   user[k].name, user[k].id);
					  //printf ("%s\n", user[k].name);

					  str_len = strlen (buf);
					  write_to_socket (i, buf, str_len);
					  bzero (buf, sizeof (buf));
					  --k;
					}
				      j = 1111;	// to end the loop
				      break;
				    case VALIDATE:
				      //strcpy(user[i].name,buf);
				      user[i].vlid = 1;
				      //printf ("%s\n", user[i].name);
				      bzero (buf, sizeof (buf));
				      sprintf (buf, ">>");
				      //str_len=strlen(buf);
				      write_to_socket (i, buf, strlen (buf));
				      j = 1111;
				      break;
				    case MESSAGE:
				      if (is_fd_valid (destination))
					{

					  strcpy (buf2, user[i].name);
					  strcat (buf2, ">>");
					  strcat (buf2, buf + 3);
					  str_len = strlen (buf2);
					  write_to_socket (destination, buf2,
							   str_len);
					}
				      else
					{
					  bzero (buf, sizeof (buf));
					  strcpy (buf,
						  "Invalid user\n>>\r\n");
					  str_len = strlen (buf);
					  write_to_socket (i, buf, str_len);
					}
				      j = 1111;	// to end the loop
				      break;

				    case HELP:
				      bzero (buf, sizeof (buf));
				      strcpy (buf,
					      "\n\n\n********************************************************************************\n\n\n"
					      "\t\t\t~Type /SHOW/ to list all online users\n\n"
					      "\t~ To send a message to everyone in the room \n"
					      "\t\t\tjust type the message and press enter\n\n"
					      "\t\t~ Type /numeric-user/ to send a message to particular user only\n\n"
					      "\t\t\t\t ex: /6/ Hi number 6 how r u\n\n"
					      "\t\t\t~ Type /HELP/ to see server commands\n\n\n>>"
					      "********************************************************************************\n\n\n\n>>");
				      str_len = strlen (buf);
				      write_to_socket (i, buf, str_len);
				      j = 1111;
				      break;
				    case NEWLINE:
				      bzero (buf, sizeof (buf));
				      sprintf (buf, ">>");
				      //str_len=strlen(buf);
				      write_to_socket (i, buf, strlen (buf));
				      j = 1111;
				      break;
				    default:
				      bzero (buf2, sizeof (buf2));
				      strcat (buf2, user[i].name);
				      strcat (buf2, ">>");
				      strcat (buf2, buf);
				      str_len = strlen (buf2);
				      write_to_socket (j, buf2, str_len);
				      break;
				    }
				  //}
				}
			    }
			}
		    }
		}
	    }
	}
    }
  return 0;
}


void
write_to_socket (int target, char *buff_to_send, int len)
{
  if (!write (target, buff_to_send, len))
    {
      perror ("Write");
      exit (0);
    }
}

int
check_for_command (char *buffer, int *p, int i, int bytes)
{
  char tmp[6];
  strncpy (tmp, buffer, 5);
  while (*buffer == ' ')
    ++buffer;
  if (user[i].vlid == 0)
    {
      //printf ("%d\n", bytes);
      strncpy (user[i].name, buffer, bytes);
      user[i].name[bytes] = '\0';

      return VALIDATE;

    }
  if (!strncmp (buffer, "/SHOW/", 6) || !strncmp (buffer, "/show/", 6))
    return SHOW;
  else if (!strncmp (buffer, "/HELP/", 6) || !strncmp (buffer, "/help/", 6))
    return HELP;
  else if ((*buffer == '/')
	   && ((*(buffer + 2) == '/') || (*(buffer + 3) == '/')))
    {
      *p = *(buffer + 1) - '0';
      if (*(buffer + 3) == '/')
	*p = (*p) * 10 + *(buffer + 2);

      return MESSAGE;
    }
  else if (*buffer == '\r')
    return NEWLINE;

  else
    return 0;
}

int
is_fd_valid (int destination)
{
  int i;
  for (i = 0; i < usercount; ++i)
    if (user[i].id == destination)
      return 1;
  return 0;
}
