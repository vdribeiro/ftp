#include "appif.h"

volatile int STOP = FALSE;



int INFSTATE = FALSE;


//Estabelece ligação retornando o descritor
int llopen(int fd) {
	
	ntramas_d = 0;
	ntramas_r = 0;

	char buf, buf2[255];
	int STATE = 0, res = 0, sent = 0; //estado inicial
	while (1) {
		res = read(fd,&buf,1);
		
		if (res != 0) { //leu dados

			//Flag
			if ((buf == FLAG) && (STATE == 0)) {
				printf("\nDados recebidos, a confirmar trama de ligação (handshake).\n");
				//printf("\nFlag inicial OK!\n");
				STATE = 1;
				
				continue;
			}

			//Address (must be ADDR_SENDER)
			else if ((buf == ADDR_SENDER) && (STATE == 1)) { 
				//printf("ADDR_SEND OK!\n");
				STATE = 2;	
				continue;
			}
			
			//Control, must be CTRL_SET
			else if ((buf == CTRL_SET) && (STATE == 2)) { 
				//printf("CTRL_SET OK!\n");
				STATE = 3;
				continue;
			}

			//BCC1 (Head bcc, must be ADDR_SENDER ^ CTRL_SET)
			else if ((buf == (ADDR_SENDER ^ CTRL_SET)) && (STATE == 3)) {
				//printf("ADDR_SENDER ^ CTRL_SET OK!\n");
				STATE = 4;
				continue;
			}

			//Must be Flag on 5th byte, returns UA supervision message
			else if ((buf == FLAG) && (STATE==4)) { //0x7E
				buf2[0] = FLAG;				
				buf2[1] = ADDR_RECEIVER;
				buf2[2] = CTRL_UA;
				buf2[3] = (ADDR_RECEIVER ^ CTRL_UA);
				buf2[4] = FLAG;
				//printf("Flag final OK!\n");

				//envio da flag de confirmacao ao cliente
				sent = write(fd, buf2, 5);
				printf("\nLigação de dados pedida correctamente.");
				printf("\nTrama de confirmação enviada: %i bytes\n", sent);
				
				//apos confirmacao da totatilade da trama,
				//a funcao retorna o file descriptor em "int fd"(sucesso de llopen())
				//o file descriptor esta pronto a receber dados ate a ligacao ser terminada
				return fd;
			}
			else return -1;
			
			//caso não seja verificada uma das condições anteriores, ao receber dados,
			//a ligação falha devido a ma trama de ligação,
			//logo o ciclo termina e llopen retorna -1 (erro)
			
		}
	}  

}

