#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h> 
#include <stdbool.h>
#include <signal.h>

#define noGroups 3
#define noUsers 9

key_t klucz = 123;
key_t klucz2 = 321;

//Zmienne do przechowania id kolejek
int k;
int k2;


struct User{
    char username[20];
    char password[20];
    bool loggedIn;
    int userID;
};

struct Group{
    struct User members[noUsers];
    char groupname[20];
    long groupID;
};

//Zmienna pomocnicza
struct User none;

struct Message rcvMsg;
struct User Users[noUsers];
struct Group Groups[noGroups];

//Funkcja do zaladowania uzytkownikow i przypisania im ID
void loadUsers(){
    FILE *file = fopen("users","r");
    int i=0;
    while(!feof(file)){
        fscanf(file,"%s",Users[i].username);
        fscanf(file,"%s",Users[i].password);
        Users[i].userID = i+1;
        Users[i].loggedIn=false;
        i++;
    }
    fclose(file);
}

//Funkcja do zaladowania grup
void loadGroups(){
    FILE *file = fopen("groups","r");
    int i=0;
    while(!feof(file)){
        fscanf(file,"%s",Groups[i].groupname);
        Groups[i].groupID = i+1;
        i++;
    }
    fclose(file);
}

//Dodaje uzytkownika do grupy
void addUserToGroup(){
    int valid = 0;
    int state = msgrcv(k,&rcvMsg,MAX,JOIN_GROUP,IPC_NOWAIT);
    if(state>0){
    char groupname[20]; for (int i = 0; i < 20; i++) groupname[i] = '\0';
    printf("Przyslana nazwa grupy: %s\n",rcvMsg.mtext);
    printf("Sender ID:%ld\n",rcvMsg.senderID);
    strcpy(groupname,rcvMsg.mtext);
    for(int i = 0; i < noGroups; i ++)
    {
        strcpy(rcvMsg.mtext,"Nie ma takiej grupy\0");
        if(strcmp(groupname,Groups[i].groupname)==0){
            valid = 1;
            for(int j = 0; j < noUsers; j++){
                if(rcvMsg.senderID==Groups[i].members[j].userID){
                    valid = 0;
                    strcpy(rcvMsg.mtext,"Nalezysz juz do tej grupy\0");
                    break;
                }
            }
            if(valid){
            for(int j = 0; j < noUsers; j++){
                int num = (int) (rcvMsg.senderID-1);
                //printf("numer w tablicy: %d\n",num);
                if(Groups[i].members[j].userID == none.userID){
                     Groups[i].members[j] = Users[num];
                     break;
                }
            }
             strcpy(rcvMsg.mtext,"Dodano do grupy\0");
            }
            break;
        }
    }
    rcvMsg.mtyp = JOIN_GROUP;
    msgsnd(k2,&rcvMsg,MAX,0);
    //msgrcv(k,&rcvMsg,MAX,JOIN_GROUP,0);
    }
}

//Usuwa uzytkownika z grupy
void removeUserFromGroup(){
    int userInGroup = 0;
    int state = msgrcv(k,&rcvMsg,MAX,REMOVE_FROM_GROUP,IPC_NOWAIT);
    if(state>0){
    char groupname[20]; for (int i = 0; i < 20; i++) groupname[i] = '\0';
    printf("Przyslana nazwa grupy: %s\n",rcvMsg.mtext);
    printf("Sender ID:%ld\n",rcvMsg.senderID);
    strcpy(groupname,rcvMsg.mtext);
    for(int i = 0; i < noGroups; i ++)
    {
        strcpy(rcvMsg.mtext,"Nie ma takiej grupy\0");
        if(strcmp(groupname,Groups[i].groupname)==0){
            for(int j = 0; j < noUsers; j++){
                if(rcvMsg.senderID==Groups[i].members[j].userID){
                    userInGroup = 1;
                    Groups[i].members[j] = none;
                    strcpy(rcvMsg.mtext,"Usunieto z grupy\0");
                    break;
                }
            }
            if(!userInGroup){            
             strcpy(rcvMsg.mtext,"Nie należysz do tej grupy\0");
            }
            break;
        }
    }
    rcvMsg.mtyp = REMOVE_FROM_GROUP;
    msgsnd(k2,&rcvMsg,MAX,0);
    }
}

