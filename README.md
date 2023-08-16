# Venda de Ingressos TP1
Trabalho da disciplina Programação Concorrente (PC) realizado no 2° semestre de 2020 na Universidade de Brasília (UnB).
O programa simula caixas de um supermercado para atender certa quantidade de clientes, nele existem caixas normais e rápidos que podem ser abertos ou fechados dependendo da demanda. Os clientes tentam entrar no caixa adequado com menor fila e caso todos os caixas estejam cheios o cliente devolve os itens e vai embora.
O trabalho utiliza mecanismos de sincronização entre processos para simular os caixas. Ele foi desenvolvido em linguagem C em sistema operacional Ubuntu usando a biblioteca POSIX Pthreads.
- Aluno: Vinícius Caixeta de Souza
- Matrícula: 180132199
- Versão do GCC: 11.4.0

Para compilar o código e executar o programa digite no terminal:
```
gcc -pthread -o caixas supermercado_caixas.c
./caixas
```