//Funcao para leitura de tramas
int llread (int fd, char* data){

	//estados iniciais
	int STATE = 0, STUFFED = FALSE, DISCONNECT_STATE = FALSE, DUPLICATE=FALSE;
	char buf, buf2[1023];
	int res = 0, cnt=0, sent = 0, x=0;

	//alarm
	/*TIMEOUT=TRUE;
	signal (SIGALRM, timeout_handler);
	alarm(WAIT_TIME);*/

	while (1) {

		//se encontrar a ultima flag avanca
		if (STATE==5) res=1;
		//senao le bytes
		else res = read(fd,&buf,1);

		//se devolver negativo deu erro
		if(res < 0)
			return res;	

		//leu dados
		else if (res > 0) {

			//F(flag: 0x7E)
			if ((buf == FLAG) && (STATE == 0)) {
				cnt = 0;
				DUPLICATE = FALSE;
				STUFFED = FALSE;
				STATE = 1;
				//printf("\nFlag inicial OK!");
				continue;
			}
			
			//A(address: sender or receiver - only accepts sender)
			else if ((buf == ADDR_SENDER) && (STATE == 1)) { 
				STATE = 2;
				//printf("\nADDR_SEND OK!");
				continue;
			}

			//C(control: SET, UA, DISC, RR, REJ, INF_0, INF_2 - only accepts DISC and INF_X)
			//this defines the type of message: Information or Supervision (UA is only sent by server)
			else if (STATE == 2){

				if(buf == CTRL_DISC) {
					STATE = 3;
					//printf("\nCTRL_DISC OK!");
					DISCONNECT_STATE = TRUE;
					continue;
				}

				else if ((buf == INF_0 ) && (INFSTATE == FALSE)) { 
					//printf("\nINF_0 OK!");
					INFSTATE = TRUE;
					STATE = 3;
					continue;
				}

				else if((buf == INF_0 ) && (INFSTATE == TRUE)) {
					//printf("\nINF_2 NOT OK!");
					DUPLICATE = TRUE;
					STATE=3;					
					continue;
				}

				else if ((buf == INF_2) && (INFSTATE == TRUE)) { 
					//printf("\nINF_2 OK!");
					INFSTATE = FALSE;
					STATE = 3;
					continue;
				}

				else if ((buf == INF_2) && (INFSTATE == FALSE)) {
					//printf("\nINF_0 NOT OK!");
					DUPLICATE = TRUE;
					STATE=3;
					continue;
				} 
			}

			//BCC1
			else if (STATE==3) {
				
				if((DISCONNECT_STATE == TRUE) && (buf == (ADDR_SENDER ^ CTRL_DISC))){
					STATE = 4;
					//printf("\nADDR_SENDER ^ CTRL_DISC OK!");
					continue;
				}

				if((INFSTATE==TRUE) && ((ADDR_SENDER ^ INF_0) == buf)) {
					STATE=4;
					//printf("\nADDR_SENDER ^ INF_0 OK!");
					continue;
				}

				else if ((INFSTATE==FALSE) && ((ADDR_SENDER ^ INF_2) == buf)) {
					STATE=4;
					//printf("\nADDR_SENDER ^ INF_2 OK!");
					continue;
				}

				else {
					STATE = 0;
					printf("\nErro");
					continue;
				}
			}

			/*
			DATA
			Leitura de dados para array data,
			com unstuffing implementado caracter a caracter
			*/
			else if (STATE==4) {
				//printf("\nbuf: %x", buf);
				//printf("\nContador %i ", cnt);
				//printf("Buf %i",buf);

				//testa se le disconnect
				if(DISCONNECT_STATE == TRUE) {

					if(buf==FLAG){

						//disconnect
						printf("\nDisconnect recebido. A enviar ACK de disconnect\n");
						return llclose(fd);
					}

					else {

						//erro;
						printf("\nErro na trama de disconnect\n");
						return -1;
					}
				}
							
				//testa se encontra a flag final
				else if (buf == FLAG) {
					
					if(STUFFED == TRUE) {
						cnt++;
						data[cnt] = STUFF;	
						STUFFED = FALSE;
					}
					printf("Leu trama completa\n");
					cnt--;
					STATE=5;
					continue;
				}

				//caso encontre 7D, HEAD de byte STUFFED
				else if(buf == STUFF) {

					//dois 7D seguidos sao tratados pelo 1o 7D independente e o 2o poderá ser 
					//head de byte stuffing, dependendo da proxima iteraçao (caracter)
					if(STUFFED == TRUE) {
						data[cnt] = STUFF;
						cnt++;
					}
				
					//printf("\n7D received, now checking if its stuffed\n");
					STUFFED = TRUE;				
					continue;				
				}
				
				else if (STUFFED == TRUE) {

					//faz unstuffing de 7D5E para flag
					if(buf == STUFF_E){
						data[cnt] = FLAG;
						//printf("\n7D 5E received, now unstuffed to FLAG\n");
						cnt++;
						STUFFED = FALSE;
						continue;
					}

					//faz unstuffing de 7D5D para 7D
					//(apesar de haver stuffing para 7D5D->7D, assume-se noutras situações
					//, em que haja 7DXX com XX diferente de 5E e 5D, também o caracter 
					//a escrever e 7D, seguido de XX, que tambem poderá estar stuffed)
					else if(buf == STUFF_D){
						data[cnt] = STUFF;
						//printf("\n7D 5D received, now unstuffed to FLAG\n");
						cnt++;
						STUFFED = FALSE;
						continue;
					}

					//a trama anterior 7D não era parte de byte stuffing
					else {
						data[cnt] = STUFF;
						cnt++;
						STUFFED = FALSE;
					}
				}
				
				//se nenhuma das situacoes anteriores se verificar, le normalmente
				data[cnt]=buf;
				cnt++;
				continue;

			}

			else if (STATE==5) {
				
				//printf("data:\n");
				//int k = 0;
				//for(k=0; k<=cnt;k++)
					//printf("%x\n",data[k]);
					
					
					
				unsigned char BCC_EXPECTED = data[0] ^ data[1];

				for(x = 2; x < cnt; x++) {
					BCC_EXPECTED ^= data[x];
				}

				printf("\nLeu BCC2");
				printf("Expected: %x   ", BCC_EXPECTED); printf("Sent: %x\n", data[cnt]);
				if((char)data[cnt] == (char)BCC_EXPECTED) {

					buf2[0] = FLAG;				
					buf2[1] = ADDR_RECEIVER;
					if(INFSTATE == FALSE){

						buf2[2] = CTRL_RR_0;
						buf2[3] = (ADDR_RECEIVER ^ CTRL_RR_0);					
					}

					else{
						buf2[2] = CTRL_RR_1;
						buf2[3] = (ADDR_RECEIVER ^ CTRL_RR_1);
					}
					buf2[4] = FLAG;
					
					//envio da flag de RR ao cliente

					tcflush(fd, TCIOFLUSH);
					sent = write(fd, buf2, 5);
					printf("\nRecebida trama com byte de controlo (BCC2) correcta.");
					printf("\nTrama de RR enviada: %i bytes : [%x,%x,%x,%x,%x]\n", sent, buf2[0],buf2[1],buf2[2],buf2[3],buf2[4]);

					if(DUPLICATE == FALSE)					
						return (cnt);
					else{ 
						printf("A trama recebida e duplicada!!!\n");
						ntramas_d++;
						STATE = 0;
					}
				} else {

					buf2[0] = FLAG;				
					buf2[1] = ADDR_RECEIVER;
					if(INFSTATE == FALSE){
						buf2[2] = CTRL_REJ_0;
						buf2[3] = (ADDR_RECEIVER ^ CTRL_REJ_0);
					}	
				
					else {
						buf2[2] = CTRL_REJ_1;
						buf2[3] = (ADDR_RECEIVER ^ CTRL_REJ_1);
					}
					buf2[4] = FLAG;
					
					//envio da flag de REJEICAO ao cliente
					sent = write(fd, buf2, 5);
					printf("\nRecebida trama com byte de controlo BCC2 errada (REJ).");
					printf("\nTrama de rejeição enviada: %i bytes\n", sent);

					ntramas_r++;
					
					if(DUPLICATE == TRUE){ 
						printf("A trama recebida e duplicada!!!\n");
						ntramas_d++;
					}
					STATE = 0;
				}
			}

			//Caso o protocolo não seja mantido, considera-se
			//que a trama era "lixo" e volta-se ao estado inicial
			//até receber uma trama completamente dentro do protocolo
			else STATE = 0;
			
		}
	}
	return -1;
}