//Funkcja przekierowująca wiadomości prywatne
void redirectMessage(){
    char message[MAX];
    strcpy(message,"\n");
    long recieverID = 0;
    bool userExists = false;
    if(msgrcv(k,&rcvMsg,MAX,SEND_MESSAGE,IPC_NOWAIT)>0){
        printf("sprawdzanie uzytkownika %s\n",rcvMsg.reciver);
        for(int i =0; i < noUsers; i++){
            if(strcmp(rcvMsg.reciver,Users[i].username)==0){
                recieverID = (long) (i + 1);
                userExists = true;
                break;
            }
        }
        rcvMsg.mtyp = SEND_MESSAGE;
        printf("Long sender = %ld\n",rcvMsg.senderID);
        printf("Long reciver = %ld\n",recieverID);
        if(userExists) {
            for(int i = 0; i < noUsers; i++){
                if(Users[i].userID == rcvMsg.senderID){
                    strcpy(rcvMsg.sender,Users[i].username);
                    break;
                }
            }
            printf("Wiadomosc od %s\n",rcvMsg.sender);
            strcpy(rcvMsg.mtext,"doszlo\0");
            msgsnd(k2,&rcvMsg,MAX,0);
            rcvMsg.mtyp = recieverID;
            if(msgrcv(k,&rcvMsg,MAX,SEND_MESSAGE_2,0)>0){
            strcat(message,rcvMsg.mtext);
            strcpy(rcvMsg.mtext,message);
            printf("Tresc wiadomosci:%s",rcvMsg.mtext);
            rcvMsg.mtyp = recieverID;
            msgsnd(k2,&rcvMsg,MAX,0);
            strcpy(rcvMsg.mtext,"Dostarczono wiadomosc\0");
            rcvMsg.mtyp = SEND_MESSAGE_2;
            msgsnd(k2,&rcvMsg,MAX,0);
            }
        }
        else{ strcpy(rcvMsg.mtext,"niedoszlo\0");
        msgsnd(k2,&rcvMsg,MAX,0);
        }
    }
}

//Funkcja przekierowująca wiadomosci w grupie
void redirectGroupMessage(){
    char message[MAX];
    int isMember = 0;
    int groupID = -1;
    bool groupExists = false;
    if(msgrcv(k,&rcvMsg,MAX,SEND_GROUP_MESSAGE,IPC_NOWAIT)>0){
        printf("sprawdzanie grupy %s\n",rcvMsg.reciver);
        for(int i =0; i < noGroups; i++){
            if(strcmp(rcvMsg.reciver,Groups[i].groupname)==0){
                groupID =  i;
                groupExists = true;
                strcpy(message,"z grupy \0");
                strcat(message,Groups[i].groupname);
                strcat(message,":\n");
                break;
            }
        }
        rcvMsg.mtyp = SEND_GROUP_MESSAGE;
        printf("Long sender = %ld\n",rcvMsg.senderID);
        printf("Groupid = %d\n",groupID);
        for(int j = 0; j < noGroups; j++){
            for(int i = 0;i < noUsers; i++)
                if(rcvMsg.senderID==Groups[j].members[i].userID){
                    printf("Uzyttkownik %s nalezy do grupy\n",Groups[j].members[i].username);
                    isMember = 1;
                    break;
                }
            }
        if(groupExists && isMember) {
            
            printf("Wiadomosc od %s\n",rcvMsg.sender);
            strcpy(rcvMsg.mtext,"doszlo\0");
            msgsnd(k2,&rcvMsg,MAX,0);
            //rcvMsg.mtyp = recieverID;
            if(msgrcv(k,&rcvMsg,MAX,SEND_GROUP_MESSAGE_2,0)>0){
            printf("Tresc wiadomosci:%s",rcvMsg.mtext);
            strcat(message,rcvMsg.mtext);
            strcpy(rcvMsg.mtext,message);
            for(int i = 0; i < noUsers; i++)
            {
                if(Groups[groupID].members[i].userID != none.userID && Groups[groupID].members[i].userID !=rcvMsg.senderID){
                rcvMsg.mtyp = (long) Groups[groupID].members[i].userID;
                printf("Wysylam wiadomosc do uzytkownika %s w grupie %s\n",Groups[groupID].members[i].username, Groups[groupID].groupname);
                msgsnd(k2,&rcvMsg,MAX,0);
                }
            }
            strcpy(rcvMsg.mtext,"Dostarczono wiadomosc\0");
            rcvMsg.mtyp = SEND_GROUP_MESSAGE_2;
            msgsnd(k2,&rcvMsg,MAX,0);
            }
        }
        else if(!isMember){ strcpy(rcvMsg.mtext,"notamember\0");}
        else strcpy(rcvMsg.mtext,"niedoszlo\0");
        msgsnd(k2,&rcvMsg,MAX,0);
        }
    }


