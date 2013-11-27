#include "libircclient/libircclient.h"
#include "libircclient/libirc_rfcnumeric.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <iconv.h>

#define DEFAULT_CFG_FILE  	"/.quizbot.cfg"
#define DEFAULT_IRC_SERVER  	"irc.freenode.org"
#define DEFAULT_IRC_PORT	"6667"
#define DEFAULT_IRC_CHANNEL  	"#qircbot"
#define DEFAULT_IRC_NICK	"qircbot"
#define DEFAULT_IRC_USERNAME 	"qircbot"
#define DEFAULT_IRC_REALNAME 	"qircbot"
//Nick to listen to for questions
#define DEFAULT_QUIZBOT_NICK	"juicer"

#define DEFAULT_QUIZ_PROMPT	"{MoxQuizz} The question no."

/*
   Example output from MozzQuiz:
   {MoxQuizz} The question no. 4 by serv is:
   (Maltin's Movies) Moody version of Herman Melville sea classic, with Peck lending a deranged dignity to the role of Captain Ahab.
   
   Example output from question file:
   Category: Maltin's Movies
   Question: Moody version of Herman Melville sea classic, with Peck lending a deranged dignity to the role of Captain Ahab.
   Answer: Moby Dick
   Author: serv
*/

static void answer_question (const char *question);

struct cfg {
	char cfg_file[1024];
	char server[255];
	char port[16];
	char channel[64];
	char nick[64];
	char username[16];
	char realname[16];
	char server_connect_msg[255];
	char server_connect_nick[16];
	char server_connect_delay[6];
	char channel_connect_msg[255];
	char channel_connect_nick[16];
	char channel_connect_delay[6];
	char quizbot_nick[16];
} irc_cfg = {
	DEFAULT_CFG_FILE,
	DEFAULT_IRC_SERVER,
	DEFAULT_IRC_PORT,
	DEFAULT_IRC_CHANNEL,
	DEFAULT_IRC_NICK,
	DEFAULT_IRC_USERNAME,
	DEFAULT_IRC_REALNAME,
	"",
	"",
	"",
	"",
	"",
	"",
	DEFAULT_QUIZBOT_NICK
};

#define NUM_CFG_OPTS 13

const char *cfg_options[] = {
	"server", "port", "channel", "nick", "username", "realname", 
	"server_connect_msg", "server_connect_nick", "server_connect_delay",
	"channel_connect_msg", "channel_connect_nick", "channel_connect_delay",
	"quizbot_nick"
};

char *cfg_vars[] = {
	irc_cfg.server, irc_cfg.port, irc_cfg.channel, irc_cfg.nick, irc_cfg.username, irc_cfg.realname,
		irc_cfg.server_connect_msg, irc_cfg.server_connect_nick, irc_cfg.server_connect_delay,
		irc_cfg.channel_connect_msg, irc_cfg.channel_connect_nick, irc_cfg.channel_connect_delay,
		irc_cfg.quizbot_nick
};

int verbose = 0;
int use_default_cfg = 1;
int quizbot_prompt = 0;

irc_session_t *session;

//Sent on successful connection to server, useful for NickServ
static void send_server_connect_msg (void)
{
	//Check to see if we have commands to run
	if (strlen (irc_cfg.server_connect_msg) != 0)
	{
		//Make sure a nick was specified
		if (strlen (irc_cfg.server_connect_nick) == 0)
		{
			fprintf (stderr, "IRC: server_connect_msg specified but not server_connect_nick\n");
			return;
		}
		if (atoi (irc_cfg.server_connect_delay) > 0)
		{
			if (verbose)
				fprintf (stdout, "IRC: Waiting %i seconds before sending command\n", atoi(irc_cfg.server_connect_delay));
			sleep (atoi (irc_cfg.server_connect_delay));
		}
		if (verbose)
			fprintf (stdout, "IRC: Sending server_connect_msg\n");
		irc_cmd_msg (session, irc_cfg.server_connect_nick, irc_cfg.server_connect_msg);
	}
}

//Sent on successful connection to channel, useful for ChanServ
static void send_channel_connect_msg (void)
{
	//Check to see if we have commands to run
	if (strlen (irc_cfg.channel_connect_msg) != 0)
	{
		//Make sure a nick was specified
		if (strlen (irc_cfg.channel_connect_nick) == 0)
		{
			fprintf (stderr, "IRC: channel_connect_msg specified but not channel_connect_nick\n");
			return;
		}
		if (atoi (irc_cfg.channel_connect_delay) > 0)
		{
			if (verbose)
				fprintf (stdout, "IRC: Waiting %i seconds before sending command\n", atoi(irc_cfg.channel_connect_delay));
			sleep (atoi (irc_cfg.channel_connect_delay));
		}
		if (verbose)
			fprintf (stdout, "IRC: Sending channel_connect_msg\n");
		irc_cmd_msg (session, irc_cfg.channel_connect_nick, irc_cfg.channel_connect_msg);
	}

}

