#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFSIZE 1024 //Taille du buffer
#define PORT "69" //Port ouvert sur le serveur
#define MAXDATA 1024 //Taille maximale d'un packet

int main(int argc, char* argv[]){

// Les variables
	struct addrinfo info;
	struct addrinfo *infoserveur;
	struct addrinfo *infoserveur2;
	int so;
	socklen_t slen;
	int sockfd;
	int lenmess;
	char *buf;
	struct sockaddr_in addr;

// Vérification du nombre d'arguments
	if(argc != 4 && argc != 6){

		perror("Commande invalide \n");
		exit(EXIT_FAILURE);

	}

// Récupération des arguments
	char *serveur = argv[2];// adresse IP du serveur
	char *fichier = argv[3]; // nom du fichier
	//char *option = argv[4]; // Option blocksize
	//char *valeurblocksize = argv[5]; // Valeur du blocksize
	char *mode = "octet";
// Configuration du client UDP
	memset(&info,0,sizeof(info)); // écrire à l'addr 0 le contenuu de info 
	info.ai_family = AF_UNSPEC;
	info.ai_socktype = SOCK_DGRAM; // Mode datagramme
	if((so = getaddrinfo(serveur, PORT, &info, &infoserveur)) !=0) {

		perror("Client : Erreur 'Getaddrinfo' \n");
		exit(EXIT_FAILURE);
	}
// Réserve socket
	for(infoserveur2 = infoserveur; infoserveur2 != NULL; infoserveur2 = infoserveur2->ai_next) {
		if ((sockfd = socket(infoserveur2->ai_family, infoserveur2->ai_socktype, infoserveur2->ai_protocol)) == -1){
			perror("Client : Erreur 'socket' \n");
			continue;
		}
		break;
	}
	if(infoserveur2 == NULL){
		fprintf(stderr, "Client : Impossible de réserver unsocket \n");
		return 2;
	}

// gettftp
	if(strcmp(argv[1],"gettftp")==0){
//======== Construction d'une requête en lecture RRQ et envoi au serveur ========

//Requête en lecture RRQ
		char *RRQ;
		int len;/*
		if (argc==6) {
			if(strcmp(option,"-blocksize")==0) {
				len=strlen(fichier)+strlen(mode)+4+2+strlen("blocksize")+strlen(valeurblocksize);
				}
		}*/
	len = 2 + strlen(fichier)+1+strlen(mode)+1;
	RRQ = malloc(len);
	RRQ[0]=0;RRQ[1]=1; //OPcode
	strcpy(RRQ+2,fichier);
	RRQ[2+strlen(fichier)]=0; // On met des 0 entre chaque argument
	strcpy(RRQ+3+strlen(fichier),mode);
	/*RRQ[3+strlen(fichier)+strlen(mode)]=0;
	if (argc==6) {
		if(strcmp(option,"-blocksize")==0) {
			strcpy(RRQ+4+strlen(fichier),"blocksize");
			RRQ[4+strlen(fichier)+strlen("blocksize")]=0;
			strcpy(RRQ+5+strlen(fichier)+strlen("blocksize"),valeurblocksize);
			strcpy(RRQ+5+strlen(fichier)+strlen("blocksize"),valeurblocksize);
			RRQ[5+strlen(fichier)+strlen("blocksize")+strlen(valeurblocksize)]=0;
		}
	}
*/
//Envoi de la requête en lecture RRQ au serveur
	if((lenmess=sendto(sockfd,RRQ,len,0,(struct sockaddr *) infoserveur2->ai_addr,infoserveur2->ai_addrlen))== -1){

		perror("Client : Erreur 'sendto RRQ' \n");
		exit(EXIT_FAILURE);
	}
	printf("CLIENT: RRQ de : %d octets envoyé au serveur\n", lenmess);

//======== Reception d'un fichier ========

	int datarecu;
	FILE *fichiertelecharge = fopen(fichier,"wb"); // on pointe sur le fichier

//Création d'un fichier
	do{
		buf=malloc(BUFSIZE);
		if ((lenmess = recvfrom(sockfd, buf, BUFSIZE , 0,(struct sockaddr *) &addr, &slen))==-1) {
			perror("Client : Erreur 'recevfrom' \n");
			exit(EXIT_FAILURE);
		}
		write(STDOUT_FILENO,buf,lenmess);
		printf("CLIENT: Réception du paquet n° %d de : %d octets \n",*(buf+3),lenmess-4);

		buf[lenmess]='\0';
//Ecriture du fichier téléchargé
		datarecu=lenmess-4;
		fwrite(buf+4,sizeof(char),datarecu,fichiertelecharge);
//======== Acquittement ========
//Création de l'acquittement
		u_int16_t *ACK;
		len=4;
		ACK = malloc(len);
		*ACK=htons((u_int16_t) 4); //Opcode: htons: échanger l'ordre des octets d'un unsignedshort pour convertir addr IP d'un poste hôte en son équivalent réseau.
		*(ACK+1)=htons((u_int16_t) *(buf+3)); //Block
//Envoi de l'acquittement
		if((sendto(sockfd,ACK, len,0,(struct sockaddr *) &addr,slen))== -1){
			perror("Client : Erreur 'sendto ACK' \n");
			exit(EXIT_FAILURE);
		}
	printf("CLIENT: Acquittement du paquet n° %d \n",*(buf+3));

	} 
	while((datarecu==MAXDATA)); //Lorsque datarecu<1024 octets le transfert est terminé

		fclose(fichiertelecharge);
		printf("CLIENT: Transfert terminé\n");
	}
	
	
	// Code pour puttftp
	if(strcmp(argv[1],"puttftp")==0){

//======== Construction d'une requête en écriture WRQ et envoi au serveur ========

// requête en écriture WRQ
	char *WRQ;
	int len=strlen(fichier)+strlen(mode)+4;
	WRQ = malloc(len);
	WRQ[0]=0;WRQ[1]=2; //OPcode
	strcpy(WRQ+2,fichier);
	WRQ[2+strlen(fichier)]=0;
	strcpy(WRQ+3+strlen(fichier),mode);
	WRQ[3+strlen(fichier)+strlen(mode)]=0;
//Envoi de la requête 
	if((lenmess=sendto(sockfd,WRQ,len,0,(struct sockaddr *) infoserveur2->ai_addr,infoserveur2->ai_addrlen))== -1){
		perror("Client : Erreur 'sendto WRQ' \n");
		exit(EXIT_FAILURE);
	}
	printf("CLIENT: WRQ de : %d octets envoyé au serveur\n", lenmess);
//======== Attente de l'aquittement du serveur========

	buf=malloc(BUFSIZE);
	if ((lenmess = recvfrom(sockfd, buf, BUFSIZE , 0,(struct sockaddr *) &addr, &slen))==-1) {
		perror("Client : Erreur 'recevfrom' \n");
		exit(EXIT_FAILURE);
	}

	if(*(buf+1)==5) {
		perror("Client : Le serveur a retourné un message d'erreur \n");
		exit(EXIT_FAILURE);
	}
	printf("CLIENT: Aqquitement du WRQ de la part du serveur : %d%d|%d%d\n",*(buf),*(buf+1),*(buf+2),*(buf+3));

//======== Preparation du fichier à envoyer ========
	FILE *fichieraenvoyer = fopen(fichier,"rb");

//Ouverture du fichier à envoyer

	if(fichieraenvoyer == NULL){

		perror("CLIENT: Ouverture du fichier à envoyer impossible\n");
		exit(EXIT_FAILURE);
	}

//======== Envoi d'un fichier ========
	u_int16_t blocknumero = 1;
	char buffer[MAXDATA];
	size_t n = MAXDATA;

	do {

//Construction d'un paquet à envoyer
	n = fread(buffer,1,MAXDATA,fichieraenvoyer);
	buffer[n]='\0';
	char *packet;
	packet = malloc(n+4);
	packet[0]=0;packet[1]=3;
	packet[2]=0;packet[3]=blocknumero;
	strcpy(packet+4,buffer);
//Envoi du paquet
	if((lenmess=sendto(sockfd,packet, n+4,0,(struct sockaddr *) &addr,slen))== -1){

		perror("Client : Erreur 'sendto DATA' \n");

		exit(EXIT_FAILURE);
	}

	printf("CLIENT: Envoi du packet n° %d de %d octet de data\n",blocknumero,lenmess-4);

//======== Attente de l'aquittement du serveur========

	buf=malloc(BUFSIZE);
	if ((lenmess = recvfrom(sockfd, buf, BUFSIZE , 0,(struct sockaddr *) &addr, &slen))==-1) {
		perror("Client : Erreur 'recevfrom'\n");
		exit(EXIT_FAILURE);
	}

	printf("CLIENT: Aqquitement du packet %d de lapart du serveur : %d%d|%d%d\n",*(buf+3),*(buf),*(buf+1),*(buf+2),*(buf+3));

		blocknumero+=1;
	} 
	while (n == MAXDATA);
	fclose(fichieraenvoyer);
	printf("CLIENT: Transfert vers le serveur terminé\n");
	}
	return 0;
}