//Przeslanie uzytkownikowi listy uzytkownikow
void sendUserList()
{
    if(msgrcv(k,&rcvMsg,MAX,USER_LIST,IPC_NOWAIT)>0){
    strcpy(rcvMsg.mtext,"\0");
    strcat(rcvMsg.mtext,"Uzytkownicy online:\n");
    for(int i = 0; i < noUsers; i++){
        if(Users[i].loggedIn==true){
        strcat(rcvMsg.mtext,Users[i].username);   
        strcat(rcvMsg.mtext," jest ONLINE\n");
        }  
    }
    rcvMsg.mtyp = USER_LIST;
    msgsnd(k2,&rcvMsg,MAX,0);
    }
}

//Przeslanie listy grup
void sendGroupList()
{
    if(msgrcv(k,&rcvMsg,MAX,GROUP_LIST,IPC_NOWAIT)>0){
    strcpy(rcvMsg.mtext,"\0");
    strcat(rcvMsg.mtext,"Dostepne grupy:\n");
    for(int i = 0; i < noGroups; i++){
        strcat(rcvMsg.mtext,Groups[i].groupname);   
        strcat(rcvMsg.mtext,"\n"); 
    }
    rcvMsg.mtyp = GROUP_LIST;
    msgsnd(k2,&rcvMsg,MAX,0);
    }
}
//Przeslanie uzytkownikowi listy czlonkow grup
void sendUsersInGroup()
{
    if(msgrcv(k,&rcvMsg,MAX,USERS_IN_GROUP,IPC_NOWAIT)>0){
    int valid = 0;
    int members = 0;
    //printf("%s\n",rcvMsg.mtext);
    for(int i = 0; i < noGroups; i++){
        if(strcmp(rcvMsg.mtext,Groups[i].groupname)==0){
            valid = 1;
            strcpy(rcvMsg.mtext,"Czlonkowie grupy:\n");
            //printf("Istnieje taka grupa");
            for(int j = 0; j < noUsers; j++)
            {
                if(Groups[i].members[j].userID==none.userID);
                else{
                    members +=1;
                    strcat(rcvMsg.mtext,Groups[i].members[j].username);   
                    strcat(rcvMsg.mtext,"\n");
                }
            }
        }        
    }
    if(!valid) strcpy(rcvMsg.mtext,"Nie ma takiej grupy\n");
    else if(!members) strcpy(rcvMsg.mtext,"Brak członkow\n");
    rcvMsg.mtyp = USERS_IN_GROUP;
    msgsnd(k2,&rcvMsg,MAX,0);
    }
}


//Funkcja pomocnicza - DO USUNIECIA
void printUsers(){
    printf("--- Zarejestrowani uzytkownicy ---\n");
    for(int i = 0; i < noUsers; i++){
        printf("Nazwa:%s Haslo:%s ID:%d \n",Users[i].username,Users[i].password,Users[i].userID);
    }
    printf("--- --- --- --- ---\n");
    printf("--- Zarejestrowane grupy ---\n");
    for(int i = 0; i < noGroups; i++){
        printf("Grupa:%s ID:%ld \n",Groups[i].groupname,Groups[i].groupID);
    }
    printf("--- --- --- --- ---\n");
}

