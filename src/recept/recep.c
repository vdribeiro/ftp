#include "appif.h"

// ficheiro de destino
FILE *appfd;
char* pacote;

//variáveis para comparar os tamanhos esperados e definitivos, e o num de sequencia
unsigned long int expectedsize = 0, definitesize, nseq = 0, ntramas_i = 0, nlog=1;
int isopen = FALSE, FILESIZE = FALSE, FILENAME = FALSE, CUSTOM_BR = FALSE;
char* name;
char* USER_BAUD;



//fragmento de ficheiro do start
char* start;


void logWriter(){

printf("\n\nA escrever log....");	
	

	int log = open("log.txt", O_CREAT | O_WRONLY, 0600);

	char buffer [50];

	sprintf(buffer,"Settings usados:\n");
	write(log, buffer, strlen(buffer));
	memset(buffer,0,strlen(buffer));

	if(FILENAME == TRUE){
		sprintf(buffer,"Filename: %s\n", name);
		write(log, buffer, strlen(buffer));
		memset(buffer,0,strlen(buffer));
	}

	if(CUSTOM_BR == TRUE){
		sprintf(buffer,"BAUDRATE: %s\n", USER_BAUD);

	}
	else
		sprintf(buffer,"BAUDRATE: 38400\n");


	write(log, buffer, strlen(buffer));
	memset(buffer,0,strlen(buffer));

	sprintf(buffer,"Numero de tramas de informacao recebidas: %li\n", ntramas_i);
	write(log, buffer, strlen(buffer));
	memset(buffer,0,strlen(buffer));

	sprintf(buffer,"Tamanho total de dados recebidos: %li\n\n", definitesize);
	write(log, buffer, strlen(buffer));
	memset(buffer,0,strlen(buffer));

	sprintf(buffer,"Numero de tramas duplicadas: %i\n", ntramas_d);
	write(log, buffer, strlen(buffer));
	memset(buffer,0,strlen(buffer));

	sprintf(buffer,"Numero de tramas REJ enviadas: %i\n", ntramas_r);
	write(log, buffer, strlen(buffer));
	memset(buffer,0,strlen(buffer));

	close(log);
	printf("Log escrito correctamente!\n");

/*
name
expectedsize, definitesize, 
CUSTOM_BR = TRUE;
USER_BAUD[0] = 'B';
ntramas_i
ntramas_r
ntramas_d
*/

}

void createfile(long int size){

	expectedsize = size;
	if((appfd = fopen("ficheiro_recebido.txt", "w"))){
		isopen = TRUE;	
		printf("Ficheiro criado com nome por defeito 'ficheiro_recebido.txt'\n");}
	
	return;

}

void createfile2(long int size, char* fn){

	expectedsize = size;	
	
	//acrescentar '\0' ???????

	if((appfd = fopen(fn, "wb"))){
		isopen = TRUE;	
		printf("Ficheiro criado com nome recebido\n");
	}
	
	return;

}



int app(int len, int STATE){

	int count = 1;
	
	
	//printf("entrou no app\n");
	//printf("pacote:%x\n", pacote[0]);


	if((pacote[0] == CTRL_START) && (STATE == 0)){

		printf("Pacote de start recebido, a processar...\n");
		
		ntramas_i = 0;		
			
		start = malloc(len*sizeof(char));	
		//parameters
		long int size;
		
		name = malloc(8*255);
		char* aux = malloc(8*1024);
		
		int t = -1, l = 0;
		
		//criar fragmento para comparar com pacote END
		int x;
		for(x = 1; x<len; x++){
			start[x-1] = pacote[x];
			printf("%x ", start[x-1]);
		}
		printf("\nPacote start criado\n");

		do {
			t = pacote[count];
			l = pacote[count+1];

			int k = 0;
			for(k = 0; k < l; k++){	
				aux[k] = pacote[count+2+k];
				printf("%x", aux[k]);
			}
			printf("\n");

			if (t==0) {
				FILESIZE = TRUE;
				size = atoi(aux);
				printf("\n Tamanho Esperado do ficheiro: %i", (int)size);
			} else if (t==1) {
				FILENAME = TRUE;
				printf("\n Nome do ficheiro: ");
				int z=0;
				for(z=0;z<=l;z++)
				{
					name[z] = aux[z];
					printf("%c", name[z]);
				}
				printf("\n");
				
			}
			
			count += 2+l;
		} while(count<len);
		
		if(FILESIZE == TRUE)
			if(FILENAME == TRUE){
				createfile2(size, name);
				return 1;
			}
			else{
				createfile(size);
				return 1;
			}
		else{
			printf("\n Pacote não obedece ao protocolo da aplicação (!tlv, ou não tem filesize definido)");
			return 0;	
		}
		
	}
	else if((pacote[0] == CTRL_DATA) && (STATE == 1)){
		
		printf("\nNSEQ recebido: %i", (int)pacote[1]);
		printf("\nNSEQ esperado: %i", (int)nseq);

		if((int)pacote[1] != nseq){
			printf("Numero de sequencia errado!\n");
			//return 1;
		}

		nseq++;
		if(nseq >= 256)
			nseq = 0;
			
		
		int k = 256 * (int)pacote[2] + (int)pacote[3];
		
		//printf("kvalue: %i         writen: ", k);
		printf("\nwriting...[");
		
		char* aux2 = malloc(sizeof(char)*1024);

		for(count = 0; count < k-1; count++){
			definitesize++;
			aux2[count] = pacote[count+4];
			//putc(pacote[count+4], appfd);
			printf("%x", aux2[count]);
		}
		
		fwrite(aux2, 1, k-1, appfd);
		
		printf("]");

		ntramas_i++;

		return 1;
	}
	else if((pacote[0] == CTRL_END) && (STATE == 1)){
		
		//comparação com fragmento de ficheiro de start
		int x;
		for(x = 1; x<len; x++)
			if(pacote[x] != start[x-1]){
				printf("\nO pacote END nao e igual ao START"); 
				return -1;
			}
		
		
		//comparar expectedsize com definitesize e retornar erro em printf
		if(definitesize != expectedsize)
			printf("\nTamanho esperado diferente de quantidade de dados recebida para escrita\n");
		else
			printf("\nTamanho esperado é igual ao recebido para escrita\n");
		
		//fechar ficheiro		
		//fclose(appfd);
		
		return 0;
	}
	
	else return 0;
	

}

