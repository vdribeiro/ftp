#include "args.h"

int porta = 34500;

void interrupt(){
	exit(0);
}

//SOCKETS ---
//inicia ligacao e devolve socket de controlo
int control_connect(char *host_name, int port)
{

	int socketc;            
	struct sockaddr_in name;
	struct hostent *serverinfo;
	char *serverIP;
	
	serverinfo = gethostbyname(host_name); //converte o nome do servidor em IP.
	if(NULL==serverinfo)
	{
		herror("Error obtaining IP\n");
		return -1;
	}
	
	serverIP = inet_ntoa(*((struct in_addr *) serverinfo->h_addr));
	//converte o nome do servidor em IP.
	printf("Server IP: %s\n", serverIP);

	bzero((char*)&name,sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	//name.sin_addr.s_addr = inet_addr(host_name);
	name.sin_addr.s_addr = inet_addr(serverIP);
	
	if ((socketc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Errno");
		return -1;
	}
		
	//fcntl(socketc, F_SETFL, O_NONBLOCK); 
	//nao interessa pk precisamos da ligação pra começar a utilizar ftp, logo bloqueia

	if ( (connect(socketc, (struct sockaddr *)&name, sizeof(name))) < 0 ) {

		perror("Errno");
		shutdown(socketc,SHUT_RDWR);
		close(socketc);
		return -1;
	}
	else {
		
		//printf("FTP connection established on: %s\n", host_name);
	}
	return socketc;
}

//obtem resposta do servidor
int serverMessage (int socket, char* message)
{
	int  i, return_code, ret;
	char response_code[4] = "000", msg[BUFFER], *ptr;

	//fica a espera e assume que existe pelo menos uma mensagem

	//do {
	//init:
	//sprintf(message,"");
	//bzero(message, strlen(message));
	
	//reset strings
	/*for (i = 0; i < BUFFER; i++) {
		msg[i]='\0';
	}*/
	bzero(msg, strlen(msg));
	bzero(message, strlen(message));
	
	//printf("Obtaining Response\n");
	ret = recv(socket, msg, sizeof(char) * BUFFER, O_NONBLOCK);
	
	ptr = strchr(msg,'\n');
	//printf ("found at %d\n",ptr-msg+1);
	message=strncpy(message, msg, ptr-msg+1);
	//printf("%s\n",message);
	
	if ( ret <= 0) {
		perror("Errno");
		return -1;
	}
	
	else {
		strncpy(response_code, message, 3);
		return_code = atoi(response_code);
		
		if (return_code!=500) {
			for (i = 0; i < ret; i++)
				printf("%c", message[i]);
				
		} else {
			return_code=1;
		}
		
		
		//continua se a mensagem tiver varias linhas     
		


		if (message[3] == '-') {
			do {
				ret = recv(socket, message, sizeof(char) * BUFFER, O_NONBLOCK);
				if ( ret < 0) {
					perror("Errno");
				}
				
				//message[ret] = NULL;
				//printf("%s", message);
				if (return_code!=500)
					for (i = 0; i < ret; i++)
						printf("%c", message[i]);
				printf("\n");
			} while (strstr(message, response_code) == NULL);
		}

	}
	//} while (ioctl(socket, I_PEEK) == TRUE);
	
	/*while (ioctl(socket, I_PEEK) == TRUE) {
		i = recv(socket, msg, sizeof(char) * BUFFER, O_NONBLOCK);
	}*/
	
	//retorna o codigo do servidor

	return return_code;
}

//cria e devolve socket de dados
int datasocket_create_PASV(int socketc){

	int socketd, rt;
	char bufas[BUFFER], buf2[BUFFER], address[BUFFER];
	char* aux;
	int serverport, code;
	 
	struct sockaddr_in remoteserver;  
	bzero((char*)&remoteserver,sizeof(remoteserver));

	if ((socketd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Errno");
		return -1;
	}
	
	//vamos requisitar a porta em espera (modo passivo) do servidor
	//este modo é mais recente e seguro do que a utilização de PORT

	//serverMessage(socket, unused);
	if((send(socketc, "PASV\r\n", 6, 0)) < 0)
	{
		perror("Errno");
		return -1;
	}

	/*do {
		code = serverMessage(socketc, buf2);
	 } while ((code!=227) || (code==500));*/
	
	//da tempo de resposta
	sleep(1);
	
	rt=0;
	do {
		//tenta ligar com o servidor "rt" vezes antes de fazer timeout
		rt++;
		code = serverMessage(socketc, buf2);
		
		if(code != 227)
		{	
			if (rt==3) {
				printf("Code received: %i is not 227 for PASV acknowledge!\n", code);		
				printf("Could not enter passive mode!\n");
				return -1;
			}
				
			printf("Out of Sync! Retrying...\n");
			while (ioctl(socketc, I_PEEK) == TRUE) {
				code = recv(socketc, bufas, sizeof(char) * BUFFER, O_NONBLOCK);
			}
		
			send(socketc, "PASV\r\n", 6, 0);
			
			sleep(1);
		} else {
			rt=3;
		}
		
	} while (rt!=3);

	//retiramos o resto da mensagem ate "a,a,a,a,p,p)"
	aux = strtok (buf2,"(");

	//concatenamos a address cada cifra do ip
	aux = strtok (NULL, ","),	sprintf(address,"%s.",aux);
	aux = strtok (NULL, ","),	strcat(address, aux); strcat(address, ".");
	aux = strtok (NULL, ",");	strcat(address, aux); strcat(address, ".");
	aux = strtok (NULL, ",");	strcat(address, aux);

	aux = strtok (NULL, ",");	serverport = 256 * atoi(aux);
	aux = strtok (NULL, ",");	serverport += atoi(aux);


	//printf("Passive mode for %s, waiting on port %i",address, serverport);


	//iniciar ligação tcp caso não haja erros


	//fcntl(socketc, F_SETFL, O_NONBLOCK); 
	//nao interessa pk precisamos da ligação pra começar a utilizar ftp, logo bloqueia

	remoteserver.sin_family = AF_INET;
	remoteserver.sin_port = htons(serverport);
	remoteserver.sin_addr.s_addr = inet_addr(address);
	

	if ( (connect(socketd, (struct sockaddr *)&remoteserver, sizeof(remoteserver))) < 0 ) {
		printf("ligação nao feita, nao chegou ao receive nem aos comandos pos-criação do datasocket\n");
		perror("Errno");
		shutdown(socketc,SHUT_RDWR);
		close(socketc);
		return -1;
	}


	//caso seja necessário definir modo de transmissão, alterar abaixo
	/*	
	send(socketc, "TYPE I\r\n", 8, 0);
	// if serverMessage != codigo then error
	send(socketc, "MODE S\r\n", 8, 0);
	// if serverMessage != codigo then error
	*/


	return socketd;
}


//cria e devolve socket de dados em modo activo

int datasocket_create_PORT(int socketc){

	int socketd, socketd2, local, addrlen;
	char buf[BUFFER], unused[BUFFER];
	struct hostent *localinfo;
	struct sockaddr_in name;


	//criamos um socket tcp para a ligaçao de dados
	socketd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (socketd == -1) {
		perror("Errno");
		printf("Erro ao criar socket.");
	}

	/*
	Utilizamos a struct hostent para extrair o ip local
	outbound, ou seja, para comunicações externas (de modo
	a ir buscar o ip do dispositivo de rede que comunica
	com o servidor externo por tcp, ao invés, por exemplo, de um
	dispositivo de bridge local ou duma placa de rede local)
	*/
	if (gethostname(buf, BUFFER) == -1) {
    		perror("Errno");
    		return -1;
 	}
	printf("\ngethostname is: %s\n", buf);

	localinfo = gethostbyname(buf);

	printf("local: %s", localinfo->h_addr_list[0]);

	local = *((int *)(localinfo->h_addr_list[0]));
	name.sin_family = localinfo->h_addrtype;

	/*
	Manter as portas da ligação de dados diferentes,
	num intervalo que não deverá estar reservado para 
	aplicações conhecidas.Além disso, não volta à porta
	imediatamente anterior,	de modo a não colidir com
	portas já em uso pela aplicação (para o caso de
	utilização de threads com múltiplas tarefas ftp).
	*/
	if(porta>36800) 
		porta = 34500;
	else 
		porta++;

	name.sin_port = htons(porta);
	name.sin_addr.s_addr = *((int *)(localinfo->h_addr_list[0]));

    	//associa-se o socket a porta definida para dados
	if(bind(socketd, (struct sockaddr *) &name, sizeof(name)) == -1) {
		perror("Errno");
		shutdown(socketd,SHUT_RDWR);
		close(socketd);
		return -1; 
	}
	/*
	Através de operações lógicas à varável que armazena o host
	local, procede-mos à criação da string a ser enviada, com 
	o comando PORT, e a sequência separada por vírgulas do IP
	local e da porta em utilização.
	*/
	sprintf(buf, "PORT %d,%d,%d,%d,%d,%d\r\n",
			(local & 0xFF000000) >> 24,
			(local & 0x00FF0000) >> 16,
			(local & 0x0000FF00) >> 8,
			(local & 0x000000FF),
			(name.sin_port & 0xFF00) >> 8,
			(name.sin_port & 0x00FF));

	printf("%s\n\n", buf);
	if((send(socketc, buf, strlen(buf), 0)) < 0)
	{
		perror("Errno");
		return -1;
	}

	int code = serverMessage(socketc, unused) ;
	printf("CODE: %i\n", code);
	if( code != 227)
	{
		printf("Code received: %i is not 227 for PASV acknowledge!\n", code);		
		printf("Could not enter passive mode!\n");
		perror("Errno");
	}
	
	listen(socketd, BUFFER * BUFFER);

	addrlen = sizeof(struct sockaddr_in);

	socketd2 = accept(socketd, (struct sockaddr *) &name, (socklen_t *) &addrlen);
	if (socketd2 == -1) {
		perror("Errno");
		return -1;
	}
	
	shutdown(socketd,SHUT_RDWR);
	close(socketd);

	return socketd2;
}

//COMANDOS ---
int cmd_ls(int socketc, int mode, int option, char* big){

	char lista[BUFFER], dummy[BUFFER];// big[BUFFER*10];

	int i, x, socketd;

	//cria data socket em modo passivo
	if(mode == 0){
		if( (socketd = datasocket_create_PASV(socketc)) < 0){
			printf("Data connection not established, exiting...\n");
			return -1;
		}
	}
	
	else{
		if( (socketd = datasocket_create_PORT(socketc)) < 0){
			printf("Data connection not established, exiting...\n");
			return -1;
		}
	}

	if (option == 1) {
		//envia comando NLST e recebe confirmação
		if ( send(socketc, "NLST\r\n", 6, 0) < 0) {
			perror("Errno");
			shutdown(socketd,SHUT_RDWR);
			close(socketd);	
			return -1;
		}
	
	}	
	else {
		//envia comando LIST e recebe confirmação
		if ( send(socketc, "LIST\r\n", 6, 0) < 0) {
			perror("Errno");
			shutdown(socketd,SHUT_RDWR);
			close(socketd);	
			return -1;
		}
	}

	serverMessage(socketc, dummy);

	
	//após envio do comando, começa a receber dados do LIST no datasocket, e imprime
	if (option!=2) {
		do {
			x = recv(socketd, lista, sizeof(char) * BUFFER, MSG_WAITALL);
			for (i = 0; i < x; i++) 
				printf("%c", lista[i]); 
		} while (x != 0);
	} else {
		i=0;
		do {
			printf("blocks: %i\n",i);
			x = recv(socketd, big, sizeof(char) * BUFFER * 10, MSG_WAITALL);
			i++;
		} while (x != 0);
	}
	
	shutdown(socketd,SHUT_RDWR);
	close(socketd);	

	return 1;
}

int cmd_get(int socketc, int mode, char* filepath){

	char data[BUFFER*2], dummy[BUFFER];
	int x, socketd;
	FILE *file;

	//cria data socket no modo activado previamente (ou default)
	if(mode == 0) {
		if( (socketd = datasocket_create_PASV(socketc)) < 0){
			printf("Data connection not established, exiting...\n");
			perror("Errno");
			return -1;
		} 
	} else {
		if( (socketd = datasocket_create_PORT(socketc)) < 0){
			printf("Data connection not established, exiting...\n");
			return -1;
		}
	}

	//envia comando RETR e recebe confirmação
	sprintf(dummy, "RETR %s\r\n", filepath);
	if ( send(socketc, dummy, sizeof(dummy), 0) < 0) {
		perror("Errno");
		shutdown(socketd,SHUT_RDWR);
		close(socketd);	
		return -1;
	}
	
	int returnval = serverMessage(socketc, dummy);
	
	if (returnval==550) {
		shutdown(socketd,SHUT_RDWR);
		close(socketd);	
		return -1;
	}
	
	//é necessário definir o nome do ficheiro local a partir do filepath
	//i.e. retirar eventuais directorias no cabeçalho do filepath, para guardar
	//o ficheiro na pasta do programa (ainda não implementado)

	/*
	filename ⁼ strtok(filepath, "");
	printf("filename\n");
	
	aux = filename;
	do{
		aux = strtok(NULL, "/");
		printf(aux);
	}
	while(strcmp(aux, ""));
	
	*/


	file = fopen(filepath, "w"); //retorna null pointer em erro

	if ( file == NULL ) {
		printf("Cannot open local file\n");
		//neste caso, deve ser pedida resposta ao servidor, que já estava a processar o RETR
		//e assim paramos a transferência do servidor
		send(socketc, "NOOP\r\n", 6, 0);  
		shutdown(socketd,SHUT_RDWR);
		close(socketd);

		return -1;
	} 

	if(returnval == 150){
		printf("Receiving data\n");
		//após envio do comando, começa a receber dados do LIST no datasocket, e imprime
		do {
			x = recv(socketd, data, sizeof(char) * BUFFER*2, MSG_WAITALL);
			//if (x!=0) 
			fwrite(data, x, 1, file);
		} while (x != 0);

	}
	else{
		perror("Errno");
		fclose(file);
		shutdown(socketd,SHUT_RDWR);
		close(socketd);	
		return -1;
	}	


	fclose(file);
	shutdown(socketd,SHUT_RDWR);
	close(socketd);	
	return returnval;
}


int cmd_put(int socketc, int mode, char* filepath){

	char data[BUFFER*2], dummy[BUFFER];
	int x, socketd, retval;
	FILE *file;

	//cria data socket no modo activado previamente (ou default)
	if(mode == 0) {
		if( (socketd = datasocket_create_PASV(socketc)) < 0){
			printf("Data connection not established, exiting...\n");
			perror("Errno");
			return -1;
		}
	} else {
		if( (socketd = datasocket_create_PORT(socketc)) < 0){
			printf("Data connection not established, exiting...\n");
			return -1;
		}
	}

	//é necessário definir o nome do ficheiro local a partir do filepath
	//i.e. retirar eventuais directorias no cabeçalho do filepath, para guardar
	//o ficheiro na pasta do programa (ainda não implementado)

	/*
	filename ⁼ strtok(filepath, "");
	printf("filename\n");
	
	aux = filename;
	do{
		aux = strtok(NULL, "/");
		printf(aux);
	}
	while(strcmp(aux, ""));
	
	*/

	file = fopen(filepath, "r"); //retorna null pointer em erro

	if ( file == NULL ) {
		printf("Cannot open local file\n");
		//neste caso, deve ser pedida resposta ao servidor, que já estava a processar o STOR
		//e assim paramos a transferência ao servidor
		send(socketc, "NOOP\r\n", 6, 0);
		shutdown(socketd,SHUT_RDWR);
		close(socketd);

		return -1;
	} 

	//envia comando STOR e recebe confirmação
	sprintf(dummy, "STOR %s\r\n", filepath);
	
	if ( send(socketc, dummy, sizeof(dummy), 0) < 0) {
		perror("Errno");
		shutdown(socketd,SHUT_RDWR);
		close(socketd);	
		return -1;
	}
	retval = serverMessage(socketc, dummy);
	
	if( retval == 150) {
		do {
			x = fread(data, sizeof(char) * BUFFER*2, 1, file);
			//if (x!=0) 
			send(socketd, data, sizeof(char) * BUFFER*2, 0);
		} while ( x != 0);

	}
	else{
		perror("Errno");
		fclose(file);
		shutdown(socketd,SHUT_RDWR);
		close(socketd);	
		return -1;
	}	


	fclose(file);
	shutdown(socketd,SHUT_RDWR);
	close(socketd);	
	return 1;
}


int cmd_mget(int socketc, int mode, char* filepath){

	return 1;
}


int cmd_mput(int socketc, int mode, char* filepath){

	return 1;
}


void cmd_help() {
	printf("\n-------------------- HELP - COMMAND LIST --------------------\n\n");
	printf("- COMMAND - DESCRIPTION - ACCEPTED FLAGS -\n");
	printf("LS - list the names of the files in the current remote directory - f\n");
	printf("LIST - list the names of the files in the local directory - f\n");
	printf("CD - change directory on the remote machine\n");
	printf("LCD - change directory on the local machine\n");
	printf("PWD - lists the pathname of the current directory on the remote machine \n");
	printf("PATH - lists the pathname of the current directory on the local machine \n");
	printf("GET - copy one file from the current remote machine to the local machine\n");
	printf("MGET - copy multiple file from the current remote machine to the local machine\n");
	printf("PUT - copy one file from the current local machine to the remote machine\n");
	printf("MPUT - copy multiple file from the current local machine to the remote machine\n");
	printf("TRANSFER - transfer one file from the current remote machine to the other remote machine\n");
	printf("DELETE - removes a file in the current remote directory\n");
	printf("MKDIR - make a new directory within the current remote directory\n");
	printf("RMDIR - removes a directory in the current remote directory \n");
	//printf("MODE - switch between active and passive mode\n");
	printf("SWAP - swap between origin and destination server\n");
	printf("SERVERS - generates a list of known ftp servers\n");
	printf("HELP - request a list of all available FTP commands - s\n");
	printf("CLEAR - clears screen\n");
	printf("QUIT - exit the FTP environment\n");
	printf("\n");
}

void cmd_servers() {
	printf("\n--- FTP SERVERS ---\n\n");
	printf("gnomo.fe.up.pt - 192.168.50.138\n");
	printf("ftp.ist.utl.pt - 193.136.128.6\n");
	printf("\n");
}

//função que recebe socket de controlo de servidor de origem e destino do ficheiro a transferir
//recebe em modo passivo, e envia em modo activo (cria 2 data sockets, 1 para cada)
int retr_stor(int controlsource, int controldest, char* filepath){

	char data[BUFFER*2], dummy[BUFFER];
	int x, datasource, datadest;

	datasource = datasocket_create_PASV(controlsource);
	
	if( datasource < 0) {
		printf("Data connection not established on source, exiting...\n");
		perror("Errno");
		return -1;
	}
	printf("Source Server Ready\n");

	datadest = datasocket_create_PASV(controldest);
	if( datadest < 0){
		printf("Data connection not established on destination, exiting...\n");
		perror("Errno");
		return -1;
	}
	printf("Destination Server Ready\n");

	//envia comando RETR para source
	sprintf(dummy, "RETR %s\r\n", filepath);
	if ( (send(controlsource, dummy, sizeof(dummy), 0)) < 0) {
		perror("Errno");
		shutdown(controlsource,SHUT_RDWR);
		close(controlsource);	
		return -1;
	}
	

	if( (serverMessage(controlsource, dummy)) == 150){

		printf("Source server accepted RETR command.\n");
		printf("Now sending STOR to destination server...\n");
	
	}
	else {
		printf("Error: RETR command not successfull on source server!\n");
		return -1;
	}

	//envia comando STOR para destination
	sprintf(dummy, "STOR %s\r\n", filepath);
	if ( (send(controldest, dummy, sizeof(dummy), 0)) < 0) {
		perror("Errno");
		shutdown(controldest,SHUT_RDWR);
		close(controldest);	
		return -1;
	}
		

	if( (serverMessage(controldest, dummy)) == 150){

		printf("Destination server accepted STOR command.\n");
	
	}
	else {
		printf("Error: STOR command not successfull on destination server!\n");
		return -1;
	}

	//após envio dos comandos, começa a receber e enviar dados pelos datasockets

	do {
		x = recv(datasource, data, sizeof(char) * BUFFER*2, MSG_WAITALL);
		if(x != 0) send(datadest, data, sizeof(char) * BUFFER*2, 0);
		//eventualmente pode ir criando um ficheiro local de backup, que fica no cliente					
	} while (x != 0);

	shutdown(datasource,SHUT_RDWR);
	shutdown(datadest,SHUT_RDWR);
	close(datasource);
	close(datadest);
	
	serverMessage(controldest, dummy);
	
	return 1;

}

int mtransfer(int socketc1, int socketc2, char * filepath) {

	char ** linhas, ** words, big[BUFFER*10];
	int n, transfered = 0, counter;

	if(cmd_ls(socketc1, 0, 2, big) < 0)
		return -1;

	n = makeargv(big, "\r\n" , &linhas);


	for(counter=0; counter<n; counter++) {
		
		makeargv(linhas[counter], " ", &words);
		
		//eliminamos directorios do pattern matching
		if(words[0][0] != 'd') {

			if(fnmatch(filepath, words[8], 0) == 0) {
				printf("Transfering file %s....\n", words[8]);
				
				retr_stor(socketc1, socketc2, words[8]);
				transfered++;
				sleep(1);
			}
		}
		freemakeargv(words);
	}
	freemakeargv(linhas);

	if(transfered<1)
		printf("No files found\n");
	else
		printf("Transfered %i files matching the given pattern. \n", transfered);
	
	return 1;
}

//LOOP
void loop(char *server1, char *server2, int socketsrc, int socketdst){

	int servercode=0, logged=FALSE, quit=FALSE, flag=FALSE, swap=0;
	char data[BUFFER], aux[BUFFER], *aux2,  name[BUFFER], *pass, unused[BUFFER];
	
	int socket=socketsrc, socketi=socketdst, x=0;
	char user1[BUFFER], user2[BUFFER], pass1[BUFFER], pass2[BUFFER], big[BUFFER*10];

	printf("Working by default on first server\n");
	do {	
		if (flag==FALSE) {
			servercode = serverMessage(socket, unused);
		}
		else {
			flag=FALSE;
		}
			
		switch (servercode) {
			case 220:	
				//USER - utilizador
				printf("Name: ");
				scanf("%s", name);
				sprintf(data, "USER %s\r\n", name);
				if (swap==0) {
					strcpy(user1, data);
				} else {
					strcpy(user2, data);
				}
				if (send(socket, data, strlen(data), 0) < 0) {
					perror("Errno");
					return;
				}
					
				break;
			case 331:
				//PASS - password
				pass = getpass("Password: ");
				sprintf(data, "PASS %s\r\n", pass);
				getchar();
				if (swap==0) {
					strcpy(pass1,data);
				} else {
					strcpy(pass2,data);
				}
				if (send(socket, data, strlen(data), 0) < 0) {
					perror("Errno");
					return;
				}
					
				break;
			case 230:
				//Sucesso de Login
				if (swap==0) {
					swap=1;
					socket=socketdst;
					printf("On Server 2\n");
				} else {
					swap=0;
					socket=socketsrc;
					logged=TRUE;
					printf("Servers online\n");
					printf("On Server 1\n");
				}
				break;
			case 530:
				//Falhou Login
				flag=TRUE;
				servercode=220;
				break;
			case 221:
				//QUIT
				quit = TRUE;
				logged = FALSE;
				break;
			case 421:
				//TIMEOUT
				quit = TRUE;
				logged = FALSE;
				printf("Servers offline\n");
				break;
			case 257:
				//PWD - directorio local
				printf("Local directory OK\n");
				break;
			/*case 200:
				//TYPE - tipo de ficheiro a transferir:
				//A - ASCII text; E - EBCDIC text; I - image (binary data); L - local format
				break;*/
			/*case 200:
				//PORT - Active Mode
				printf("\n");
				pasvmode=FALSE;
				break;*/
			/*case 227:
				//PASV - Passive Mode
				printf("Port: n1*256+n2\n");
				pasvmode=TRUE;
				break;*/
			case 150:
				//RETR - recebe data
				printf("Receiving data\n");
				break;
			case 226:
				//Transferencia completa	
				while (ioctl(socketsrc, I_PEEK) == TRUE) {
					x = recv(socketsrc, data, sizeof(char) * BUFFER, O_NONBLOCK);
				}
				while (ioctl(socketdst, I_PEEK) == TRUE) {
					x = recv(socketdst, data, sizeof(char) * BUFFER, O_NONBLOCK);
				}
				/*send(socketsrc, "QUIT\r\n", 6, 0);
				send(socketdst, "QUIT\r\n", 6, 0);
				shutdown(socketsrc,SHUT_RDWR);
				shutdown(socketdst,SHUT_RDWR);
				close(socketsrc);
				close(socketdst);
				socketsrc = control_connect(server1, 21);
				socketdst = control_connect(server2, 21);
				send(socketsrc, user1, strlen(user1), 0);
				send(socketsrc, pass1, strlen(pass1), 0);
				send(socketdst, user2, strlen(user2), 0);
				send(socketdst, pass2, strlen(pass2), 0);
				socket=socketsrc;
				socketi=socketdst;*/
				//send(socket, "NOOP\r\n", 6, 0);
				printf("Operation complete\n");
				break;
			case -1:
				printf("FTP ERROR\n");
				quit = TRUE;
				logged = FALSE;
				break;
			case 1:
				//printf("Out of Sync. Try again\n");
				printf("Out of Sync. Retrying...\n");
				sleep(1);
				break;
			default :
				/*printf("\n");
				quit = TRUE;
				logged = FALSE;*/
				break;

		}
		
		if (logged==TRUE) {
			if (servercode!=1) {
				printf("\nftp> ");
				gets(data);
			}
			//scanf("%s",data);
			//getchar();
			
			if (strncasecmp(data, "HELP", 4) == 0) {
				if (strncasecmp(data, "HELP -S", 7) == 0) {
					if (send(socket, "HELP\r\n", 6, 0) < 0) {
						perror("Errno");
						return;
					}
				} else {
					flag=TRUE;
					servercode=0;
					cmd_help();
				}
			}
			
			else if (strncasecmp(data, "SERVERS", 7) == 0) {
				flag=TRUE;
				cmd_servers();
				servercode=0;
			}
			
			else if (strncasecmp(data, "CLEAR", 5) == 0) {
				flag=TRUE;
				system("clear");
				servercode=0;
			}
			
			else if (strncasecmp(data, "SWAP", 4) == 0) {
				flag=TRUE;
				if (swap==0) {
					swap=1;
					socket=socketdst;
					socketi=socketsrc;
					printf("Swapped to server 2\n");
				} else {
					swap=0;
					socket=socketsrc;
					socketi=socketdst;
					printf("Swapped to server 1\n");
				}
				servercode=0;
			}
			
			else if (strncasecmp(data, "QUIT", 4) == 0) {
				if (send(socketsrc, "QUIT\r\n", 6, 0) < 0) {
					printf("Error on close\n");
					return;
				}
				if (send(socketdst, "QUIT\r\n", 6, 0) < 0) {
					printf("Error on close\n");
					return;
				}
			}
			
			else if (strncasecmp(data, "LS", 2) == 0) {
				if (strncasecmp(data, "LS -F", 5) == 0) {
					if (cmd_ls(socket, 0, 1, big)<0) {
						printf("Could not list\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				} else {
					if (cmd_ls(socket, 0, 0, big)<0) {
						printf("Could not list\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				}
			}
			
			else if (strncasecmp(data, "LIST", 4) == 0) {
				flag=TRUE;
				servercode=0;
				if (strncasecmp(data, "LIST -F", 7) == 0) {
					system("ls");
				} else {
					system("ls -l");
				}
			}
			
			else if (strncasecmp(data, "CD", 2) == 0) {
			
				if (strcasecmp(data, "CD") == 0) {
					printf("(remote-directory) ");
					scanf("%s", data);
					getchar();
					sprintf(aux, "CWD %s\r\n", data);
				} else {
					sprintf(aux, "CWD %s\r\n", data + 3);
				}
				
				if (send(socket, aux, strlen(aux), 0) < 0) {
					printf("Could not change directory\n");
					//return;
					flag=TRUE;
					servercode=0;
				}
			}
			
			else if (strncasecmp(data, "LCD", 3) == 0) {
				flag=TRUE;
				servercode=0;
				
				if (strcasecmp(data, "LCD") == 0) {
					printf("(local-directory) ");
					scanf("%s", aux);
					getchar();
				} else {
					strcpy(aux, data + 4);
				}
				
				if (chdir(aux) != 0) {
					printf("Error in chdir\n");
					//return;
				} else {
					printf("Directory change successful\n");
				}
			}
			
			else if (strncasecmp(data, "PWD", 3) == 0) {
				if (send(socket, "PWD\r\n", 5, 0) < 0) {
					printf("Cannot read current directory\n");
					//return;
					flag=TRUE;
					servercode=0;
				}
			}
			
			else if (strncasecmp(data, "PATH", 4) == 0) {
				flag=TRUE;
				getcwd(aux, sizeof(aux));
				servercode=0;
				printf("Local directory: %s\n", aux);
			}
			
			else if (strncasecmp(data, "DELETE", 6) == 0) {
			
				if (strcasecmp(data, "DELETE") == 0) {
					printf("(remote-file) ");
					scanf("%s", data);
					getchar();
					sprintf(aux, "DELE %s\r\n", data);
				} else {
					sprintf(aux, "DELE %s\r\n", data + 7);
				}

				if (send(socket, aux, strlen(aux), 0) < 0) {
					perror("Errno");
					//return;
					flag=TRUE;
					servercode=0;
				}
			}
			
			else if (strncasecmp(data, "MKDIR", 5) == 0) {
			
				if (strcasecmp(data, "MKDIR") == 0) {
					printf("(remote-directory) ");
					scanf("%s", data);
					getchar();
					sprintf(aux, "MKD %s\r\n", data);
				} else {
					sprintf(aux, "MKD %s\r\n", data + 6);
				}

				if (send(socket, aux, strlen(aux), 0) < 0) {
					perror("Errno");
					//return;
					flag=TRUE;
					servercode=0;
				}
					
			}
			
			else if (strncasecmp(data, "RMDIR", 5) == 0) {
			
				if (strcasecmp(data, "RMDIR") == 0) {
					printf("(remote-directory) ");
					scanf("%s", data);
					getchar();
					sprintf(aux, "RMD %s\r\n", data);
				} else {
					sprintf(aux, "RMD %s\r\n", data + 6);
				}

				if (send(socket, aux, strlen(aux), 0) < 0) {
					perror("Errno");
					//return;
					flag=TRUE;
					servercode=0;
				}
			}
			
			else if (strncasecmp(data, "GET", 3) == 0) {
			
				if (strcasecmp(data, "GET") == 0) {
					printf("(remote-directory) ");
					scanf("%s", aux);
					getchar();
					if (cmd_get(socket, 0, aux)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;	
					}
				} else {
					aux2= (char*) malloc (sizeof(char));
					aux2=strtok(data, " ");
					aux2=strtok(NULL, "\0");
					if (cmd_get(socket, 0, aux2) < 0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				}
				//send(socket, "NOOP\r\n", 6, 0);
			}
			
			else if (strncasecmp(data, "MGET", 4) == 0) {
			
				if (strcasecmp(data, "MGET") == 0) {
					printf("(remote-directory) ");
					scanf("%s", aux);
					getchar();
					if (cmd_mget(socket, 0, aux)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				} else {
					aux2= (char*) malloc (sizeof(char));
					aux2=strtok(data, " ");
					aux2=strtok(NULL, "\0");
					if (cmd_mget(socket, 0, aux2) < 0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				}
				//send(socket, "NOOP\r\n", 6, 0);
			}
			
			else if (strncasecmp(data, "PUT", 3) == 0) {
				if (strcasecmp(data, "PUT") == 0) {
					printf("(remote-directory) ");
					scanf("%s", data);
					getchar();
					if (cmd_put(socket, 0, data)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				} else {
					aux2=(char*) malloc (sizeof(char));
					aux2=strtok(data, " ");
					aux2=strtok(NULL, "\0");
					if (cmd_put(socket, 0, aux2)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				}
				//send(socket, "NOOP\r\n", 6, 0);
				
			}
			
			else if (strncasecmp(data, "MPUT", 4) == 0) {
				if (strcasecmp(data, "MPUT") == 0) {
					printf("(remote-directory) ");
					scanf("%s", data);
					getchar();
					if (cmd_mput(socket, 0, data)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				} else {
					aux2=(char*) malloc (sizeof(char));
					aux2=strtok(data, " ");
					aux2=strtok(NULL, "\0");
					if (cmd_mput(socket, 0, aux2)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				}
				//send(socket, "NOOP\r\n", 6, 0);
				
			}
			
			else if (strncasecmp(data, "MTRANSFER", 9) == 0) {
				if (strcasecmp(data, "MTRANSFER") == 0) {
					printf("(remote-directory) ");
					scanf("%s", data);
					getchar();
					if (mtransfer(socket, socketi, data)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				} else {
					aux2=(char*) malloc (sizeof(char));
					aux2=strtok(data, " ");
					aux2=strtok(NULL, "\0");
					if (mtransfer(socket, socketi, aux2)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				}
				send(socket, "NOOP\r\n", 6, 0);
			
			}
			
			
			else if (strncasecmp(data, "TRANSFER", 8) == 0) {
				if (strcasecmp(data, "TRANSFER") == 0) {
					printf("(remote-directory) ");
					scanf("%s", data);
					getchar();
					if (retr_stor(socket, socketi, data)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				} else {
					aux2=(char*) malloc (sizeof(char));
					aux2=strtok(data, " ");
					aux2=strtok(NULL, "\0");
					if (retr_stor(socket, socketi, aux2)<0) {
						printf("Transfer not completed\n");
						//return;
						flag=TRUE;
						servercode=0;
					}
				}
				send(socket, "NOOP\r\n", 6, 0);
			
			}
			
			/*else if (strncasecmp(data, "MODE", 4) == 0) {
				if (mode==TRUE) {
					mode=FALSE;
					printf("Server-to-Server connection established\n");
				} else {
					mode=TRUE;
					printf("Server-to-Client connection established\n");
				}
				
				flag=TRUE;
				servercode=0;
			}*/
			
			else {
				flag=TRUE;
				servercode=0;
				printf("Invalid Command\n");
			}
			
		}
	      	
		
	} while(quit==FALSE);
	
	return;
}

int main (int argc, char** argv)
{

	int socketcsrc, socketcdst;	//sockets de control_connection
	signal(SIGINT, &interrupt);

	if (argc > 3) {
		printf("Usage: myftp <Server 1> <Server2>\n");
		return -1;
	}
	
	system ("clear");
	
	/*
		Primeiro deve ser efectuada a ligação de controlo
		ao servidor ftp, e manter-se ligada durante toda a 
		a execução do cliente.
	*/
	
	printf("Establishing connection on Server 1\n");
	socketcsrc = control_connect(argv[1], 21);
	if (socketcsrc < 0) {
		printf("Error connecting server1.\n");
		return 0;
	} else {
		printf("FTP connection established on: %s\n", argv[1]);
	}
	
	printf("Establishing connection on Server 2\n");
	socketcdst = control_connect(argv[2], 21);
	if (socketcdst < 0) {
		printf("Error connecting server2.\n");
		return 0;
	} else {
		printf("FTP connection established on: %s\n", argv[2]);
	}
	
	printf("Remote system type is UNIX.\n");
	printf("Using binary mode to transfer files.\n");

	loop(argv[1],argv[2],socketcsrc,socketcdst);
	
	//fecha ligacao
	shutdown(socketcsrc,SHUT_RDWR);
	shutdown(socketcdst,SHUT_RDWR);
	close(socketcsrc);
	close(socketcdst);
	
	return 0;
}