//Called when successfully connected to a server
void event_connect (irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	fprintf (stdout, "IRC: Successfully connected to server %s\n", irc_cfg.server);

	send_server_connect_msg ();
	
	if (verbose)
		fprintf (stdout, "IRC: Attempting to join %s\n", irc_cfg.channel);

	if (irc_cmd_join (session, irc_cfg.channel, NULL))
	{
		fprintf (stderr, "IRC: Error joining channel %s\n", irc_cfg.channel);
		return;
	}
	
	fprintf (stdout, "IRC: Connected to %s\n", irc_cfg.channel);

	send_channel_connect_msg ();
}

void event_privmsg (irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	printf ("'%s' said to me (%s): %s\n", origin ? origin : "someone", params[0], params[1] );
}

void event_numeric (irc_session_t *session, unsigned int event, const char *origin, const char **params, unsigned int count)
{
	if (!verbose)
		return;

	switch (event)
	{
		case LIBIRC_RFC_RPL_NAMREPLY:
		      fprintf (stdout, "IRC: User list: %s\n", params[3]);
		      break;
		case LIBIRC_RFC_RPL_WELCOME:
		      fprintf (stdout, "IRC: %s\n", params[1]);
		      break;
		case LIBIRC_RFC_RPL_YOURHOST:
		      fprintf (stdout, "IRC: %s\n", params[1]);
		case LIBIRC_RFC_RPL_ENDOFNAMES:
		      fprintf (stdout, "IRC: End of user list\n");
		      break;
		case LIBIRC_RFC_RPL_MOTD:
		      fprintf (stdout, "%s\n", params[1]);
		      break;
		default:
		      fprintf (stdout, "IRC: event_numeric %u\n", event);
		      break;
	}
}

void event_channel (irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	char channel_text[1024];
	printf ("Channel text %s\n", channel_text);
	//We just saw a message from the quizbot
	if (strcmp (origin, irc_cfg.quizbot_nick) == 0)
	{

		if (quizbot_prompt)
		{		
			//This message should contain the question text
			answer_question (channel_text);
			quizbot_prompt = 0;
		}
		//The message was the start of a new question
		else if (strstr (channel_text, DEFAULT_QUIZ_PROMPT) != NULL)
		{
			if (verbose)
				fprintf (stdout, "Found prompt, ready for question\n");
			quizbot_prompt = 1;
		}
	}
}

void print_usage (void)
{
	fprintf (stdout, "quizbot\n");
	fprintf (stdout, "Options:\n");
	fprintf (stdout, "-c		Specify config file location, default %s\n", DEFAULT_CFG_FILE);
	fprintf (stdout, "-h		This help text\n");
}

static void answer_question (const char *question)
{
	char question_buff[1024];
	char answer_buff[1024];
	FILE *q_file;
	int found_answer = 0;

	//Strip out the category part
	strncpy (question_buff, strchr (question, ')') + 3, sizeof(question_buff));
	if (verbose)
		fprintf (stdout, "Question was: %s\n", question_buff);

	q_file = fopen ("questions.db", "r");
	if (q_file == NULL)
	{
		fprintf (stderr, "Error reading question database\n");
		return;
	}

	//Scan answer file
	while (fgets (answer_buff, sizeof(answer_buff), q_file) != NULL && !found_answer)
	{
		if (strstr (answer_buff, question_buff) != NULL)
		{
			fprintf (stdout, "Found match %s\n", answer_buff);
			//We found a match, grab the next line which will be the answer
			fgets (answer_buff, sizeof(answer_buff), q_file);
			found_answer = 1;
			fclose (q_file);
		}
	}
	//Strip off the answer part
	strcpy (answer_buff, strchr (answer_buff, ':') + 1);
	if (verbose)
		fprintf (stdout, "I think the answer is %s\n", answer_buff);
	irc_cmd_msg (session, irc_cfg.channel, answer_buff);
}