int main(int argc, char** argv)
{

	ntramas_d = 0;
	ntramas_r = 0;

	int fd = -1;

	USER_BAUD = malloc(20*sizeof(char));

	if(argc < 2 ){
		printf("Utilização:\tnserial SerialPort <Baudrate: B[valor]>\n\tie: nserial /dev/ttyS1 38400 \nCUIDADO: Baudrate deve ter o mesmo valor no emissor e receptor! \n");
		exit(1);
		}
		
	else if(argc==2){
		printf("A usar valor padrão de Baudrate: 38400 \n");

	}
	else{
		
		CUSTOM_BR = TRUE;
		USER_BAUD[0] = 'B';
		
		int baudcount;
		
		for(baudcount = 0; baudcount < strlen(argv[2]); baudcount++){

			USER_BAUD[baudcount+1] = argv[2][baudcount];
		}
		//strcat(USER_BAUD, argv[2]);
		printf("\nA usar valor do utilizador de Baudrate: %s\n", USER_BAUD);

	}

	
	/*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
	*/
	    
	fd = open(argv[1], O_RDWR | O_NOCTTY );
	if (fd <0) {perror(argv[1]); exit(-1); }
	printf("Port Open\n");

	if ( tcgetattr(fd,&oldtio) == -1) { // save current port settings
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));

	//custom baudrate setup
	if(CUSTOM_BR == TRUE)
		newtio.c_cflag = atoi(USER_BAUD) | CS8 | CLOCAL | CREAD;
	else
		newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	// set input mode (non-canonical, no echo,...)
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 0;   // inter-character timer unused
	newtio.c_cc[VMIN]     = 0;   // blocking read until n chars received

	/* 
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
	leitura do(s) próximo(s) caracter(es)
	*/

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");
	
	/*
	Porta aberta
	*/



	/*
	Maquina de estados do servidor.
	0: Tenta estabelecer ligacao (llopen)
	1: A receber dados (llread)
	*/
	int SERVERSTATE = 0, APPSTATE = 0; 
	int read_ret = 0;	
	char data[1023];

	while(1) {
		
		switch(SERVERSTATE)
		{

			case 0:
			{
				nseq=0;
				/*
				E chamada a funcao de estabelecimento de ligacao, llopen.
				Esta funcao bloqueia devido a um ciclo while (nao na funcao read,
				pois a transmissao e assincrona). Apos sucesso, o programa pode
				iniciar a transferencia dos dados pretendidos.
				*/
				fd = llopen(fd);
				printf("Descritor: %i\n", fd);
				if (fd<0) {
					printf("Inicio de ligação mal efectuado!\n");
					printf("Trama enviada nao obedece ao protocolo.\n");
				}
				else {	
					SERVERSTATE = 1;
					printf("---------------LIGACAO-----------------\n");
				}
				break;
			}
		
			case 1:
			{
				printf("\nLeitura...");
				/*
				Transmissao de dados. 
				Visto tratar-se do servidor, executamos sempre a funcao llread
				*/
				read_ret = llread(fd, data);
				pacote = (char*) malloc(read_ret*sizeof(char));
			
				
				if (read_ret<0) {
					printf("Erro\n");
					if(isopen == TRUE){
						fclose(appfd);
						isopen = FALSE;
					}
				} else if(read_ret == 0) {
					SERVERSTATE = 0;
					logWriter();
					nlog++;

					if(isopen == TRUE){
						fclose(appfd);	
					isopen = FALSE;}
				}
				else {
					//strncpy(pacote, data, read_ret);
					int k = 0;
					//printf("pacote: [");
					for(k=0; k<read_ret;k++){
						pacote[k] = data[k];
						//printf("%x, ", pacote[k]);
					}
					//printf("]\n");
					
					APPSTATE = app(read_ret, APPSTATE);
					//if(APPSTATE <0)
						
				}
				break;
			}
		}
	}
	
	if(isopen == TRUE)
		fclose(appfd);

	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);

	return 0;
}


