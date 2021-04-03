#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h> 
#include <ctype.h>
#include <signal.h>
#include <sys/prctl.h>
#include "inf141077_*.h"

int l,l2,userID;;

Message msg;

int checkServer(){
    msg.mtyp = SERVER_ONLINE;
    strcpy(msg.mtext,"zapytanie\0");
    msgsnd(l,&msg,MAX,0);
    msgrcv(l2,&msg,MAX,SERVER_ONLINE,0);
    if(strcmp(msg.mtext,"online\0")==0) return 1;
    else return 0;
}

int login()
{
    char username[20]; for(int i =0; i < 20; i++) username[i] = '\0';
    char password[20]; for(int i =0; i < 20; i++) password[i] = '\0';
    printf("Podaj nazwe uzytkownika: ");
    scanf("%s", username);
    msg.mtyp = LOGIN_REQUEST;
    strcpy(msg.mtext,username);
    msgsnd(l,&msg,MAX,0);
    msgrcv(l2,&msg,MAX,LOGIN_RESPONSE,0);
    printf("Podaj haslo: ");
    scanf("%s", password);
    strcpy(msg.mtext,password);
    msg.mtyp = LOGIN_REQUEST_2;
    msgsnd(l,&msg,MAX,0);
    msgrcv(l2,&msg,MAX,LOGIN_RESPONSE_2,0);
    if(*msg.mtext == '0')
    {
        printf("Nie udalo sie zalogowac\n");
    }
    else 
    {
        printf("Udalo sie zalogowac!\n");
    }
    int y = (int)msg.mtext[0] - 48;
    return y;
}

int Logout(){
    msg.mtyp = LOGOUT_REQUEST;
    //printf("UserID: %d",userID);
    sprintf(msg.mtext, "%d", userID);
    msgsnd(l,&msg,MAX,0);
    msgrcv(l2,&msg,MAX,LOGOUT_RESPONSE,0);
    if(strcmp(msg.mtext,"logout\0")==0) return 1;
    else return 0;
}

void getUserList(){
    msg.mtyp = USER_LIST;
    strcpy(msg.mtext,"zapytanie\0"); 
    msgsnd(l,&msg,MAX,0);
    while(msgrcv(l2,&msg,MAX,USER_LIST,0)<0){};
    printf("%s",msg.mtext);
}

void getUsersInGroup(){
    char groupname[20]; for(int i = 0; i < 20; i++) groupname[i] = '\0';
    msg.mtyp = USERS_IN_GROUP;
    printf("Podaj nazwe grupy do sprawdzenia jej czlonkow:");
    scanf("%s",groupname);
    strcpy(msg.mtext,groupname); 
    msgsnd(l,&msg,MAX,0);
    while(msgrcv(l2,&msg,MAX,USERS_IN_GROUP,0)<0){};
    printf("%s",msg.mtext);
}

void getGroupList()
{
    msg.mtyp = GROUP_LIST;
    strcpy(msg.mtext,"zapytanie2\0"); 
    msgsnd(l,&msg,MAX,0);
    while(msgrcv(l2,&msg,MAX,GROUP_LIST,0)<0){};
    printf("%s",msg.mtext);
}

void joinGroup(){
    char groupname[20]; for(int i =0; i < 20; i++) groupname[i] = '\0';
    printf("Podaj nazwe grupy do ktorej chcesz dolaczyc:\n");
    scanf("%s", groupname);
    msg.mtyp = JOIN_GROUP;
    msg.senderID = userID;
    strcpy(msg.mtext,groupname);
    msgsnd(l,&msg,MAX,0);
    if(msgrcv(l2,&msg,MAX,JOIN_GROUP,0)>0){
    printf("%s\n",msg.mtext);
    }
}

void leaveGroup(){
    char groupname[20]; for(int i =0; i < 20; i++) groupname[i] = '\0';
    printf("Podaj nazwe grupy z ktorej chcesz odejsc:\n");
    scanf("%s", groupname);
    msg.mtyp = REMOVE_FROM_GROUP;
    msg.senderID = userID;
    strcpy(msg.mtext,groupname);
    msgsnd(l,&msg,MAX,0);
    if(msgrcv(l2,&msg,MAX,REMOVE_FROM_GROUP,0)>0){
    printf("%s\n",msg.mtext);
    }
}

void sendPrivateMessage(){
    int lifesaver;
    char reciver[20];
    char message[MAX]; for(int i = 0; i < MAX; i++) message[i] = '\0';
    msg.mtyp = SEND_MESSAGE;
    msg.senderID = (long) userID;
    printf("Podaj nazwe uzytkownika do ktorego chcesz wyslac wiadomosc:\n");
    scanf("%s", reciver);
    strcpy(msg.reciver,reciver);
    msgsnd(l,&msg,MAX,0);
    msgrcv(l2,&msg,MAX,SEND_MESSAGE,0);
    //printf("%s\n",msg.mtext);
    if(strcmp(msg.mtext,"doszlo\0")==0){
        printf("Napisz swoja wiadomosc (200 znakow):\n");
        scanf("%d",&lifesaver);
        fgets(message, MAX, stdin);
        strcpy(msg.mtext,message);
        printf("Wyslano wiadomosc do %s.\n",msg.reciver);
        msg.mtyp = SEND_MESSAGE_2;
        msg.senderID = (long) userID;
        msgsnd(l,&msg,MAX,0);
        msgrcv(l2,&msg,MAX,SEND_MESSAGE_2,0);
        printf("%s.\n",msg.mtext);
    }
    else{
        printf("Nie ma takiego uzytkownika.\n");
    }
}

