#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define T_MSG 20

typedef struct {
    long msgtype;
    int sender;
    char text[512];
} msgbuf;

typedef struct {
    long msgtype;
    char timestamp[512];
} msgbuf_ack;

void get_input(char* text, char** buf, size_t* size) {
    fputs(text, stdout);

    if (getline(buf, size, stdin) == -1) {
        perror("getline");
        exit(1);
    }
}

int get_id(char* text) {
    char* input = NULL;
    size_t len;

    get_input(text, &input, &len);

    int id = (int) strtol(input, NULL, 10);
    free(input);
    return id;
}


int main() {
    int pid;
	int user_id = get_id("User ID: ");
	int receiver_id = get_id("Receiver ID: ");

    
    // TODO: create key and message queue
    key_t key;
    int qid;

    key = ftok(".", 'a');
    qid = msgget(key, IPC_CREAT|IPC_NOWAIT|0666);
    if(qid==-1){
	    perror("msgget error: ");
	    exit(0);
    }

    switch (pid = fork()) {
        case -1:
            perror("fork");
            exit(1);
        case 0:
            // The child is the receiver
            while (1) {
                msgbuf buf;
                msgbuf_ack read_tim;

                // TODO: Receive a message with IPC_NOWAIT
                if ((msgrcv(qid, (void*)buf, sizeof(buf), user_id, 0) > 0)) {
                    buf.sender = receiver_id;
			
		    msgbuf_ack ack;
                    ack.msgtype = T_MSG + receiver_id; // you can remove this if you do not need this
                    printf("\33[2K\rUser %d:\t%s", buf.sender, buf.text); // This line erases the current line and prints the received message
                    printf("User %d:\t", user_id);
                    fflush(stdout);

                    // TODO: Send an ack message (current timestamp) to the sender
		    time_t t = time(NULL);
		    struct tm tm = *localtime(&t);
		    strftime(ack.timestamp, sizeof(ack.timestamp), "%Y-%m-%d %H:%M:%S", &tm);

		    if((msgsnd(qid, (void*)ack, sizeof(ack), 0) < 0)){
			    perror("msgsnd error: ");
			    exit(1);
		    }
                }
            
                // TODO: Receive ack message using IPC_NOWAIT and store the messeg in `read_time`
                if ((msgrcv(qid, (void*)read_tim, sizeof(read_tim), T_MSG+user_id, 0)) > 0) {
                    // NOTE: The printf statement below clears the current line and then sends a message
                    // This printf only works properly when the sender and receiver are running at the same itme
                    // Otherwise, it will clear the input from the user's perspective, but not the actual stdin
		    
                    printf("\33[2K\rUser %d read message at %s\n", receiver_id, read_tim.timestamp);
                    printf("User %d:\t", user_id);
                    fflush(stdout);
                }           
            }		
            break;
        default:
            // The parent is the sender
            while (1) {
                /** TODO:
                 * 1. Get regular message from stdin (implemented)
                 * 2. If the message is "quit", exit both parent and child (use SIGINT)
                 * 3. Send normal message
                 */

                msgbuf buf;
                buf.msgtype = receiver_id;
                char* line = NULL;
                size_t len = 0;
                printf("User %d:\t", user_id);
                get_input("", &line, &len);
                // Your code

		if(strcmp(line, "quit\n")==0){
			kill(pid, SIGINT);
			
			free(line);
			pid=getpid();
			kill(pid, SIGINT);
		}
		else{
			strncpy(buf.text, line, len);
			if((msgsnd(pid, (void*)buf, sizeof(buf), 0))==-1){
				perror("msgsnd error: ");
				exit(1);
			}
		}

                free(line);
            }
            break;
    }

	return 0;
}
