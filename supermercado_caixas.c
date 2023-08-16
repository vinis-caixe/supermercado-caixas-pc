/*
Trabalho Final
Programação Concorrente
Aluno: Vinícius Caixeta de Souza
Matrícula: 180132199
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define N_CLIENTES 350
#define N_CAIXA_RAPIDO 3
#define N_CAIXA_NORMAL 9

// Representa o tamanho da fila em todos os caixas
int filas[N_CAIXA_RAPIDO + N_CAIXA_NORMAL];

pthread_t cliente[N_CLIENTES];
pthread_t caixa_rapido[N_CAIXA_RAPIDO];
pthread_t caixa_normal[N_CAIXA_NORMAL];

sem_t sem_caixa_rapido;  // Determina quantidade de caixas rapidos abertos
sem_t sem_caixa_normal;  // Determina quantidade de caixas normais abertos
sem_t sem_tamanho_fila[N_CAIXA_RAPIDO + N_CAIXA_NORMAL];  // Determina a quantidade de pessoas que podem entrar na fila do caixa
sem_t sem_caixa_atendendo[N_CAIXA_RAPIDO + N_CAIXA_NORMAL];  // Representa caixa esperando aparecer cliente
sem_t sem_cliente_no_caixa[N_CAIXA_RAPIDO + N_CAIXA_NORMAL];  // Representa cliente esperando caixa passando os produtos

// Garante exclusão mútua quando o cliente acessa o caixa
pthread_mutex_t caixa_mutex[N_CAIXA_RAPIDO + N_CAIXA_NORMAL] = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t posicoes_mutex = PTHREAD_MUTEX_INITIALIZER;  // Garante exclusão mútua ao acessar o vetor de filas
pthread_mutex_t prioridade_mutex = PTHREAD_MUTEX_INITIALIZER;  // Garante prioridade no vetor de filas para quando algum cliente sair do caixa

void* f_cliente(void* arg);
int menor_fila(int itens);
void* f_caixa_rapido(void* arg);
void* f_caixa_normal(void* arg);

int main(){
	int *id;
	int i;
	
	sem_init(&sem_caixa_rapido, 0, 1);  // Inicia com apenas um caixa rápido aberto
	sem_init(&sem_caixa_normal, 0, 2);  // Inicia com apenas dois caixa normais abertos
	
	for(i = 0; i < (N_CAIXA_RAPIDO + N_CAIXA_NORMAL); i++){
		filas[i] = -1;  // Todas as filas iniciam com -1 para mostrar que o caixa está fechado
		sem_init(&sem_caixa_atendendo[i], 0, 0);
		sem_init(&sem_cliente_no_caixa[i], 0, 0);
	}
	
	printf("---SUPERMERCADO ABRINDO---\n");
	for(i = 0; i < N_CAIXA_RAPIDO; i++){
		id = (int *) malloc(sizeof(int));
		*id = i;
		pthread_create(&caixa_rapido[i], NULL, f_caixa_rapido, (void *) (id));
	}
	for(i = 0; i < N_CAIXA_NORMAL; i++){
		id = (int *) malloc(sizeof(int));
		*id = i;
		pthread_create(&caixa_normal[i], NULL, f_caixa_normal, (void *) (id));
	}

	usleep(100000);

	for(i = 0; i < N_CLIENTES; i++){
		id = (int *) malloc(sizeof(int));
		*id = i;
		pthread_create(&cliente[i], NULL, f_cliente, (void *) (id));
	}
	
	for(i = 0; i < N_CLIENTES; i++){
		pthread_join(cliente[i], NULL);
	}

	return 0;
}

void* f_cliente(void* arg){
	int id = *(int*) arg;
	int itens = 5 + (rand() % 56);  // Cliente escolhe entre 5 a 60 itens
	int fila_caixa;


	sleep(rand() % 11);  // Clientes entram entre 0 a 10 segundos no supermercado 
	
	printf("Cliente %d entrou no supermercardo para comprar %d itens.\n", id, itens);
	
	sleep(5 + (5*(itens / 10)));  // Determina quanto tempo o cliente gastará pegando os itens
	
	printf("Cliente %d terminou de pegar os itens e esta indo para algum caixa.\n", id);
	
	// Garante exclusão mútua ao acessar o vetor de filas
	pthread_mutex_lock(&posicoes_mutex);
	// Este mutex garante prioridade para quando o cliente sair do caixa
	pthread_mutex_lock(&prioridade_mutex);
		fila_caixa = menor_fila(itens);  // Retorna o caixa com a menor fila
		printf("Cliente %d com %d itens vai pro caixa %d com tamanho %d.\n", id, itens, fila_caixa, filas[fila_caixa]);
		filas[fila_caixa]++;

		// Se a fila que o cliente estiver possuir mais de 5 pessoas é aberto mais um caixa
		if(filas[fila_caixa] >= 5 && itens <= 20){
			sem_post(&sem_caixa_rapido);
		}
		else if(filas[fila_caixa] >= 5 && itens > 20){
			sem_post(&sem_caixa_normal);
		}
	pthread_mutex_unlock(&prioridade_mutex);
	pthread_mutex_unlock(&posicoes_mutex);

	// Verifica se existe permissão para o cliente entrar no caixa
	if(sem_trywait(&sem_tamanho_fila[fila_caixa]) == 0){
		pthread_mutex_lock(&caixa_mutex[fila_caixa]);

		sem_post(&sem_tamanho_fila[fila_caixa]);  // Diminui o tamanho da fila

		pthread_mutex_lock(&prioridade_mutex);
			filas[fila_caixa]--;
		pthread_mutex_unlock(&prioridade_mutex);

		sem_post(&sem_caixa_atendendo[fila_caixa]);  // Acorda o caixa

		sem_wait(&sem_cliente_no_caixa[fila_caixa]);  // Espera o caixa atender

		pthread_mutex_unlock(&caixa_mutex[fila_caixa]);

		printf("\033[0;32mCliente %d conseguiu fazer suas compras e esta saindo do supermercardo. \033[0m\n", id);
	}
	else{
		printf("\033[0;31mCliente %d nao encontrou fila pequena, logo devolveu os itens e saiu do mercado. \033[0m\n", id);
		pthread_mutex_lock(&prioridade_mutex);
			filas[fila_caixa]--;
		pthread_mutex_unlock(&prioridade_mutex);
	}
	
	pthread_exit(0);
}

// Retorna o caixa com a menor fila de acordo com a quantidade de itens que o cliente pegou
int menor_fila(int itens){
	int menor = 100;
	int i, caixa;
	
	if(itens <= 20){
		for(i = 0; i < N_CAIXA_RAPIDO; i++){
			if(filas[i] > -1){
				if(filas[i] < menor){
					menor = filas[i];
					caixa = i;
				}
			}
		}

		return caixa;
	}
	else{
		for(i = N_CAIXA_RAPIDO; i < (N_CAIXA_RAPIDO + N_CAIXA_NORMAL); i++){
			if(filas[i] > -1){
				if(filas[i] < menor){
					menor = filas[i];
					caixa = i;
				}
			}
		}
		
		return caixa;
	}
}

void* f_caixa_rapido(void* arg){
	int id = *(int*) arg;
	
	sem_wait(&sem_caixa_rapido);  // Verifica se tem permissão para abrir

	printf("Caixa rapido %d esta aberto.\n",id);

	sem_init(&sem_tamanho_fila[id], 0, 10);  // Inicia as 10 permissões na sua fila

	filas[id] = 0;

	while(1){
		sem_wait(&sem_caixa_atendendo[id]);  // Espera algum cliente acordá-lo

		printf("Caixa rapido %d esta atendendo um cliente.\n", id);
		usleep(600000);

		sem_post(&sem_cliente_no_caixa[id]);  // Libera o cliente do caixa
	}
	
	pthread_exit(0);
}

void* f_caixa_normal(void* arg){
	int id = *(int*) arg;
	id = id + N_CAIXA_RAPIDO;
	
	sem_wait(&sem_caixa_normal);  // Verifica se tem permissão para abrir

	printf("Caixa normal %d esta aberto.\n",id);

	sem_init(&sem_tamanho_fila[id], 0, 10);  // Inicia as 10 permissões na sua fila
	
	filas[id] = 0;
	
	while(1){
		sem_wait(&sem_caixa_atendendo[id]);  // Espera algum cliente acordá-lo

		printf("Caixa normal %d esta atendendo um cliente.\n", id);
		usleep(1500000);

		sem_post(&sem_cliente_no_caixa[id]);  // Libera o cliente do caixa
	}
	
	pthread_exit(0);
}