int llclose (int fd){
	
	char buf, buf2[255];
	int STATE = 0, res = 0, sent = 0; //estado inicial
	
	buf2[0] = FLAG;				
	buf2[1] = ADDR_RECEIVER;
	buf2[2] = CTRL_DISC;
	buf2[3] = (ADDR_RECEIVER ^ CTRL_DISC);
	buf2[4] = FLAG;

	//envio da flag de desconecao ao cliente
	sent = write(fd, buf2, strlen(buf2)+1);
	
	/*printf("\n[%x, ", buf2[0]);
	printf("%x, ", buf2[1]);
	printf("%x, ", buf2[2]);
	printf("%x, ", buf2[3]);
	printf("%x]\n", buf2[4]);
	*/
	
	while (1) {
		res = read(fd,&buf,1);
		if (res != 0) { //leu dados
			
			//printf("%x", buf);

			//Flag
			if ((buf == FLAG) && (STATE == 0)) {
				//printf("\nFlag inicial OK!\n");
				STATE = 1;
				continue;
			}

			//Address (must be ADDR_SENDER)
			else if ((buf == ADDR_SENDER) && (STATE == 1)) { 
				//printf("ADDR_SEND OK!\n");
				STATE = 2;	
				continue;
			}
			
			//Control, must be CTRL_UA
			else if ((buf == CTRL_UA) && (STATE == 2)) { 
				//printf("CTRL_UA OK!\n");
				STATE = 3;
				continue;
			}

			//BCC1 (Head bcc, must be ADDR_SENDER ^ CTRL_UA)
			else if ((buf == (ADDR_SENDER ^ CTRL_UA)) && (STATE == 3)) {
				//printf("BCC OK!\n");
				STATE = 4;
				continue;
			}

			//Must be Flag on 5th byte, returns UA supervision message
			else if ((buf == FLAG) && (STATE==4)) {//0x7E
				printf("\nLigação de dados terminada correctamente.");
				printf("\n========================================");
				
				return 0;
			}
			
			//caso não seja verificada uma das condições anteriores, ao receber dados,
			//a ligação falha devido a má trama de ligação, logo o ciclo termina e llclose retorna -1 (erro)
			
		}
		
	} 
	return -1;
}
