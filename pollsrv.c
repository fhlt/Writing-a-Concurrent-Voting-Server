/* 
 * pollsrv - a server to track a voting poll
 * 
 * <liangtao>
 */

#include <string.h>
#include <stdlib.h>
#include "csapp.h"



/* You will implement these functions */
void handle_reset();
int  handle_vote();
int  send_vote_totals(int connfd);
int  send_candidates(int connfd);
int  parse(int connfd);
void init();
void *thread(void *vargp);

/* TODO: PUT ANY GLOBAL VARIABLES HERE */
const char *VOTE = "vote";
const char *RESET = "reset";
const char *RESULTS = "results";
const char *CANDIDATES = "candidates";
const char *title_candidates = "flavor,Chocolate Leaderchip Experience,McCandyCain Crunch,Yes Pecan\n";
sem_t oneSignal;
sem_t readCountSignal;
sem_t keySignal;
sem_t semABC;
int readCount = 0;
int a = 0;
int b = 0;
int c = 0;

/* TODO: Complete the following functions */


// handle_reset
// ------------
// Use this function to set all of your vote counters to zero

/*
 * write
 */
void handle_reset()
{
	P(&oneSignal);
	P(&keySignal);
	P(&semABC);
	a = 0;
	b = 0;
	c = 0;	
	V(&semABC);
	V(&keySignal);
	V(&oneSignal);
}

// handle_vote
// -----------
// Use this function to update your vote counts
// for a particular vote (ballot). Note that ballot 
// should be a 1-indexed vote (only 
// 1, 2, and 3 are valid votes)
// Returns: 0 if successful, non-zero value otherwise

/*
 * write
 */
int handle_vote(int ballot)
{
	P(&oneSignal);
	P(&keySignal);
	P(&semABC);
	switch(ballot)
	{
		case 1:
			a++;
			break;
		case 2:
			b++;
			break;
		case 3:
			c++;
			break;
	}
	V(&semABC);
	V(&keySignal);
	V(&oneSignal);
    return 0;
}

// send_vote_totals
// ----------------
// Use this function to respond to a client 
// (noted by connfd) with a total of all the votes. 
// The response should be in the format "x,y,z\n", 
// where x, y, and z are the vote totals for 
// candidates 1, 2, and 3 respectively.
// Returns: 0 if successful, 1 if unsuccessful.

/*
 * read
 */
int send_vote_totals(int connfd)
{
	P(&keySignal);
	P(&readCountSignal);
	if(!readCount)
		P(&semABC);
	readCount++;
	V(&readCountSignal);
	V(&keySignal);

	char send_buffer[MAXBUF];
	char a_str[32];
	char b_str[32];
	char c_str[32];
	memset(a_str, 0, sizeof(a_str));
	memset(b_str, 0, sizeof(b_str));
	memset(c_str, 0, sizeof(c_str));
	sprintf(a_str, "%d", a);
	sprintf(b_str, "%d", b);
	sprintf(c_str, "%d", c);
	strcpy(send_buffer, a_str);
	strcat(send_buffer, ",");
	strcat(send_buffer, b_str);
	strcat(send_buffer, ",");
	strcat(send_buffer, c_str);
	strcat(send_buffer, "\n");

	//printf("------------%s\n",send_buffer);

	V(&readCountSignal);
	readCount--;
	if(!readCount)
		V(&semABC);
	V(&readCountSignal);

	if(send(connfd, send_buffer,strlen(send_buffer), 0) <= 0)
		unix_error("send failed\n");
	else
		return 1;

    return 0;
}

// send_candidates
// ---------------
// Use this function to send the names of the 
// candidates. This should be in the form
// "title,x,y,z\n", where title is the name of
// the poll, and x, y, and z are the names
// of candidates 1, 2, and 3 respectively.
// Returns: 0 if successful, non-zero value otherwise

/*
 * send directly
 */
int send_candidates(int connfd)
{
	char send_buff[MAXBUF];
	strcpy(send_buff, title_candidates);
	if(send(connfd, send_buff, sizeof(send_buff), 0) <= 0)
		unix_error("send failed\n");
	else
		return 1;
    return 0;
}


// parse
// -----
// Use the parse function to read in a client's
// transmission, parse it, and react accordingly.
// Returns: 0 if successful, non-zero value otherwise.

int parse(int connfd) 
{
	int i, j;
	char BUF[MAXBUF];
	char buffer[MAXBUF];
	while(1)
	{
		memset(buffer, 0, MAXBUF);
		memset(BUF, 0, MAXBUF);
		j = 0;
		int len = recv(connfd, buffer, MAXBUF, 0);
		if(len == 0)
			break;
		len = strlen(buffer);

		//printf("recv:%s", buffer);

		for(i = 0;i < len; i++)
		{
			if(buffer[i] == ':')
				break;
			BUF[j++] = buffer[i];
		}
		BUF[j] = '\0';

		if(!strcmp(BUF, VOTE))
		{

			BUF[0] = buffer[++i];
			BUF[1] = '\0';
			handle_vote(atoi(BUF));
			send_vote_totals(connfd);
		}
		else if(!strcmp(BUF, RESET))
		{
			handle_reset(connfd);
			send_vote_totals(connfd);
			//printf(RESET);
		}
		else if(!strcmp(BUF, RESULTS))
		{
			//printf("into results\n");
			send_vote_totals(connfd);
			//printf("results\n");
		}
		else if(!strcmp(BUF, CANDIDATES))
		{
			send_candidates(connfd);
			//printf(CANDIDATES);
		}
		else
		{
			printf("Other------------ %s\n", BUF);
			break;
		}
	}
    return 0;
}

// init
// ----
// Use this function to initialize any global variables

void init()
{
	/* init semphere */
	Sem_init(&keySignal, 0, 1);
	Sem_init(&oneSignal, 0, 1);
	Sem_init(&readCountSignal, 0, 1);
	Sem_init(&semABC, 0, 1);
	/* init a,b,c */
	a = 0;
	b = 0;
	c = 0;

	readCount = 0;
}

// thread
// ------
// All new threads should initially call this function.
// Use this function to handle incoming client transmissions
// and process them accordingly. The return value can be
// returned to the thread's creator.

void *thread(void *vargp)
{
	int connfd = *((int *)vargp);
	Pthread_detach(pthread_self());
	Free(vargp);
	parse(connfd);
	close(connfd);
	pthread_exit(NULL);
    return NULL;
}

// main
// ----
// use this function to do any necessary initialization,
// and listen for incomming transmissions. On an 
// incoming transmission, you should spawn off a 
// thread to handle it.

int main(int argc, char **argv)
{
	init();
	int listenfd, *connfdp, port;
	socklen_t clientlen=sizeof(struct sockaddr_in);
	struct sockaddr_in clientaddr;
	if(argc != 2)
	{
		fprintf(stderr, "usage:%s <port>\n",argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);

	listenfd = Open_listenfd(port);
	while(1)
	{
		pthread_t tid;
		connfdp = Malloc(sizeof(int));
		*connfdp = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		Pthread_create(&tid, NULL, thread ,connfdp);
	}
    return 0;
}