Message rcvMsg;
void onlineUsers();

//Logowanie uzytkownika
int checkUser(char* name,char* pass){
for(int i = 0; i < noUsers; i++)
{
    if(strcmp(name,Users[i].username)==0){
        printf("Istnieje taki uzytkownik!\n");
        if(strcmp(pass,Users[i].password)==0){
            if(Users[i].loggedIn==false){
            printf("Wprowadzono poprawne haslo!\n");
            printf("Zalogowano uzytkownika %s ID:%d \n",Users[i].username,Users[i].userID);
            Users[i].loggedIn = true;
            return Users[i].userID;
            }
            else{
                printf("Uzytkownik jest juz zalogowany\n");
                return 0;
            }
        }else printf("Wprowadzono niepoprawne haslo!\n");
    } else if(i == noUsers -1) printf("Nie ma takiego uzytkownika!\n");
}
    onlineUsers();
    return 0;
}

//funkcja pomocniczna
void onlineUsers()
{
    for(int i = 0; i < noUsers; i++)
    {
        if(Users[i].loggedIn) printf("User%d ONLINE\n",i);
        else printf("User%d OFFLINE\n",i);
    }
}

//Serwer online
void serverOnline()
{
    int state = msgrcv(k,&rcvMsg,MAX,SERVER_ONLINE,0);
    if(state > 0){
        rcvMsg.mtyp = SERVER_ONLINE;
        strcpy(rcvMsg.mtext,"online\0");
        msgsnd(k2,&rcvMsg,MAX,0);
    }
}

//Logowanie i wylogowywanie uzytkownikow
void logUsers()
{
struct Message loginMsg;
int state = msgrcv(k,&loginMsg,MAX,LOGIN_REQUEST,IPC_NOWAIT);
if(state>0){
int id;
char buff[3];
char username[20];
char password[20];
printf("Przyslana nazwa: %s\n",loginMsg.mtext);
strcpy(username,loginMsg.mtext);
loginMsg.mtyp = LOGIN_RESPONSE;
msgsnd(k2,&loginMsg,MAX,0);
msgrcv(k,&loginMsg,MAX,LOGIN_REQUEST_2,0);
strcpy(password,loginMsg.mtext);
printf("Przyslana haslo: %s\n",loginMsg.mtext);
id = (checkUser(username,password));
sprintf(buff, "%d", id);
strcpy(loginMsg.mtext,buff);
loginMsg.mtyp = LOGIN_RESPONSE_2;
msgsnd(k2,&loginMsg,MAX,0);
}
state = msgrcv(k,&rcvMsg,MAX,LOGOUT_REQUEST,IPC_NOWAIT);
    if(state > 0){
        long ret;
        char* ptr;
        ret =strtol(rcvMsg.mtext,&ptr,10);
        int num = (int) ret;
        num = (num % 10)-1;
        Users[num].loggedIn = false;      
        rcvMsg.mtyp = LOGOUT_RESPONSE;
        strcpy(rcvMsg.mtext,"logout\0");
        msgsnd(k2,&rcvMsg,MAX,0);
        strcpy(rcvMsg.mtext,"logout\0");
        msgsnd(k2,&rcvMsg,MAX,0);
    }
}

int main(int argc, char *argv[]){

    k = msgget(klucz, IPC_CREAT|0644);
    k2 = msgget(klucz2, IPC_CREAT|0644);  

    loadGroups();
    loadUsers();
    printUsers();
    if(fork()==0){
        while(true){
        serverOnline();
        }
    }
    if(fork()==0){
        while(true){
        logUsers();
        sendUserList();
        sendGroupList();
        addUserToGroup();
        sendUsersInGroup();
        removeUserFromGroup();
        redirectGroupMessage();
        }
    }

      if(fork()==0)
    {
       while(true){
        redirectMessage();
        }
    }
          if(fork()==0)
    {
       while(true){
        redirectMessage();
        }
    }
          if(fork()==0)
    {
       while(true){
        redirectMessage();
        }
    }
    wait(NULL);
    msgctl(k,IPC_RMID,0);
    msgctl(k2,IPC_RMID,0);
}