void sendGroupMessage(){
    int lifesaver;
    char reciver[20];
    char message[MAX]; for(int i = 0; i < MAX; i++) message[i] = '\0';
    msg.mtyp = SEND_GROUP_MESSAGE;
    msg.senderID = (long) userID;
    printf("Podaj nazwe grupy do ktorej chcesz wyslac wiadomosc:\n");
    scanf("%s", reciver);
    strcpy(msg.reciver,reciver);
    msgsnd(l,&msg,MAX,0);
    msgrcv(l2,&msg,MAX,SEND_GROUP_MESSAGE,0);
    //printf("%s\n",msg.mtext);
    if(strcmp(msg.mtext,"doszlo\0")==0){
        printf("Napisz swoja wiadomosc (200 znakow):\n");
        scanf("%d",&lifesaver);
        fgets(message, MAX, stdin);
        strcpy(msg.mtext,message);
        printf("Wyslano wiadomosc do %s.\n",msg.reciver);
        msg.mtyp = SEND_GROUP_MESSAGE_2;
        msg.senderID = (long) userID;
        msgsnd(l,&msg,MAX,0);
        msgrcv(l2,&msg,MAX,SEND_GROUP_MESSAGE_2,0);
        printf("%s.\n",msg.mtext);
    }
    else if(strcmp(msg.mtext,"notamember\0")){
        printf("Nie mozna wyslac wiadomosci, nie jestes jej czlonkiem.\n");
    }
    else{
        printf("Nie mozna wyslać wiadomosci\n");
    }

}

void recieveMessage()
{
    if(msgrcv(l2,&msg,MAX,(long) userID,IPC_NOWAIT)>0){
    printf("Odebrano wiadomosc od %s %s",msg.sender,msg.mtext);
    }
}


int main (int argc, char *argv[]) {
    key_t klucz = 123;
    key_t klucz2 = 321;
    l = msgget(klucz, 0);
    l2 = msgget(klucz2, 0);

    if(checkServer()){ 
    printf("Server online!\n");

    int invalidInput;
    int loggedIn = 0;
    int runLogMenu = 1;
    int choice = 0;


    while(runLogMenu && !loggedIn){
        fflush(stdin);
        invalidInput = 0;
        printf("1.Zaloguj sie.\n2.Wyjdz.\n"); 
        while ((choice = getchar()) != '\n' && choice != EOF){
        choice = choice - 48;
        if(!invalidInput){
            switch(choice)
            {
                case 1:
                    userID = login();
                    //printf("Moje id: %d\n",userID);
                    if(userID > 0) loggedIn = 1;
                    break;
                case 2:
                    runLogMenu = 0;
                    //printf("Wykonuje case 2...\n");
                    break;
                default:
                    printf("Niepoprawna komenda.\n");
                    invalidInput = 1;
                    break;
            }
        }
        }
    }

    if(loggedIn) printf("Zalogowano uzytkownika test%d\n",userID);
    if(fork()==0)
    {
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        while(1){
            recieveMessage();            
        }
        exit(0);
    }
    while(runLogMenu && loggedIn){
        fflush(stdin);
        invalidInput = 0;
        
        printf("1.Wyswietl liste uzytkownikow.\n");
        printf("2.Wyswietl dostepne grupy.\n");
        printf("3.Wyswietl czlonkow grupy.\n"); 
        printf("4.Wyslij wiadomosc do uzytkownika.\n");
        printf("5.Wyslij wiadomosc grupie.\n"); 
        printf("6.Dolacz do grupy.\n");
        printf("7.Odejdz z grupy.\n");  
        printf("8.Wyloguj.\n\n"); 
        while ((choice = getchar()) != '\n' && choice != EOF){
        choice = choice - 48;
        if(!invalidInput){
            switch(choice)
            {
                case 1:
                    getUserList();
                    break;
                case 2:
                    getGroupList();               
                    break;
                case 3:
                    getUsersInGroup();   
                    break;
                case 4:
                    sendPrivateMessage();
                    break;
                case 5:
                    sendGroupMessage();
                    break;
                case 6:
                    joinGroup();
                    break;
                case 7:
                    leaveGroup();
                    break;
                case 8:
                    if(Logout()){ 
                        loggedIn = 0;
                        printf("Wylogowywanie...\n");
                    }
                    else printf("Nie udalo sie wylogowac\n");
                    break;
                default:
                    printf("Niepoprawna komenda.\n");
                    invalidInput = 1;
                    break;
            }
        }
        }
    }
    } else printf("Server offline!\n");
	return 0;
}