static int cfg_load (void)
{
	FILE *in_file;
	char cfg_buff[64];
	int i;
	

	//If no config file was specified on the command line, look in the users home directory for one
	if (use_default_cfg)
	{
		strcpy (cfg_buff, getenv("HOME"));
		strcat (cfg_buff, irc_cfg.cfg_file);
		strcpy (irc_cfg.cfg_file, cfg_buff);
	}
	
	if (verbose)
	{
		fprintf (stdout, "IRC: Attempting to Load config file %s\n", irc_cfg.cfg_file);
	}

	in_file = fopen (irc_cfg.cfg_file, "r");

	if (in_file == NULL && !use_default_cfg)
		return 2;
	else if (in_file == NULL && use_default_cfg)
		return 1;

	//Copy a line from cfg file into cfg_buff
	while (fgets (cfg_buff, 64, in_file) != NULL)
	{
		//Cycle through the different options
		for (i = 0; i < NUM_CFG_OPTS; i++)
		{
			//Look for a match and make sure there's a = after it
			if (strncmp (cfg_buff, cfg_options[i], strlen(cfg_options[i])) == 0 &&
					strncmp (cfg_buff + strlen (cfg_options[i]), "=", 1) == 0)
			{
				//Copy to variable
				strcpy (cfg_vars[i], cfg_buff + strlen (cfg_options[i]) + 1);
				//Strip newline
				strtok (cfg_vars[i], "\n");
				//Check if empty var
				if (strcmp (cfg_vars[i], "\n") == 0)
				{
					fprintf (stderr, "IRC: Empty cfg option %s\n", cfg_options[i]);
					fclose (in_file);
					return 1;
				}
			}
		}
	}
	fclose (in_file);
	return 0;

}

int main (int argc, char **argv)
{
	irc_callbacks_t callbacks;
	int c;
	int ret;

	// Read command line options
	while ((c = getopt (argc, argv, "c:vh")) != -1)
	{
		switch (c)
		{
			default:
			case 'h':
				print_usage ();
				return 0;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'c':
				strcpy (irc_cfg.cfg_file, optarg);
				use_default_cfg = 0;
				break;
			case '?':
				if (optopt == 'c')
					fprintf (stderr, "Option -c requires an argument\n");
				else
					fprintf (stderr, "Unrecognised option %c\n", optopt);
				fprintf (stderr, "Error reading commmand line options\n");
				return 1;
				break;
		}
	}
	
	ret = cfg_load ();
	if (ret > 1)
	{
		fprintf (stderr, "Error reading configuration file %s\n", irc_cfg.cfg_file);
		return 1;
	}
	else if (ret == 1)
	{
		fprintf (stdout, "No configuration file found, using defaults\n");
	}

	if (verbose)
	{
		fprintf (stdout, "Configuration options:\n");
		fprintf (stdout, "server = %s\n", irc_cfg.server);
		fprintf (stdout, "port = %s\n", irc_cfg.port);
		fprintf (stdout, "channel = %s\n", irc_cfg.channel);
		fprintf (stdout, "nick = %s\n", irc_cfg.nick);
		fprintf (stdout, "username = %s\n", irc_cfg.username);
		fprintf (stdout, "realname = %s\n", irc_cfg.realname);
		fprintf (stdout, "server_connect_msg = %s\n", irc_cfg.server_connect_msg);
		fprintf (stdout, "server_connect_nick = %s\n", irc_cfg.server_connect_nick);
		fprintf (stdout, "server_connect_delay = %s\n", irc_cfg.server_connect_delay);
		fprintf (stdout, "channel_connect_msg = %s\n", irc_cfg.channel_connect_msg);
		fprintf (stdout, "channel_connect_nick = %s\n", irc_cfg.channel_connect_nick);
		fprintf (stdout, "channel_connect_delay = %s\n", irc_cfg.channel_connect_delay);
		fprintf (stdout, "quizbot_nick = %s\n", irc_cfg.quizbot_nick);
	}
	fprintf (stdout, "IRC: Bot initilising\n");

	memset (&callbacks, 0, sizeof(callbacks));

	callbacks.event_connect = event_connect;
	callbacks.event_numeric = event_numeric;
	callbacks.event_privmsg = event_privmsg;
	callbacks.event_channel = event_channel;
	
	session = irc_create_session(&callbacks);

	if (!session)
	{
		fprintf (stderr, "IRC: Error setting up session\n");
		return 1;
	}
	
	irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);

	if (verbose)
	{
		fprintf (stdout, "IRC: Attempting to connect to server %s:%i channel %s with nick %s\n", 
				irc_cfg.server, atoi(irc_cfg.port), irc_cfg.channel, irc_cfg.nick);
	}

	if (irc_connect (session, irc_cfg.server, atoi(irc_cfg.port), 0, irc_cfg.nick, irc_cfg.username, irc_cfg.realname ))
	{
		fprintf (stderr, "IRC: ERROR %s\n", irc_strerror(irc_errno (session)));
	}

	//Enter main loop
	if (irc_run (session))
	{
		fprintf (stderr, "IRC: ERROR %s\n", irc_strerror(irc_errno (session)));
		return 1;
	}
	return 0;
}
