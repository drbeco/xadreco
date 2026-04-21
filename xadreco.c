//-----------------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%% Direitos Autorais segundo normas do codigo-livre: (GNU - GPL)
//%% Universidade Estadual Paulista - UNESP, Universidade Federal de Pernambuco - UFPE
//%% Jogo de xadrez: xadreco
//%% Arquivo xadreco.c
//%% Tecnica: MiniMax
//%% Autor: Ruben Carlo Benante
//%% Contato: rcb@beco.cc
//%% Criacao: 19/08/1994
//%% Atualizacao: 27/03/1997
//%% Atualizacao: 10/06/1999
//%% Atualizacao para XBoard: 26/09/2004
//%% Atualizacao versao 6.1: 2026-04-15
//%% Atualizacao edicao v7.x: 2026-04-15
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//-----------------------------------------------------------------------------
/***************************************************************************
 *   xadreco version 6.1. Chess engine compatible                          *
 *   with XBoard Protocol (C)                                              *
 *   Copyright (C) 1994-2026 by Ruben Carlo Benante <rcb@beco.cc>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 * To contact the author, please write to:                                 *
 * Ruben Carlo Benante.                                                    *
 * email: rcb@beco.cc                                                      *
 * web page:                                                               *
 * http://xadreco.beco.cc/                                                 *
 ***************************************************************************/

/* ---------------------------------------------------------------------- */
/* includes */
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> /* get options from system argc/argv */
#include <stdarg.h> /* function with multiple arguments */

/* ---------------------------------------------------------------------- */
/* definitions */

#define SQ(c, r)  ((c) + (r) * 8)
#define COL(sq)   ((sq) & 7)
#define ROW(sq)   ((sq) >> 3)

//Versao do programa
#ifndef VERSION /* gcc -DVERSION="0.1" */
#define VERSION "6.x" /**< Fallback version if makefile doesn't set it */
#endif

#ifndef BUILD
#define BUILD "19940819.190934"
#endif
#define TRUE 1
#define FALSE 0

/* Debug */
#ifndef DEBUG /* gcc -DDEBUG=1 */
#define DEBUG 0 /**< Activate/deactivate debug mode */
#endif

#if DEBUG==0
#define NDEBUG
#endif
#include <assert.h> /* Verify assumptions with assert. Turn off with #define NDEBUG */

/** @brief Debug message if DEBUG on */
#define IFDEBUG(M) if(DEBUG) fprintf(stderr, "# [DEBUG file:%s line:%d]: " M "\n", __FILE__, __LINE__); else {;}

/* Command line defaults */
#ifndef RANDOM
#define RANDOM -1
#endif
#ifndef NOWAIT
#define NOWAIT 0
#endif
#ifndef XDEBUG
#define XDEBUG 0
#endif
#ifndef XDEBUGFOUT
#define XDEBUGFOUT stderr
#endif

//estimativa da duracao do jogo
#define TOTAL_MOVIMENTOS 50
//margem para usar a estatica preguicosa
#define MARGEM_PREGUICA 400
//a porcentagem de movimentos considerados, do total de opcoes
#define PORCENTO_MOVIMENTOS 0.85
//usada para indicar que o tempo acabou, na busca minimax
#define FIMTEMPO 106550
//define o limite para a funcao de avaliacao estatica
#define LIMITE 105000
//define o valor do xequemate
#define XEQUEMATE 100000
//numero do movimento que a maquina pode pedir empate pela primeira vez
#define MOVE_EMPATE1 52
//numero do movimento que a maquina pode pedir empate pela segunda vez
#define MOVE_EMPATE2 88
// pede empate se tiver menos que 2 PEOES
#define QUANTO_EMPATE1 -200
//pede empate se lances > MOVE_EMPATE2 e tem menos que 0.2 PEAO
#define QUANTO_EMPATE2 20
// geramodo de geramov: gera todos, gera unico (confere afogado), gera deste (0-63)
#define GERA_TUDO  -1
#define GERA_UNICO -2
// geramodo 0-63: gera apenas lances da casa 'deste' (otimiza valido)
// profundidade maxima de busca (limita mel[] e melhor)
#define MAX_PROF 64
// Cores: brancas=0, pretas=bit3. Pecas: brancas 1-6, pretas 9-14, vazia 0.
#define BRANCO 0                         // cor branca
#define PRETO  8                         // cor preta (bit 3)
#define EHBRANCA(p) ((p) != 0 && !((p) & 8)) // peca eh branca? (1-6)
#define EHPRETA(p)  ((p) & 8)            // peca eh preta? (9-14)
#define EHVAZIA(p)  ((p) == 0)           // casa vazia?
#define TIPO(p)     ((p) & 7)            // tipo da peca: PEAO=1..REI=6
#define COR(p)      ((p) & 8)            // cor da peca: 0=branca, 8=preta
#define DACOR(p, c) ((p) | (c))          // monta peca com tipo p e cor c
#define ICOR(c)     ((c) >> 3)           // indice da cor: 0=branca, 1=preta
#define ADV(v)      ((v) ^ 8)            // adversario: troca branco/preto
// tamanho das arenas em bytes
#define ARENA_TAB  (2 * 1024 * 1024)
#define ARENA_MOV  (2 * 1024 * 1024)

// dados ----------------------

/* ---------------------------------------------------------------------- */
/* typedefs, enums and structures */

// arena ----------------------
typedef struct sarena
{
    char *ptr; // ponteiro para area alocada
    size_t usado; // quantidade de bytes
    size_t total; // total alocado
    void (*destrutor)(struct sarena *); // funcao destrutora (opcional)
}
arena;

// nodo generico para listas
typedef struct sno
{
    void *info; // conteudo do nodo
    size_t tam; // tamanho do nodo apontado por info
    struct sno *prox; // ponteiro para o proximo (dentro da arena)
    struct sno *ant; // ponteiro para o anterior (dentro da arena)
}
no;

// lista generica encadeamento simples/duplo
typedef struct slista
{
    no *cabeca; // primeiro no ou cabeca=cauda=null
    no *cauda; // ultimo no ou cabeca=cauda=null
    int qtd; // quantidade de nos na lista, ou zero se cabeca=cauda=null
    arena *a; // auto-referencia ao armazenamento da lista
    struct slista **pl; //auto-referencia para a propria lista
}
lista;

// Unificar (POSTPONE)
// tabu.situa, tabu.empate_50, tabu.especial
// movi.flag_50, movi.especial
//
//
// tabu.roqueb, tabu.roquep
// movi.roque
// tabu.peao_pulou
// movi.peao_pulou

typedef struct stabuleiro
{
    int tab[64]; //contem as pecas: brancas 1-6, pretas 9-14, vazia 0. SQ(col,lin)=col+lin*8
    int vez; //contem BRANCO(0) ou PRETO(8)
    int peao_pulou; //contem coluna do peao adversario que andou duas, ou -1 para 'nao pode comer enpassant'
    int roqueb, roquep; //roque: 0:nao pode mais. 1:pode ambos. 2:mexeu Torre do Rei. Pode O-O-O. 3:mexeu Torre da Dama. Pode O-O.
    float empate_50; //contador: quando chega a 50, empate.
    int situa; //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.) 7: * sem resultado
    int de; //lance executado de=origem (0-63)
    int pa; //lance executado pa=destino (0-63)
    int especial; //especial 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque 9=captura
    int meionum; //meionum: meio-numero (ply/half-move): 0=inicio, 1=e2e4, 2=e7e5, 3=g1f3, 4=b8c6
    int rei_pos[2]; // cache da posicao dos reis para xeque_rei_das()
    // flags
}
tabuleiro;

typedef struct smovimento
{
    //lance notacao inteira de/pa: a1=0, h1=7, a8=56, h8=63. SQ(col,row)=col+row*8, COL(sq)=sq&7, ROW(sq)=sq>>3 (0-63, indices de tab[64])
    int de; //lance notacao inteira: de=origem
    int pa; //lance notacao inteira: pa=destino
    int peao_pulou; //coluna do peao que andou duas neste lance
    // flags
    int roque; //roque: 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
    int flag_50; //flag_50: 0=nada, 1=Moveu peao, 2=Comeu, 3=Peao comeu. Zera empate_50.
    // flag_50 reseta apenas com movimento de peoes e capturas, nada mais (regra FIDE art.9.3)
    int especial; //especial: 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque 9=captura
    int valor_estatico; //valor estatico deste tabuleiro apos a execucao deste movimento
}
movimento;

//resultado de uma analise de posicao
typedef struct sresultado
{
    movimento linha[MAX_PROF]; //melhor linha de movimentos
    int tamanho;               //tamanho da linha
    int valor;                 //valor da linha
}
resultado;

// identidade das pecas (mesma para brancas e pretas)
enum piece_identity
{
    VAZIA = 0, PEAO = 1, CAVALO = 2, BISPO = 3, TORRE = 4, DAMA = 5, REI = 6, NULA = 7
};

int val[] = {0, 100, 300, 325, 500, 900, 10000}; // centipawn lookup by TIPO index

/* ---------------------------------------------------------------------- */
/* globals */

// listas em arenas ----------------------------------
lista *pltab = NULL; // ponteiro para lista de tabuleiros
lista *plmov = NULL; // ponteiro para a primeira lista de movimentos de uma sequencia de niveis a ser analisadas (antiga succ_geral)
// array triangular de PV: mel[prof] = melhor linha na profundidade prof
resultado mel[MAX_PROF];
// melhor linha salva entre iteracoes (o pote de mel)
resultado melhor;
// posicao inicial do tabuleiro (constante unica, evita duplicacao)
const tabuleiro TAB_INICIO =
{
    {   /* tab[64]: SQ(col,row)=col+row*8, rank by rank a-h */
        DACOR(TORRE,BRANCO), DACOR(CAVALO,BRANCO), DACOR(BISPO,BRANCO), DACOR(DAMA,BRANCO), DACOR(REI,BRANCO), DACOR(BISPO,BRANCO), DACOR(CAVALO,BRANCO), DACOR(TORRE,BRANCO), /* rank 1: a1-h1 */
        DACOR(PEAO,BRANCO),  DACOR(PEAO,BRANCO),   DACOR(PEAO,BRANCO),  DACOR(PEAO,BRANCO), DACOR(PEAO,BRANCO), DACOR(PEAO,BRANCO), DACOR(PEAO,BRANCO),   DACOR(PEAO,BRANCO), /* rank 2: a2-h2 */
        VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA,  /* rank 3 */
        VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA,  /* rank 4 */
        VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA,  /* rank 5 */
        VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA, VAZIA,  /* rank 6 */
        DACOR(PEAO,PRETO),  DACOR(PEAO,PRETO),   DACOR(PEAO,PRETO),  DACOR(PEAO,PRETO), DACOR(PEAO,PRETO), DACOR(PEAO,PRETO), DACOR(PEAO,PRETO),   DACOR(PEAO,PRETO), /* rank 7: a7-h7 */
        DACOR(TORRE,PRETO), DACOR(CAVALO,PRETO), DACOR(BISPO,PRETO), DACOR(DAMA,PRETO), DACOR(REI,PRETO), DACOR(BISPO,PRETO), DACOR(CAVALO,PRETO), DACOR(TORRE,PRETO)  /* rank 8: a8-h8 */
    },
    BRANCO, -1, 1, 1, 0, 0, // vez=BRANCO, peao_pulou=-1, roqueb=1, roquep=1, empate_50=0, situa=0
    0, 0,              // de=0, pa=0
    0, 0,              // especial=0, meionum=0
    {SQ(4, 0), SQ(4, 7)} // rei_pos: white e1, black e8
};

// outros globais ------------------------------------
//my rating and opponent rating
int myrating, opprating;
//log file for debug==2
FILE *fmini;
//1:consulta o livro de aberturas livro.txt. 0:nao consulta
int USALIVRO;
//o computador pode oferecer empate (duas|uma) vezes.
int ofereci;
//sem uso. variavel de descarte, para nao sujar o stdin, dic=pega()
char disc;
//mostra as linhas que o computador esta escolhendo
int mostrapensando = 0;
//primeiro e segundo jogadores: h:humano, c:computador
char primeiro = 'h', segundo = 'c';
//0 nao analise. 1 para analise. ("exit" coloca ela como 0 novamente)
int analise = 0;
// 0: pensa para jogar. 1: joga ao acaso.
int randomchess = 0;
//-1 para primeira vez. 0 nao edita. 1 edita. Posicao FEN.
int setboard = 0;
//armazena o ultimo resultado, ex: 1-0 {White mates}
char ultimo_resultado[80] = "";
//pause entre lances (computador x computador)
char pausa;
//nivel de profundidade (agora com aprofundamento iterativo, esta sem uso)
int nivel = 3;
//Flag de captura ou xeque para liberar mais um nivel em profsuf
int profflag = 1;
//total de nodos analisados para fazer um lance
int totalnodo = 0;
//total de nodos analisados em um nivel da arvore
int totalnodonivel = 0;

// controle de tempo
time_t tinijogo, tinimov, tatual, tultimoinput;
double tbrancasac, tpretasac; //tempo brancas/pretas acumulado
double tdifs; // diferenca em segundos tdifs=difftime(t2,t1), calcular o tempo gasto no minimax e outras
//3000 miliseg = 3 seg por lance = 300 seg por 100 lances = 5 min por jogo (de 100 lances)
double tempomovclock; //em segundos
double tempomovclockmax; // max allowed
//Nome do arquivo de log
char flog[80] = "";
//abandona a partida quando perder algo como DAMA, 2 TORRES, 1 BISPO e 1 PEAO; ou nunca, quando jogando contra outra engine
int ABANDONA = -2430;
//flag que diz se esta jogando contra outra engine.
int COMPUTER = 0;
//flag que diz que o comando "?" foi executado
//int teminterroga = 0;

char bookfname[80] = "livro.txt"; /* book file name */
/* int verb = 0; /1* verbose variable *1/ */
//coloque zero para evitar gravar arquivo. 0:sem debug, 1:-v debug, 2:-vv debug minimax
int debug = 0; /* BUG: trocar por DEBUG - usando chave -v */
int pong; /* what to ping back */
/* se decidir oferecer empate, primeiro manda o lance, depois a oferta */
int OFERECEREMPATE = 0;

/* ---------------------------------------------------------------------- */
/* prototipos gerais */ /* general prototypes */
//imprime o tabuleiro
void imptab(tabuleiro tabu);
void mostra_tabu(tabuleiro tabu);
//mostra na tela informacoes do jogo e analises
void mostra_lances(tabuleiro tabu);
//transforma lances int 0077 em char tipo a1h8
void lance2movi(char *m, int de, int pa, int especial);
//faz o contrario: char b1c3 em int 1022. Retorna falso se nao existe.
int movi2lance(int *de, int *pa, char *m);
//retorna o adversario: ADV(v) macro em defines
//pegar caracter (linux e windows)
char pega(char *no, char *msg);
/* retorna x2 pertencente a (min2,max2) equivalente a x1 pertencente a (min1,max1) */
float mudaintervalo(float min1, float max1, float min2, float max2, float x1);
/* retorna valor inteiro aleatorio entre [min,max[ */
int irand_minmax(int min, int max);
//termina o programa
void sai(int error);
//para inicializar alguns valores no inicio do programa
void inicia(tabuleiro *tabu);
//zera pecas do tabuleiro para setboard preencher via FEN
void zera_pecas(tabuleiro *tabu);
//testa posicao dada. devera ser melhorado.
void testapos(char *pieces, char *color, char *castle, char *enpassant, char *halfmove, char *fullmove);
//retorna um lance do jogo de teste
void testajogo(char *movinito, int mnum);
//limpa algumas variaveis para iniciar a ponderacao
void limpa_pensa(void);
//preenche a estrutura movimento usando arena e lst_insere
//Argumentos:
//   c0c1-c2c3: lance
//   pp peao_pulou: nao -1 ou coluna do peao que andou duas neste lance
//   rr roque: 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
//   ee especial: 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque 9=captura
//   ff flag_50: 0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
void enche_lmovi(lista *lmov, int de, int pa, int pp, int rr, int ee, int ff);
//mensagem antes de sair do programa (por falta de memoria etc, ou tudo ok)
void msgsai(char *msg, int error);
//imprime uma sequencia de lances armazenada em resultado, numerados.
void imprime_linha(resultado *res, int mnum, int vez);
// retorna verdadeiro se existe algum caracter no buffer para ser lido
int pollinput(void);
//calcula diferenca de tempo em segundo do lance atual
double difclocks(void);
// joga aleatorio!
char randommove(tabuleiro *tabu);
/* imprime o help e termina */
void help(void);
/* imprime mensagem de copyright */
void copyr(void);
/* imprime info debug */
void printdbg(int dbg, ...);
/* imprime outros debugs */
void printf2(char *fmt, ...);
/* le entrada padrao */
void scanf2(char *movinito);

// apoio xadrez -----------------------------------------------------
//retorna 1 se "cor" ataca casa(col,lin) no tabuleiro tabu
int ataca(int cor, int col, int lin, tabuleiro tabu);
//retorna o numero de ataques da "cor" na casa(col,lin) de tabu, *menor e a menor peca que ataca.
int qataca(int cor, int col, int lin, tabuleiro tabu, int *menor);
//retorna 1 se o rei de cor "cor" esta em xeque
int xeque_rei_das(int cor, tabuleiro tabu);
//para voltar um movimento. Use duas vezes para voltar um lance inteiro.
void volta_lance(tabuleiro *tabu);
//analisa uma posicao mas nao joga
char analisa(tabuleiro *tabu);
//procura nos movimentos de geramov se o lance em questao eh valido. Retorna o *movimento preenchido. Se nao, retorna NULL.
int valido(tabuleiro tabu, int de, int pa, movimento *result);
//retorna char que indica a situacao do tabuleiro, como mate, empate, etc...
char situacao(tabuleiro tabu);
// livro --------------------------------------------------------------
//preenche melhor com uma variante do livro, baseado na posicao do tabuleiro tabu
void usalivro(tabuleiro tabu);
//pega a lista de tabuleiros e cria uma string de movimentos, como "e1e2 e7e5"
void listab2string(char *strlance);
//le string linha do livro de aberturas e preenche melhor.linha e melhor.tamanho
void livro_linha(int mnum, char *linha);
//retorna verdadeiro se o jogo atual strlances casa com a linha do livro atual strlinha
int igual_strlances_strlinha(char *strlances, char *strlinha);
// computador joga ----------------------------------------------------------
//retorna lista de lances possiveis, ordenados por xeque e captura. Deveria ser uma ordem melhor aqui.
int geramov(tabuleiro tabu, lista *lmov, int geramodo);
//retorna (int) valor. Escreve mel[prof] com a melhor linha.
int minimax(tabuleiro atual, int prof, int alfa, int beta, int niv);
//retorna verdadeiro se (prof>=niv) ou (prof>=MAX_PROF) ou (tempo estourou)
int profsuf(tabuleiro atual, int prof, int alfa, int beta, int niv, int *valor);
//retorna um valor estatico que avalia uma posicao do tabuleiro, fixa. Cod==1: tempo estourou no meio da busca. Niv: o nivel de distancia do tabuleiro real para a copia examinada
int estatico(tabuleiro tabu, int cod, int niv, int alfa, int beta);
//joga o movimento movi em tabuleiro tabu. retorna situacao. Insere no listab *plfinal se cod==1
char joga_em(tabuleiro *tabu, movimento movi, int cod);

// listas dinamicas com arena ------------------------------------------------------------
void arena_inicia(arena *a, size_t capa); // inicializa uma arena de alocacao de memoria
void arena_destroi(arena *a); // desaloca area de memoria da arena
char *arena_aloca(arena *a, size_t tam); // reserva um espaco na area da arena
void arena_libera(arena *a, size_t tam); // libera apenas o ultimo item da area reservada na arena
void arena_zera(arena *a); // libera toda area reservada na arena (simula loop free all)
void arena_destrutor(arena *a, void (*destrutor)(arena *)); // callback para limpar pltab

int lst_cria(arena *a, lista **pl); // cria uma lista alocada em uma arena. retorna 0 ok, -1 falha
void lst_zera(lista *l); // libera reserva de uma lista alocada em uma arena
int lst_insere(lista *l, void *info, size_t tam); // insere um item ao final de uma lista (append). retorna 0 ok, -1 falha
void lst_remove(lista *l); // remove o ultimo item da lista
int lst_conta(lista *l); // conta o numero de elementos em uma lista
void lst_limpa(arena *a); // limpa pointeiro externo da lista
void lst_furafila(lista *l, no *n); // desencaixa no e reinsere na cabeca da lista
void lst_recria(lista **pl); // zera arena e recria lista do inicio
void lst_parte(lista *l); // particiona: capturas e especiais primeiro
void lst_ordem(lista *l); // ordena por valor_estatico decrescente

// prototipos listas dinamicas -----------------------------------------------------------
//insere tabuleiro na arena pltab. Retorna contagem de repeticao (>=3 empate)
int tab_insere(tabuleiro tabu);
//copia font para dest. dest=font.
void copitab(tabuleiro *dest, tabuleiro *font);

// turnos -----------------------------------------------------------
//humano joga. Aceita comandos XBoard/WinBoard.
char humajoga(tabuleiro *tabu);
//computador joga. Chama o livro de aberturas ou o minimax.
char compjoga(tabuleiro *tabu);

/* ---------------------------------------------------------------------- */
/* codigo principal - main code */
int main(int argc, char *argv[])
{
    // gerenciamento de memoria com arenas
    arena atab; // historico de posicoes de tabuleiro do jogo
    arena_inicia(&atab, ARENA_TAB);
    arena_destrutor(&atab, lst_limpa); // callback para limpar pltab
    if(lst_cria(&atab, &pltab))
        msgsai("# Erro arena cheia em main lst_cria atab", 39);

    arena amov; // movimentos gerados para analise
    arena_inicia(&amov, ARENA_MOV);
    arena_destrutor(&amov, lst_limpa); // callback para limpar plmov
    if(lst_cria(&amov, &plmov))
        msgsai("# Erro arena cheia em main lst_cria amov", 39);

    int opt; /* return from getopt() */
    tabuleiro tabu;
    char feature[256];
    char res;
    char movinito[80] = "wait_xboard"; /* scanf : entrada de comandos ou lances */
    int d2 = 0; /* wait for 2 dones */
    // int joga; /* flag enquanto joga */
    struct tm *tmatual;
    char hora[] = "2013-12-03 00:28:21";
    int seed = 0;

    srand(time(NULL) + getpid());
    IFDEBUG("Starting optarg loop...");

    /* getopt() configured options:
     *        -h  help
     *        -V  version
     *        -v  verbose
     */
    /* Usage: xadreco [-h|-v] [-r] [-x] [-b path/bookfile.txt] */
    /* -r seed,  --random seed        Xadreco plays random moves. Initializes seed. If seed=0, 'true' random */
    /* -x,  --xboard                  Gives 'xboard' keyword immediately */
    /* -b path/bookfile.txt, --book   Sets the path for book file */
    opterr = 0;
    while((opt = getopt(argc, argv, "vhVr:xb:")) != EOF)
        switch(opt)
        {
            case 'h':
                help();
                break;
            case 'V':
                copyr();
                break;
            case 'v':
                debug++;
                break;
            case 'r':
                seed = atoi(optarg);
                if(seed)
                    srand(seed);
                randomchess = 1;
                break;
            case 'x': /* no wait for first xboard keyword */
                strcpy(movinito, "xboard");
                d2 = 2; /* don't wait for done */
                break;
            case 'b': /* book file name */
                strcpy(bookfname, optarg);
                break;
            case '?':
            default:
                printf("Type\n\t$man %s\nor\n\t$%s -h\nfor help.\n\n", argv[0], argv[0]);
                return EXIT_FAILURE;
        }
    /* RANDOM >= 0, defined YES at compiler time */
    /* RANDOM < -1, defined NO at compiler time */
    /* Otherwise, RANDOM == -1 defined by command line */
#if RANDOM >= 0
    seed = RANDOM;
    if(seed)
        srand(seed);
    randomchess = 1;
#endif
#if RANDOM < -1
    randomchess = 0;
#endif

    /* if NOWAIT == 1, defined to NOT wait at compiler time */
#if NOWAIT == 1
    strcpy(movinito, "xboard");
    d2 = 2; /* don't wait for done */
#endif

    /* if XDEBUG<=0, command line -vvv..., otherwise debug=XDEBUG */
#if XDEBUG > 0
    debug = XDEBUG;
#endif

    printdbg(debug, "# DEBUG MACRO compiled value: %d\n", DEBUG);
    printdbg(debug, "# Debug verbose level set at: %d\n", debug);
    printdbg(debug, "# play random: %s. seed: -r %d\n", randomchess ? "yes" : "no", seed);
    printdbg(debug, "# wait: -x %s\n", movinito);
    printdbg(debug, "# book: -b %s\n", bookfname);

    //turn off buffers. Immediate input/output.
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    printdbg(debug, "# Xadreco version %s build %s (C) 1994-2026, by Dr. Beco\n"
             "# Xadreco comes with ABSOLUTELY NO WARRANTY;\n"
             "# This is free software, and you are welcome to redistribute it\n"
             "# under certain conditions; Please, visit http://www.fsf.org/licenses/gpl.html\n"
             "# for details.\n\n", VERSION, BUILD);

    /* 'xboard': scanf if not there yet */
    if(strcmp("xboard", movinito))
        scanf2(movinito);

    if(strcmp(movinito, "xboard"))   // primeiro comando: xboard
    {
        /* printdbg(debug, "# xboard: %s\n", movinito); */
        msgsai("# xadreco : xboard command missing.\n", 36);
    }
    printf2("\n"); /* output a newline when xboard comes in */
    sleep(1);

    /* Xadreco 5.8 accepts Xboard Protocol V2 */
    sprintf(feature, "%s", "feature ping=1 setboard=1 playother=1 san=0 usermove=0 time=1 draw=1 sigint=0 sigterm=1 reuse=1 analyze=1 variants=\"normal\" colors=0 ics=1 name=0 pause=0 nps=0 debug=1 memory=0 smp=0 exclude=0 setscore=0");


    printf2("feature done=0\n");
    printf2("feature myname=\"Xadreco %s\"\n", VERSION);
    printf2("%s\n", feature);
    printf2("feature done=1\n");

    /* comandos que aparecem no inicio, terminando com done
     * ignorar todos exceto: quit e post, ate contar dois 'done'
     */
    while(d2 < 2)
    {
        scanf2(movinito);

        if(!strcmp(movinito, "quit"))
            msgsai("# Thanks for playing Xadreco.", 0);
        //protover N = versao do protocolo
        //new = novo jogo, brancas iniciam
        //hard, easy = liga/desliga pondering (pensar no tempo do oponente). Ainda nao implementado.

        if(!strcmp(movinito, "easy"))   //showthinking ou mostrapensando
        {
            analise = 0;
            mostrapensando = 0;
            printdbg(debug, "# xboard: easy. Xadreco stop thinking. (1)\n");
        }
        else
            if(!strcmp(movinito, "post"))   //showthinking ou mostrapensando
            {
                analise = 0;
                mostrapensando = 1;
                printdbg(debug, "# xboard: post. Xadreco will show what its thinking. (1)\n");
            }
            else
                if(!strcmp(movinito, "done"))
                    d2++;
                else
                    if(!strcmp(movinito, "ping"))
                    {
                        scanf2(movinito);
                        pong = atoi(movinito);
                        printf2("pong %d\n", pong);
                    }
                    else
                        if(!strcmp(movinito, "force")) /* lichess don't send 'accept', goes by 'force' */
                            break;
                        else
                            if(!strcmp(movinito, "new")) /* python-chess sends new without done */
                                break;
                            else
                                printdbg(debug, "# xboard: ignoring %s\n", movinito);
    } /* main while starting xboard protocol */
    printdbg(debug, "# xboard: main while done\n");

    /* joga==0, fim. joga==1, novo lance. (joga==2, nova partida) */
    //------------------------------------------------------------------------------
    // novo jogo
    inicia(&tabu);  // zera variaveis
    assert(tabu.vez == BRANCO && "board not initialized");
    tab_insere(tabu);
    assert(pltab != NULL && pltab->cabeca != NULL && "board history null");
    //------------------------------------------------------------------------------
    //joga_novamente: (play another move)

    while(TRUE)
    {
        tatual = time(NULL);
        // printdbg(debug, "# xadreco : Tempo atual %s", ctime(&tatual)); //ctime returns "\n"
        tmatual = localtime(&tatual); // convert time_t to struct tm
        /* strftime(hora, sizeof(hora), "%F %T", tmatual); */ /* not accepted in that other operating xystem */
        strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tmatual);
        if(tabu.meionum == 2) //pretas jogou o primeiro. Relogios iniciam
        {
            tinijogo = tinimov = tatual;
            printdbg(debug, "# xadreco : N.2. Relogio ligado em %s\n", hora);  //ctime(&tinijogo));
        }
        if(tabu.meionum > 2)
        {
            tdifs = difftime(tatual, tinimov);
            if(tabu.vez == BRANCO) //iniciando vez das brancas
            {
                tpretasac += tdifs; //acumulou lance anterior, das pretas
                printdbg(debug, "# xadreco : N.%d. Pretas. Tempo %fs. Acumulado: %fs. Hora: %s\n", tabu.meionum, tdifs, tpretasac, hora);
            }
            else
            {
                tbrancasac += tdifs;
                printdbg(debug, "# xadreco : N.%d. Brancas. Tempo %fs. Acumulado: %fs. Hora: %s\n", tabu.meionum, tdifs, tbrancasac, hora);
            }
            tinimov = tatual; //ancora para proximo tempo acumulado
        }

        if(tabu.vez == BRANCO)  /* jogam as brancas */
            if(primeiro == 'c')
                res = compjoga(&tabu);
            else
                res = humajoga(&tabu);
        else                     /* jogam as pretas */
            if(segundo == 'c')
                res = compjoga(&tabu);
            else
                res = humajoga(&tabu);

        if(res != 'w' && res != 'c') /* BUG : porque ? */
            imptab(tabu);
        switch(res)
        {
            case 'w': //Novo jogo
                inicia(&tabu);  // zera variaveis
                tab_insere(tabu);
                break;
            case '*':
                strcpy(ultimo_resultado, "* {Game was unfinished}");
                printf2("* {Game was unfinished}\n");
                primeiro = segundo = 'h';
                break;
            case 'M':
                strcpy(ultimo_resultado, "0-1 {Black mates}");
                printf2("0-1 {Black mates}\n");
                primeiro = segundo = 'h';
                break;
            case 'm':
                strcpy(ultimo_resultado, "1-0 {White mates}");
                printf2("1-0 {White mates}\n");
                primeiro = segundo = 'h';
                break;
            case 'a':
                strcpy(ultimo_resultado, "1/2-1/2 {Stalemate}");
                printf2("1/2-1/2 {Stalemate}\n");
                primeiro = segundo = 'h';
                break;
            case 'p':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by endless checks}");
                printf2("1/2-1/2 {Draw by endless checks}\n");
                primeiro = segundo = 'h';
                break;
            case 'c':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by mutual agreement}");  //aceitar empate
                printdbg(debug, "# xadreco : offer draw, draw accepted\n");
                printf2("offer draw\n");
                printf2("1/2-1/2 {Draw by mutual agreement}\n");
                primeiro = segundo = 'h';
                break;
            case 'i':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by insufficient mating material}");
                printf2("1/2-1/2 {Draw by insufficient mating material}\n");
                primeiro = segundo = 'h';
                break;
            case '5':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by fifty moves rule}");
                printf2("1/2-1/2 {Draw by fifty moves rule}\n");
                primeiro = segundo = 'h';
                break;
            case 'r':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by triple repetition}");
                printf2("1/2-1/2 {Draw by triple repetition}\n");
                primeiro = segundo = 'h';
                break;
            case 'T':
                strcpy(ultimo_resultado, "0-1 {White flag fell}");
                printf2("0-1 {White flag fell}\n");
                primeiro = segundo = 'h';
                break;
            case 't':
                strcpy(ultimo_resultado, "1-0 {Black flag fell}");
                printf2("1-0 {Black flag fell}\n");
                primeiro = segundo = 'h';
                break;
            case 'B':
                strcpy(ultimo_resultado, "0-1 {White resigns}");
                printf2("0-1 {White resigns}\n");
                primeiro = segundo = 'h';
                break;
            case 'b':
                strcpy(ultimo_resultado, "1-0 {Black resigns}");
                printf2("1-0 {Black resigns}\n");
                primeiro = segundo = 'h';
                break;
            case 'e': //se existe um resultado, envia ele para finalizar a partida
                if(ultimo_resultado[0] != '\0')
                {
                    printdbg(debug, "# xadreco : case 'e' (empty) %s\n", ultimo_resultado);
                    printf2("%s\n", ultimo_resultado);
                    primeiro = segundo = 'h';
                }
                else
                {
                    printdbg(debug, "# xadreco (main) : I really don't know what to play... resigning!\n");
                    printf2("resign\n");
                    if(primeiro == 'c')
                    {
                        res = 'B';
                        strcpy(ultimo_resultado, "0-1 {White resigns}");
                        printf2("0-1 {White resigns}\n");

                    }
                    else
                    {
                        res = 'b';
                        strcpy(ultimo_resultado, "1-0 {Black resigns}");
                        printf2("1-0 {Black resigns}\n");
                    }
                    primeiro = segundo = 'h';
                }
                break;
            case 'x': //xeque: joga novamente
            case 'y': //retorna y: simplesmente gira o laco para jogar de novo. Troca de adv.
            default: //'-' para tudo certo...
                //       primeiro = segundo = 'h';
                break;
        } //fim do switch(res)
    } //while (TRUE)

    msgsai("# out of main loop...\n", 0);  //vai apenas para o log, mas nao para a saida
    return EXIT_SUCCESS;
}


// mostra a lista de lances da tela, a melhor variante, e responde ao comando "hint"
void mostra_lances(tabuleiro tabu)
{
//     struct movimento *pmovi, *loop;
//     char m[80];
//     int linha = 1, coluna = 34, nmov = 0;
//     m[0] = '\0';
    //nivel pontuacao tempo totalnodo variante
    //nivel esta errado!
//    printf ("# xadreco : ply score time nodes pv\n");
    printf2("%3d %+6d %3d %7d ", nivel, melhor.valor, (int)difclocks(), totalnodo);
    //melhor.valor*10 o xboard divide por 100. centi-peao
    //melhor.valor/100 para a Dama ficar com valor 9
    imprime_linha(&melhor, tabu.meionum, tabu.vez);
//    fflush(stdout);
}

//fim do mostra_lances

// imprime o movimento
// funcao intermediaria chamada no intervalo humajoga / compjoga
void imptab(tabuleiro tabu)
{
    char movinito[80];
    movinito[79] = '\0';
    lance2movi(movinito, tabu.de, tabu.pa, tabu.especial);
    /* imprime o movimento */
    if((tabu.vez == BRANCO && segundo == 'c') || (tabu.vez == PRETO && primeiro == 'c'))
    {
        printf2("move %s\n", movinito);
        if(OFERECEREMPATE == 1)
        {
            printf2("offer draw\n");
            OFERECEREMPATE = 0;
        }
    }
    if(debug) mostra_tabu(tabu);
}

//mostra tabuleiro em ascii no stderr (debug)
void mostra_tabu(tabuleiro tabu)
{
    int col, row, p, ap;
    char c;

    fprintf(stderr, "  a b c d e f g h\n");
    for(row = 7; row >= 0; row--)
    {
        fprintf(stderr, "%d ", row + 1);
        for(col = 0; col < 8; col++)
        {
            p = tabu.tab[SQ(col, row)];
            ap = TIPO(p);
            if(ap == 0) c = '.';
            else if(ap == PEAO) c = 'P';
            else if(ap == CAVALO) c = 'N';
            else if(ap == BISPO) c = 'B';
            else if(ap == TORRE) c = 'R';
            else if(ap == DAMA) c = 'Q';
            else if(ap == REI) c = 'K';
            else c = '?';
            if(EHPRETA(p)) c += 32; /* pretas minuscula */
            fprintf(stderr, "%c ", c);
        }
        fprintf(stderr, "%d\n", row + 1);
    }
    fprintf(stderr, "  a b c d e f g h\n");
    fprintf(stderr, "vez=%s\n", tabu.vez == BRANCO ? "brancas" : "pretas");
}

//int para char: de/pa (0-63) para string "e2e4"
void lance2movi(char *m, int de, int pa, int espec)
{
    const int pro=4; // indice promocao
    m[0] = COL(de) + 'a';
    m[1] = ROW(de) + '1';
    /* if(espec==9) m[2]='x'; /1* xizinho em captura *1/ else m[2]='-'; // tracinho */
    m[2] = COL(pa) + 'a';
    m[3] = ROW(pa) + '1';
    m[pro] = '\0'; // promocao
    m[5] = '\0';
    switch(espec) //promocao para 4,5,6,7 == D,C,T,B
    {
        case 4: //promoveu a Dama
            m[pro] = 'q'; break;
        case 5: //promoveu a Cavalo
            m[pro] = 'n'; break;
        case 6: //promoveu a Torre
            m[pro] = 'r'; break;
        case 7: //promoveu a Bispo
            m[pro] = 'b'; break;
    }
}

//transforma char de entrada em int de/pa (0-63)
int movi2lance(int *de, int *pa, char *m)
{
    if(m[0] < 'a' || m[0] > 'h' || m[1] < '1' || m[1] > '8')
        return 0;
    if(m[2] < 'a' || m[2] > 'h' || m[3] < '1' || m[3] > '8')
        return 0;
    *de = SQ((int)(m[0] - 'a'), (int)(m[1] - '1'));
    *pa = SQ((int)(m[2] - 'a'), (int)(m[3] - '1'));
    return 1;
}

void copitab(tabuleiro *dest, tabuleiro *font)
{
    int i, j;
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
            dest->tab[SQ(i, j)] = font->tab[SQ(i, j)];
    dest->vez = font->vez;
    dest->de = font->de;
    dest->pa = font->pa;
    dest->meionum = font->meionum;
    dest->rei_pos[0] = font->rei_pos[0];
    dest->rei_pos[1] = font->rei_pos[1];
    dest->peao_pulou = font->peao_pulou;
    dest->roqueb = font->roqueb;
    dest->roquep = font->roquep;
    dest->empate_50 = font->empate_50;
    dest->especial = font->especial;
}

// gera lista de movimentos legais na lista lmov
// geramodo: GERA_TUDO=todos, GERA_UNICO=para no primeiro, GERA_ESTE=confere legalidade de um movimento
// retorna 1 se ha movimentos, 0 se nao ha
int geramov(tabuleiro tabu, lista *lmov, int geramodo)
{
    /* IFDEBUG("geramov()"); */
    tabuleiro tabaux;
    int i, j, k, l, m, flag;
    int col, lin;
    int peca;
    int pp, rr, ee, ff; //roque, especial e flag50;
    int i0, i1, j0, j1; //limites do loop

    assert(tabu.vez == BRANCO || (tabu.vez == PRETO && "Invalid turn in geramov"));
    if(geramodo >= 0) // GERA_DESTE: gera apenas da casa geramodo (0-63)
    {
        i0 = i1 = COL(geramodo);
        j0 = j1 = ROW(geramodo);
    }
    else
    {
        i0 = 0; i1 = 7;
        j0 = 0; j1 = 7;
    }
    for(i = i0; i <= i1; i++)  //#coluna
        for(j = j0; j <= j1; j++)  //#linha
        {
            if(EHVAZIA(tabu.tab[SQ(i, j)]) || COR(tabu.tab[SQ(i, j)]) != tabu.vez)
                continue; //casa vazia, ou cor errada
            peca = TIPO(tabu.tab[SQ(i, j)]);
            switch(peca)
            {
                case REI:
                    for(col = i - 1; col <= i + 1; col++)  //Rei anda normalmente
                        for(lin = j - 1; lin <= j + 1; lin++)
                        {
                            if(lin < 0 || lin > 7 || col < 0 || col > 7)
                                continue; //casa de indice invalido
                            if(!EHVAZIA(tabu.tab[SQ(col, lin)]) && COR(tabu.tab[SQ(col, lin)]) == tabu.vez)
                                continue; //casa possui peca da mesma cor ou o proprio rei
                            if(ataca(ADV(tabu.vez), col, lin, tabu))
                                continue; //adversario esta protegendo a casa
                            copitab(&tabaux, &tabu);
                            tabaux.tab[SQ(i, j)] = VAZIA;
                            tabaux.tab[SQ(col, lin)] = DACOR(REI, tabu.vez);
                            tabaux.rei_pos[ICOR(tabu.vez)] = SQ(col, lin);
                            if(!xeque_rei_das(tabu.vez, tabaux))
                            {
                                //pp contem -1 ou coluna do peao que andou duas neste lance
                                pp = -1; // nao foi peao capturando enpassant
                                //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                rr = 0; //nao pode mais fazer roque. Mexeu Rei
                                //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=deu xeque
                                ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                if(tabu.tab[SQ(col, lin)] == VAZIA)
                                    ff = 0; //nao capturou
                                else
                                {
                                    ff = 2; //Rei capturou peca adversaria
                                    if(ee == 0) ee = 9; // captura
                                }
                                enche_lmovi(lmov, /*de*/SQ(i,j), /*pa*/SQ(col,lin), /*peao*/pp, /*roque*/rr, /*especial*/ee, /*flag50*/ff);
                                if(geramodo == GERA_UNICO)
                                    return 1;
                            }
                        }
                    //----------------------------- Roque de Brancas e Pretas
                    if(tabu.vez == BRANCO)
                        if(tabu.roqueb == 0)  //Se nao pode mais rocar: break!
                            break;
                        else
                        {
                            //roque de brancas
                            if(tabu.roqueb != 2 && tabu.tab[SQ(7, 0)] == DACOR(TORRE, BRANCO))
                            {
                                //Nao mexeu TR (e ela esta la Adv poderia ter comido antes de mexer)
                                // roque pequeno
                                if(tabu.tab[SQ(5, 0)] == VAZIA && tabu.tab[SQ(6, 0)] == VAZIA) //f1,g1
                                {
                                    flag = 0;
                                    for(k = 4; k < 7; k++)  //colunas e,f,g
                                        if(ataca(PRETO, k, 0, tabu))
                                        {
                                            flag = 1; //casas do roque atacadas
                                            break;
                                        }
                                    if(flag == 0)
                                    {
                                        //nao pode mais fazer roque. Mexeu Rei
                                        //Rei rocou pqn, espec=1. flag=0
                                        //pp contem -1 ou coluna do peao que andou duas
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque 9=captura
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        enche_lmovi(lmov, SQ(4,0), SQ(6,0), /*pp*/ -1, /*rr*/0, /*ee*/1, /*ff*/0); //BUG flag50 zera no roque? consultar FIDE
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;

                                    }
                                } // roque grande
                            } //mexeu TR
                            if(tabu.roqueb != 3 && tabu.tab[SQ(0, 0)] == DACOR(TORRE, BRANCO)) //Nao mexeu TD (e a torre existe!)
                            {
                                if(tabu.tab[SQ(1, 0)] == VAZIA && tabu.tab[SQ(2, 0)] == VAZIA && tabu.tab[SQ(3, 0)] == VAZIA) //b1,c1,d1
                                {
                                    flag = 0;
                                    for(k = 2; k < 5; k++)  //colunas c,d,e
                                        if(ataca(PRETO, k, 0, tabu))
                                        {
                                            flag = 1; //casas do roque atacadas
                                            break;
                                        }
                                    if(flag == 0)
                                    {
                                        //nao pode mais fazer roque. Mexeu Rei
                                        //Rei rocou grd, espec=2. flag=0
                                        enche_lmovi(lmov, SQ(4,0), SQ(2,0), /*peao*/ -1, /*roque*/0, /*especial*/2, /*flag50*/0);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                }
                            } //mexeu TD
                        }
                    else // vez das pretas jogarem
                        if(tabu.roquep == 0)  //Se nao pode rocar: break!
                            break;
                        else //roque de pretas
                        {
                            if(tabu.roquep != 2 && tabu.tab[SQ(7, 7)] == DACOR(TORRE, PRETO)) //Nao mexeu TR (e a torre nao foi capturada)
                            {
                                // roque pequeno
                                if(tabu.tab[SQ(5, 7)] == VAZIA && tabu.tab[SQ(6, 7)] == VAZIA) //f8,g8
                                {
                                    flag = 0;
                                    for(k = 4; k < 7; k++)  //colunas e,f,g
                                        if(ataca(BRANCO, k, 7, tabu))
                                        {
                                            flag = 1;
                                            break;
                                        }
                                    if(flag == 0)
                                    {
                                        //nao pode mais fazer roque. Mexeu Rei
                                        //Rei rocou pqn, espec=1. flag=0.
                                        enche_lmovi(lmov, SQ(4,7), SQ(6,7), /*peao*/ -1, /*roque*/0, /*especial*/1, /*flag50*/0);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                } // roque grande.
                            } //mexeu TR
                            if(tabu.roquep != 3 && tabu.tab[SQ(0, 7)] == DACOR(TORRE, PRETO)) //Nao mexeu TD (e a torre esta la)
                            {
                                if(tabu.tab[SQ(1, 7)] == VAZIA && tabu.tab[SQ(2, 7)] == VAZIA && tabu.tab[SQ(3, 7)] == VAZIA) //b8,c8,d8
                                {
                                    flag = 0;
                                    for(k = 2; k < 5; k++)  //colunas c,d,e
                                        if(ataca(BRANCO, k, 7, tabu))
                                        {
                                            flag = 1;
                                            break;
                                        }
                                    if(flag == 0)
                                    {
                                        //nao pode mais fazer roque. Mexeu Rei
                                        //Rei rocou grd, espec=2. flag=0.
                                        enche_lmovi(lmov, SQ(4,7), SQ(2,7), /*peao*/ -1, /*roque*/0, /*especial*/2, /*flag50*/0);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                }
                            } //mexeu TD
                        }
                    break;
                case PEAO:
                    //peao comeu en passant
                    if(tabu.peao_pulou != -1)  //Existe peao do adv que pulou...
                    {
                        if(i == tabu.peao_pulou - 1 || i == tabu.peao_pulou + 1)  //este peao esta na coluna certa
                        {
                            copitab(&tabaux, &tabu);  // tabaux = tabu
                            if(tabu.vez == BRANCO)
                            {
                                if(j == 4)  //E tambem esta na linha certa. Peao branco vai comer enpassant!
                                {
                                    tabaux.tab[SQ(tabu.peao_pulou, 5)] = DACOR(PEAO, BRANCO);
                                    tabaux.tab[SQ(tabu.peao_pulou, 4)] = VAZIA;
                                    tabaux.tab[SQ(i, 4)] = VAZIA;
                                    if(!xeque_rei_das(BRANCO, tabaux))   //nao deixa rei branco em xeque
                                    {
                                        enche_lmovi(lmov, SQ(i,j), SQ(tabu.peao_pulou,5), /*peao*/ -1, /*roque*/1, /*especial*/3, /*flag50*/3);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                }
                            }
                            else // vez das pretas.
                            {
                                if(j == 3)  // Esta na linha certa. Peao preto vai comer enpassant!
                                {
                                    tabaux.tab[SQ(tabu.peao_pulou, 2)] = DACOR(PEAO, PRETO);
                                    tabaux.tab[SQ(tabu.peao_pulou, 3)] = VAZIA;
                                    tabaux.tab[SQ(i, 3)] = VAZIA;
                                    if(!xeque_rei_das(PRETO, tabaux))
                                    {
                                        enche_lmovi(lmov, SQ(i,j), SQ(tabu.peao_pulou,2), /*peao*/ -1, /*roque*/1, /*especial*/3, /*flag50*/3);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    } //deixa rei em xeque
                                } //nao esta na fila correta
                            } //fim do else da vez
                        } //nao esta na coluna adjacente ao peao_pulou
                    } //nao peao_pulou.
                    //peao andando uma casa
                    copitab(&tabaux, &tabu);  //tabaux = tabu;
                    if(tabu.vez == BRANCO)
                    {
                        if(j + 1 < 8)
                        {
                            if(tabu.tab[SQ(i, j + 1)] == VAZIA)
                            {
                                tabaux.tab[SQ(i, j)] = VAZIA;
                                tabaux.tab[SQ(i, j + 1)] = DACOR(PEAO, BRANCO);
                                if(!xeque_rei_das(BRANCO, tabaux))
                                {
                                    if(j + 1 == 7)  //se promoveu
                                    {
                                        //TODO inverter laco e=4, e<8, ee++
                                        for(ee = 4; ee < 8; ee++) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_lmovi(lmov, SQ(i,j), SQ(i,j+1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    else //nao promoveu
                                    {
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        enche_lmovi(lmov, SQ(i,j), SQ(i,j+1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    if(geramodo == GERA_UNICO) //se nao afogou retorna
                                        return 1;
                                } //deixa rei em xeque
                            } //casa ocupada
                        } //passou da casa de promocao e caiu do tabuleiro!
                    }
                    else //vez das pretas andar com peao uma casa
                    {
                        if(j - 1 > -1)
                        {
                            if(tabu.tab[SQ(i, j - 1)] == VAZIA)
                            {
                                tabaux.tab[SQ(i, j)] = VAZIA;
                                tabaux.tab[SQ(i, j - 1)] = DACOR(PEAO, PRETO);
                                if(!xeque_rei_das(PRETO, tabaux))
                                {
                                    if(j - 1 == 0)  //se promoveu
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_lmovi(lmov, SQ(i,j), SQ(i,j-1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    else //nao promoveu
                                    {
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        enche_lmovi(lmov, SQ(i,j), SQ(i,j-1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    if(geramodo == GERA_UNICO) //se nao afogou retorna
                                        return 1;
                                } //deixa rei em xeque
                            } //casa ocupada
                        } //passou da casa de promocao e caiu do tabuleiro!
                    }
                    //peao anda duas casas -----------
                    copitab(&tabaux, &tabu);
                    if(tabu.vez == BRANCO)  //vez das brancas
                    {
                        if(j == 1)  //esta na linha inicial
                        {
                            if(tabu.tab[SQ(i, 2)] == VAZIA && tabu.tab[SQ(i, 3)] == VAZIA)
                            {
                                tabaux.tab[SQ(i, 1)] = VAZIA;
                                tabaux.tab[SQ(i, 3)] = DACOR(PEAO, BRANCO);
                                if(!xeque_rei_das(BRANCO, tabaux))
                                {
                                    ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                    enche_lmovi(lmov, SQ(i,1), SQ(i,3), /*peao*/i, /*roque*/1, /*especial*/ee, /*flag50*/1);
                                    if(geramodo == GERA_UNICO) //se nao afogou retorna
                                        return 1;
                                } //deixa rei em xeque
                            } //alguma das casas esta ocupada
                        } //este peao ja mexeu antes
                    }
                    else //vez das pretas andar com peao 2 casas
                    {
                        if(j == 6)  //esta na linha inicial
                        {
                            if(tabu.tab[SQ(i, 5)] == VAZIA && tabu.tab[SQ(i, 4)] == VAZIA)
                            {
                                tabaux.tab[SQ(i, 6)] = VAZIA;
                                tabaux.tab[SQ(i, 4)] = DACOR(PEAO, PRETO);
                                if(!xeque_rei_das(PRETO, tabaux))
                                {
                                    ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                    enche_lmovi(lmov, SQ(i,6), SQ(i,4), /*peao*/i, /*roque*/1, /*especial*/ee, /*flag50*/1);
                                    if(geramodo == GERA_UNICO) //se nao afogou retorna
                                        return 1;
                                } //deixa rei em xeque
                            } //alguma das casas esta ocupada
                        } //este peao ja mexeu antes
                    }
                    // peao comeu normalmente (i.e., nao eh en passant) -------------
                    copitab(&tabaux, &tabu);
                    if(tabu.vez == BRANCO)  //vez das brancas
                    {
                        k = i - 1;
                        if(k < 0)  //peao da torre so come para direita
                            k = i + 1;
                        do //diagonal esquerda e direita do peao.
                        {
                            //peca preta
                            if(EHPRETA(tabu.tab[SQ(k, j + 1)]))
                            {
                                tabaux.tab[SQ(i, j)] = VAZIA;
                                tabaux.tab[SQ(k, j + 1)] = DACOR(PEAO, BRANCO);
                                if(!xeque_rei_das(BRANCO, tabaux))
                                {
                                    if(j + 1 == 7)  //se promoveu comendo
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_lmovi(lmov, SQ(i,j), SQ(k,j+1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3);
                                    }
                                    else //comeu sem promover
                                    {
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0) ee = 9; // captura (ff=3)
                                        enche_lmovi(lmov, SQ(i,j), SQ(k,j+1), /*peao*/ -1, /*roque*/1, /*especial*/ee, /*flag50*/3);
                                    }
                                    if(geramodo == GERA_UNICO) //se nao afogou retorna
                                        return 1;
                                } //deixa rei em xeque
                            } //nao tem peca preta para comer
                            k += 2; //proxima diagonal do peao
                        }
                        while(k <= i + 1 && k < 8);
                    } // vez das pretas comer normalmente (i.e., nao eh en passant)
                    else
                    {
                        k = i - 1;
                        if(k < 0)
                            k = i + 1;
                        do //diagonal esquerda e direita do peao.
                        {
                            if(EHBRANCA(tabu.tab[SQ(k, j - 1)])) //peca branca
                            {
                                tabaux.tab[SQ(i, j)] = VAZIA;
                                tabaux.tab[SQ(k, j - 1)] = DACOR(PEAO, PRETO);
                                if(!xeque_rei_das(PRETO, tabaux))
                                {
                                    //4:dama, 5:cavalo, 6:torre, 7:bispo
                                    //peao preto promove comendo
                                    //                                 (*nmovi)++;
                                    if(j - 1 == 0)  //se promoveu comendo
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_lmovi(lmov, SQ(i,j), SQ(k,j-1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3);
                                    }
                                    else //comeu sem promover
                                    {
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0) ee = 9; // captura (ff=3)
                                        enche_lmovi(lmov, SQ(i,j), SQ(k,j-1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3);
                                    }
                                    if(geramodo == GERA_UNICO) //se nao afogou retorna
                                        return 1;
                                } //rei em xeque
                            } //nao tem peao branco para comer
                            k += 2;
                        }
                        while(k <= i + 1 && k < 8);
                    }
                    break;
                case CAVALO:
                    for(col = -2; col < 3; col++)
                        for(lin = -2; lin < 3; lin++)
                        {
                            if(abs(col) == abs(lin) || col == 0 || lin == 0)
                                continue;
                            if(i + col < 0 || i + col > 7 || j + lin < 0 || j + lin > 7)
                                continue;
                            if(!EHVAZIA(tabu.tab[SQ(i + col, j + lin)]) && COR(tabu.tab[SQ(i + col, j + lin)]) == tabu.vez)
                                continue; //casa possui peca da mesma cor.
                            copitab(&tabaux, &tabu);
                            tabaux.tab[SQ(i, j)] = VAZIA;
                            tabaux.tab[SQ(i + col, j + lin)] = DACOR(CAVALO, tabu.vez);
                            if(!xeque_rei_das(tabu.vez, tabaux))
                            {
                                if(tabu.tab[SQ(col + i, lin + j)] == VAZIA)
                                    ff = 0; //Cavalo nao capturou
                                else
                                    ff = 2; // Cavalo capturou peca adversaria.
                                ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                if(ee == 0 && ff > 1) ee = 9; // captura
                                enche_lmovi(lmov, SQ(i,j), SQ(col+i,lin+j), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/ff);
                                if(geramodo == GERA_UNICO) //se nao afogou retorna
                                    return 1;
                            }
                        }
                    break;
                case DAMA:
                case TORRE:
                case BISPO:
                    //Anda reto - So nao o bispo
                    if(peca != BISPO)
                    {
                        for(k = -1; k < 2; k += 2)  //k= -1, 1
                        {
                            col = i + k;
                            lin = j + k;
                            l = 0; //l=flag da horizontal para a primeira peca que trombar
                            m = 0; //m=idem para a vertical
                            do
                            {
                                if(col >= 0 && col <= 7 && (EHVAZIA(tabu.tab[SQ(col, j)]) || COR(tabu.tab[SQ(col, j)]) != tabu.vez) && l == 0)  //gira col, mantem lin
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[SQ(i, j)] = VAZIA;
                                    tabaux.tab[SQ(col, j)] = DACOR(peca, tabu.vez);
                                    if(!xeque_rei_das(tabu.vez, tabaux))
                                    {
                                        rr = 1; //ainda pode fazer roque
                                        if(peca == TORRE)  //mas, se for torre ...
                                        {
                                            if(tabu.vez == BRANCO && j == 0)  //se for vez das brancas e linha 1
                                            {
                                                if(i == 0)  //e coluna a
                                                    rr = 3; //mexeu TD
                                                if(i == 7)  //ou coluna h
                                                    rr = 2; //mexeu TR
                                            }
                                            if(tabu.vez == PRETO && j == 7)  //se for vez das pretas e linha 8
                                            {
                                                if(i == 0)  //e coluna a
                                                    rr = 3; //mexeu TD
                                                if(i == 7)  //ou coluna h
                                                    rr = 2; //mexeu TR
                                            }
                                        }
                                        ff = 0; // nao muda flag50 (nao capturou) pmovi->flag_50=0;
                                        if(tabu.tab[SQ(col, j)] != VAZIA)
                                        {
                                            ff = 2; //Dama ou Torre capturou peca adversaria.
                                            l = 1;
                                        }
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0 && ff > 1) ee = 9; // captura
                                        enche_lmovi(lmov, SQ(i,j), SQ(col,j), /*peao*/ -1, /*roque*/rr, /*especial*/ ee, /*flag50*/ff);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                    else //deixa rei em xeque
                                        if(tabu.tab[SQ(col, j)] != VAZIA)
                                            //alem de nao acabar o xeque, nao pode mais seguir nessa direcao pois tem peca
                                            l = 1;
                                }
                                else
                                    //nao inclui mais nenhuma casa desta linha; A casa esta fora do tabuleiro, ou tem peca de mesma cor ou capturou
                                    l = 1;
                                if(lin >= 0 && lin <= 7 && (EHVAZIA(tabu.tab[SQ(i, lin)]) || COR(tabu.tab[SQ(i, lin)]) != tabu.vez) && m == 0)  //gira lin, mantem col
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[SQ(i, j)] = VAZIA;
                                    tabaux.tab[SQ(i, lin)] = DACOR(peca, tabu.vez);
                                    if(!xeque_rei_das(tabu.vez, tabaux))
                                    {
                                        rr = 1; //ainda pode fazer roque
                                        if(peca == TORRE)  //mas, se for torre ...
                                        {
                                            if(tabu.vez == BRANCO && j == 0)  //se for vez das brancas e linha 1
                                            {
                                                if(i == 0)  //e coluna a
                                                    rr = 3; //mexeu TD
                                                if(i == 7)  //ou coluna h
                                                    rr = 2; //mexeu TR
                                            }
                                            if(tabu.vez == PRETO && j == 7)  //se for vez das pretas e linha 8
                                            {
                                                if(i == 0)  //e coluna a
                                                    rr = 3; //mexeu TD
                                                if(i == 7)  //ou coluna h
                                                    rr = 2; //mexeu TR
                                            }
                                        }
                                        ff = 0;
                                        if(tabu.tab[SQ(i, lin)] != VAZIA)
                                        {
                                            ff = 2; //Dama ou Torre capturou peca adversaria.
                                            m = 1;
                                        }
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0 && ff > 1) ee = 9; // captura
                                        enche_lmovi(lmov, SQ(i,j), SQ(i,lin), /*peao*/ -1, /*roque*/rr, /*especial*/ ee, /*flag50*/ff);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                    else //deixa rei em xeque
                                        if(tabu.tab[SQ(i, lin)] != VAZIA)
                                            //alem de nao acabar o xeque, nao pode mais seguir nessa direcao pois tem peca
                                            m = 1;
                                }
                                else //nao inclui mais nenhuma casa desta coluna; A casa esta fora do tabuleiro, ou tem peca de mesma cor ou capturou
                                    m = 1;
                                col += k;
                                lin += k;
                            }
                            while(((col >= 0 && col <= 7) || (lin >= 0 && lin <= 7)) && (m == 0 || l == 0));
                        } //roda o incremento
                    } //a peca era bispo
                    if(peca != TORRE)  //anda na diagonal
                    {
                        col = i;
                        lin = j;
                        flag = 0; // pode seguir na direcao
                        for(k = -1; k < 2; k += 2)
                            for(l = -1; l < 2; l += 2)
                            {
                                col += k;
                                lin += l;
                                while(col >= 0 && col <= 7 && lin >= 0 && lin <= 7 && (EHVAZIA(tabu.tab[SQ(col, lin)]) || COR(tabu.tab[SQ(col, lin)]) != tabu.vez) && flag == 0)
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[SQ(i, j)] = VAZIA;
                                    tabaux.tab[SQ(col, lin)] = DACOR(peca, tabu.vez);
                                    if(!xeque_rei_das(tabu.vez, tabaux))
                                    {
                                        //preenche a estrutura movimento
                                        //                                     if ((*nmovi) == -1)
                                        //                                         return (movimento *) TRUE;
                                        ff = 0; //flag50 nao altera
                                        if(tabu.tab[SQ(col, lin)] != VAZIA)
                                        {
                                            ff = 2; //Dama ou Bispo capturou peca adversaria.
                                            flag = 1;
                                        }
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0 && ff > 1) ee = 9; // captura
                                        enche_lmovi(lmov, SQ(i,j), SQ(col,lin), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/ff);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                    else //deixa rei em xeque
                                        if(tabu.tab[SQ(col, lin)] != VAZIA) //alem de nao acabar o xeque, nao pode mais seguir nessa direcao pois tem peca
                                            flag = 1;
                                    col += k;
                                    lin += l;
                                } //rodou toda a diagonal, ate tab acabar ou achar peca da mesma cor ou capturar
                                col = i;
                                lin = j;
                                flag = 0;
                            } //for dos incrementos l, k
                    } //peca era torre
                case VAZIA:
                    break;
                case NULA:
                default:
                    msgsai("# Erro: Achei uma peca esquisita! geramov, default.", 1);
            } // switch(peca)
        } //loop das pecas todas do tabuleiro

    if(geramodo == GERA_UNICO) //GERA_UNICO: nao achou nenhum lance
        return 0;
    if(lmov && lmov->qtd > 1)
        lst_parte(lmov); //particiona: capturas e especiais primeiro
    return (lmov ? lmov->qtd > 0 : 0);
}
//fim de   ----------- int geramov(tabuleiro tabu, lista *lmov, int geramodo)

int ataca(int cor, int col, int lin, tabuleiro tabu)
{
    //retorna verdadeiro (1) ou falso (0)
    //cor==brancas   => brancas atacam casa(col,lin)
    //cor==pretas    => pretas  atacam casa(col,lin)
    int icol, ilin, casacol, casalin;
    //torre ou dama atacam a casa...
    for(icol = col - 1; icol >= 0; icol--)
        //desce coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(icol, lin)] == DACOR(TORRE, cor) || tabu.tab[SQ(icol, lin)] == DACOR(DAMA, cor))
            return (1);
        break;
    }
    for(icol = col + 1; icol < 8; icol++)
        //sobe coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(icol, lin)] == DACOR(TORRE, cor) || tabu.tab[SQ(icol, lin)] == DACOR(DAMA, cor))
            return (1);
        break;
    }
    for(ilin = lin + 1; ilin < 8; ilin++)
        // direita na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(col, ilin)] == DACOR(TORRE, cor) || tabu.tab[SQ(col, ilin)] == DACOR(DAMA, cor))
            return (1);
        break;
    }
    for(ilin = lin - 1; ilin >= 0; ilin--)
        // esquerda na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(col, ilin)] == DACOR(TORRE, cor) || tabu.tab[SQ(col, ilin)] == DACOR(DAMA, cor))
            return (1);
        break;
    }
    // cavalo ataca casa...
    for(icol = -2; icol < 3; icol++)
        for(ilin = -2; ilin < 3; ilin++)
        {
            if(abs(icol) == abs(ilin) || icol == 0 || ilin == 0)
                continue;
            if(col + icol < 0 || col + icol > 7 || lin + ilin < 0
                    || lin + ilin > 7)
                continue;
            if(tabu.tab[SQ(col + icol, lin + ilin)] == DACOR(CAVALO, cor))
                return (1);
        }
    // bispo ou dama atacam casa...
    for(icol = -1; icol < 2; icol += 2)
        for(ilin = -1; ilin < 2; ilin += 2)
        {
            casacol = col;
            //para cada diagonal, comece na casa origem.
            casalin = lin;
            do
            {
                casacol = casacol + icol;
                casalin = casalin + ilin;
                if(casacol < 0 || casacol > 7 || casalin < 0 || casalin > 7)
                    break;
            }
            while(tabu.tab[SQ(casacol, casalin)] == VAZIA);

            if(casacol >= 0 && casacol <= 7 && casalin >= 0 && casalin <= 7)
                if(tabu.tab[SQ(casacol, casalin)] == DACOR(BISPO, cor)
                        || tabu.tab[SQ(casacol, casalin)] == DACOR(DAMA, cor))
                    return 1;
        }
    // proxima diagonal
    // ataque de rei...
    for(icol = col - 1; icol <= col + 1; icol++)
        for(ilin = lin - 1; ilin <= lin + 1; ilin++)
        {
            if(icol == col && ilin == lin)
                continue;
            if(icol < 0 || icol > 7 || ilin < 0 || ilin > 7)
                continue;
            if(tabu.tab[SQ(icol, ilin)] == DACOR(REI, cor))
                return (1);
        }
    if(cor == BRANCO)
        // ataque de peao branco: peao branco ataca para baixo (lin-1)
    {
        if(lin > 1)
        {
            ilin = lin - 1;
            if(col - 1 >= 0)
                if(tabu.tab[SQ(col - 1, ilin)] == DACOR(PEAO, BRANCO))
                    return (1);
            if(col + 1 <= 7)
                if(tabu.tab[SQ(col + 1, ilin)] == DACOR(PEAO, BRANCO))
                    return (1);
        }
    }
    else
        //ataque de peao preto: peao preto ataca para cima (lin+1)
    {
        if(lin < 6)
        {
            ilin = lin + 1;
            if(col - 1 >= 0)
                if(tabu.tab[SQ(col - 1, ilin)] == DACOR(PEAO, PRETO))
                    return (1);
            if(col + 1 <= 7)
                if(tabu.tab[SQ(col + 1, ilin)] == DACOR(PEAO, PRETO))
                    return (1);
        }
    }
    return (0);
}

//fim de int ataca(int cor, int col, int lin, tabuleiro tabu)
int xeque_rei_das(int cor, tabuleiro tabu)
{
    int sq = tabu.rei_pos[ICOR(cor)];

    return ataca(ADV(cor), COL(sq), ROW(sq), tabu);
}

// fim de int xeque_rei_das(int cor, tabuleiro tabu)

//----------------------------------------------
char humajoga(tabuleiro *tabu)
{
    char movinito[80];
    //movimento ou comando entrado pelo usuario ou XBoard/WinBoard
    movimento mval;
    char res;
    //    char aux;
    //     char peca;
    int tente;
    int de, pa, moves, minutes;
    double secs, osecs = 0.0, incre = 0.0;
    int i, j, k;
    //     int casacor;
    //nao precisa se tem textbackground
    //     listab *plaux;
    char pieces[80], color[80], castle[80], enpassant[80], halfmove[80], fullmove[80]; //para testar uma posicao
    int testasim = 0, estat;
    movinito[79] = movinito[0] = '\0';
    char *tc = NULL; /* time control has ":" */

    do
    {
        tente = 0;
        scanf2(movinito);	//---------------- (*quase) Toda entrada do xboard

        if(!strcmp(movinito, "hint"))   // Dica
        {
            mostra_lances(*tabu);
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "force"))   // nao joga, apenas acompanha
        {
            primeiro = 'h';
            segundo = 'h';
            tente = 1;
            printdbg(debug, "# xboard: force. Xadreco is in force mode.\n");
            continue;
        }
        if(!strcmp(movinito, "undo"))   // volta um movimento (ply)
        {
            primeiro = 'h';
            segundo = 'h';
            volta_lance(tabu);
            tente = 1;
            printdbg(debug, "# xboard: undo. Back one ply. Xadreco is in force mode.\n");
            continue;
        }
        if(!strcmp(movinito, "remove"))   // volta dois movimentos == 1 lance
        {
            volta_lance(tabu);
            volta_lance(tabu);
            tente = 1;
            printdbg(debug, "# xboard: remove. Back one move.\n");
            continue;
        }
        if(!strcmp(movinito, "post"))   //showthinking ou mostrapensando
        {
            analise = 0;
            mostrapensando = 1;
            tente = 1;
            printdbg(debug, "# xboard: post. Xadreco will show what its thinking. (2)\n");
            continue;
        }
        if(!strcmp(movinito, "nopost"))   //no show thinking ou desliga o mostrapensando
        {
            mostrapensando = 0;
            tente = 1;
            printdbg(debug, "# xboard: nopost. Xadreco will not show what its thinking.\n");
            continue;
        }
//        if (!strcmp (movinito, "randomchess")) //randomchess: pensa ou joga ao acaso
//        {
//            randomchess = 1;
//            tente = 1;
//            continue;
//        }
//        if (!strcmp (movinito, "norandomchess")) //desliga randomchess
//        {
//            randomchess = 0;
//            tente = 1;
//            continue;
//        }
//        if (!strcmp (movinito, "random")) //toogle: liga/desliga randomchess
//        {
//            randomchess = (randomchess?0:1);
//            tente = 1;
//            continue;
//        }

        if(!strcmp(movinito, "analyze"))   // analise inicia
        {
//            if (analise == -1)
//            {
//                analise = 0;
//                tente = 1;
//                continue;
//            }
            printdbg(debug, "# xboard: analyze. Xadreco starts analyzing in force mode.\n");
            analise = 1;
            mostrapensando = 0;
            primeiro = 'h';
            segundo = 'h';
            disc = analisa(tabu);  //disc=variavel de descarte
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "exit"))   // analise termina
        {
            analise = 0;
            tente = 1;
            printdbg(debug, "# xboard: exit. Xadreco stops analyzing.\n");
            continue;
        }
        if(!strcmp(movinito, "draw"))   //aceitar empate? (o outro esta oferecendo)
        {
            if(primeiro == segundo && primeiro == 'h')  //humano contra humano, aceita sempre
                return ('c');
            else //humano x computador ou computador contra computador
            {
                estat = -estatico(*tabu, 0, 0, -LIMITE, +LIMITE);
                //ao receber proposta, o valor e calculado em funcao de quem e a vez.
                // 0:nao e por falta de tempo. 0: nivel do tabuleiro real
                //se comp. versus humano: estamos na funcao "humano-joga", logo a vez e do humano!
                //quando vc esta perdendo e pede empate, eh sua vez, e portanto o valor
                //que estat retorna negativo (vc esta perdendo)
                //O computador precisa avaliar esse resultado com sinal trocado, pois ele esta ganhando
                //Se o computador (positivo) < Limite QUANTO_EMPATE1, entao ele aceita.
                //O detalhe e que o computador so aceita se ele estiver perdendo dois peoes.
                //Ou seja, voce precisa estar positivo em 20 pontos, para o computador aceitar

                //if primeiro=='c'

                if(estat < QUANTO_EMPATE1)	// && !COMPUTER)
                    //bugbug deve aceitar de outro computer se esta perdendo!
                    //so se perdendo 2 peoes (e nao esta jogando contra outra engine)
                {
                    //bug: eu era branca, e ele tinha dama aceitou empate.
                    printdbg(debug, "# xadreco : draw accepted (HumaJ-1) %d  turn: %d\n", estat, tabu->vez);
                    return ('c');
                }
                else
                {
                    printdbg(debug, "# xadreco : draw rejected (HumaJ-2) %d  turn: %d\n", estat, tabu->vez);
                    tente = 1;
                    continue;
                }
            }
        }
        if(!strcmp(movinito, "version"))
        {
            printdbg(debug, "# Xadreco v%s build %s, XBoard protocol, by Ruben Carlo Benante, 1994-2026.\n", VERSION, BUILD);
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "sd"))   //set deep: coloca o nivel de profundidade. Nao esta sendo usado. Implementar.
        {
            nivel = (int)(movinito[3] - '0');
            nivel = (nivel > 6 || nivel < 0) ? 2 : nivel;
            tente = 1;
            printdbg(debug, "# xboard: sd. Xadreco is set deep %d\n", nivel);
            continue;
        }
        if(!strcmp(movinito, "st"))   //set time: seconds per move
        {
            scanf2(movinito);
            tempomovclock = atof(movinito);
            if(tempomovclock < 0.5)
                tempomovclock = 0.5;
            tempomovclockmax = tempomovclock;
            printdbg(debug, "# xboard: st. Xadreco is set time %f s per move\n", tempomovclock);
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "go"))   //troca de lado e joga. (o mesmo que "adv")
        {
            if(tabu->vez == BRANCO)
            {
                primeiro = 'c';
                segundo = 'h';
                printdbg(debug, "# xboard: go. Xadreco is now white.\n");
            }
            else
            {
                primeiro = 'h';
                segundo = 'c';
                printdbg(debug, "# xboard: go. Xadreco is now black.\n");
            }
            return ('y'); //retorna para jogar ou humano ou computador.
        }
        if(!strcmp(movinito, "playother"))   //coloca o computador com a cor que nao ta da vez. (switch)
        {
            if(tabu->vez == BRANCO)
            {
                primeiro = 'h';
                segundo = 'c';
                printdbg(debug, "# xboard: playother. Xadreco is now black.\n");
            }
            else
            {
                primeiro = 'c';
                segundo = 'h';
                printdbg(debug, "# xboard: playother. Xadreco is now white.\n");
            }
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "computer"))   //altera estrategia para jogar contra outra engine;
        {
            ofereci = 2; //pode oferecer mais empates
            ABANDONA = -LIMITE; //nao abandona nunca
            COMPUTER = 1;
            tente = 1;
            printdbg(debug, "# xboard: computer. Xadreco now knows its playing against another engine.\n");
            continue;
        }
        if(!strcmp(movinito, "new"))   //novo jogo
        {
            printdbg(debug, "# xboard: new. Xadreco sets the board in initial position.\n");
            return ('w');
        }
        if(!strcmp(movinito, "resign"))
        {
            if(tabu->vez == PRETO)
                return 'b';        //pretas abandonam
            else
                return 'B';        //brancas abandonam
        }
        if(!strcmp(movinito, "quit"))
            msgsai("# Thanks for playing Xadreco.", 0);

        //bug: pegou o rating, e agora faz o que com isso?
        if(!strcmp(movinito, "rating"))
        {
            scanf2(movinito);
            myrating = atoi(movinito);
            scanf2(movinito);
            opprating = atoi(movinito);
            printdbg(debug, "# xboard: myrating: %d opprating: %d\n", myrating, opprating);
            tente = 1;
            continue;
        }
        //vai para o tela ou para o log
        // controle de tempo:
        //level 100 2 0
        //100 moves
        //2 min
        //0 inc
        //= 1200 milisec per mov
        //
        //
        //level 40 5 12
        //40 moves
        //5 min
        //12 inc
        //= 7500 milisec per mov + 12 inc
        //
        //level 0 5 0
        //0 all moves = 100 moves
        //5 min
        //0 inc
        //= 3000 milisec per mov
        //or.... level 0 5:10 0
        //0 moves, 5min and 10 seconds, 0=conventional (ignore)
        if(!strcmp(movinito, "level"))
        {
            scanf2(movinito);
            moves = atoi(movinito);

            scanf2(movinito);
            if((tc = strchr(movinito, ':')) != NULL) /* tem : */
            {
                *tc = '\0';
                minutes = atoi(movinito);
                tc++;
                secs = atof(tc);
                secs += minutes * 60.0;
            }
            else
            {
                minutes = atoi(movinito);
                secs = minutes * 60.0;
            }

            /* ultimo numero: incremento */
            scanf2(movinito);
            incre = atof(movinito);
            if(moves <= 0)
                moves = TOTAL_MOVIMENTOS;
            if(secs <= 0.0) /* sem tempo inicial, considere 10s */
                secs = 10.0;
            if(incre <= 0.0)
                incre = 0.0;
            //quantos minutos tenho para 40 lances?
            //tempomovclock = (40.0 * minutes) / (float)moves;
            //mais o incremento estimado para 40 lances.
            //tempomovclock += incre * 40.0;
            // tempo maximo por lance:
            //tempomovclock /= 40.0;
            // tempomovcloock/=2;
            // ?? usar metade do tempo disponivel.
//            tempomovtclock /= 2.0;
            //bug apesar de correto, esta gastando o dobro do tempo. Problema com as rotinas de apoio? Problema com o minimax que nao sabe retornar?
            if(minutes == 0 && secs < 10.0 && incre <= 0.0)
                secs = 30.0 * 60.0; /* no time, use 30 min */
            else //minutes !=0
                secs += incre * TOTAL_MOVIMENTOS; /* incremento baseado em 60 lances */
            tempomovclockmax = secs / 4.0; /* never spend more than 25% of remaining time */
            if(tempomovclockmax < 1.0)
                tempomovclockmax = 1.0;
            tempomovclock = secs / (float)moves; //em segundos
            if(tempomovclock > tempomovclockmax)
                tempomovclock = tempomovclockmax; //maximo tempomovclockmax
            if(tempomovclock < 0.5) //minimo meio segundo
                tempomovclock = 0.5;

            printdbg(debug, "# xadreco level: %.1fs+%.1fs por %d lances: ajustado para st %f s por lance (max %f)\n", secs, incre, moves, tempomovclock, tempomovclockmax);
            tente = 1;
            continue;
        }

        if(!strcmp(movinito, "otim"))   /* opponent time: can arrive before or after "time" */
        {
            scanf2(movinito);
            osecs = atof(movinito) / 100.0; /* seconds opponent has left */
            printdbg(debug, "# xboard: otim. Opponent has %.1fs\n", osecs);
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "time"))
        {
            scanf2(movinito);
            secs = atof(movinito) / 100.0; /* seconds I have left */

            moves = TOTAL_MOVIMENTOS - tabu->meionum / 2;
            if(moves <= 0)
                moves = 5;
            if(incre >= 0.0)
            {
                secs += incre * moves;
                osecs += incre * moves;
            }

            tempomovclockmax = secs / 4.0; /* never spend more than 25% of remaining time */
            if(tempomovclockmax < 1.0)
                tempomovclockmax = 1.0;
            tempomovclock = secs / (float)moves; //base time per move
            if(osecs > 0.0) //ratio: think longer when ahead on time, faster when behind
                tempomovclock *= (secs / osecs);
            if(tempomovclock > tempomovclockmax)
                tempomovclock = tempomovclockmax; //upper cap
            if(tempomovclock < 0.5) //lower cap
                tempomovclock = 0.5;

            printdbg(debug, "# xadreco time: meu: %.1fs opo:%.1fs ratio:%.2f para %d lances: st %f s (max %f)\n", secs, osecs, osecs > 0.0 ? secs / osecs : 0.0, moves, tempomovclock, tempomovclockmax);
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "setboard"))   //funciona no prompt tambem
        {
            printdbg(debug, "# xboard: setboard. Xadreco will set a board position.\n");
//            if (setboard == -1)
//            {
//                setboard = 0;
//                tente = 1;
//                continue;
//            }
            inicia(tabu);
            zera_pecas(tabu); // limpa pecas para FEN preencher
            //o jogo e editado
            //Posicao FEN
            scanf2(movinito);  //trava se colocar uma posicao FEN invalida!
            if(!strcmp(movinito, "testpos"))   //xadreco command
            {
                testasim = 1; //a posicao vem da funcao testapos, e nao de scanf()
                testapos(pieces, color, castle, enpassant, halfmove, fullmove);
            }
            if(testasim)
                strcpy(movinito, pieces);
            printdbg(debug, "# xboard: %s\n", movinito);
            j = 7; //linha de '8' a '1'
            i = 0; //coluna de 'a' a 'h'
            k = 0; //indice da string
            while(movinito[k] != '\0')
            {
                if(i < 0 || i > 8)  //coluna fora do tabuleiro (8 valido: aguarda '/' para resetar)
                    msgsai("# Posicao FEN incorreta.", 24);  //em vez de msg sai, printf2("Error (Wrong FEN %s): setboard", movinito);
                switch(movinito[k])  //KkQqRrBbNnPp
                {
                    case 'K':
                        tabu->tab[SQ(i, j)] = DACOR(REI, BRANCO);
                        tabu->rei_pos[0] = SQ(i, j);
                        i++;
                        break;
                    case 'k':
                        tabu->tab[SQ(i, j)] = DACOR(REI, PRETO);
                        tabu->rei_pos[1] = SQ(i, j);
                        i++;
                        break;
                    case 'Q':
                        tabu->tab[SQ(i, j)] = DACOR(DAMA, BRANCO);
                        i++;
                        break;
                    case 'q':
                        tabu->tab[SQ(i, j)] = DACOR(DAMA, PRETO);
                        i++;
                        break;
                    case 'R':
                        tabu->tab[SQ(i, j)] = DACOR(TORRE, BRANCO);
                        i++;
                        break;
                    case 'r':
                        tabu->tab[SQ(i, j)] = DACOR(TORRE, PRETO);
                        i++;
                        break;
                    case 'B':
                        tabu->tab[SQ(i, j)] = DACOR(BISPO, BRANCO);
                        i++;
                        break;
                    case 'b':
                        tabu->tab[SQ(i, j)] = DACOR(BISPO, PRETO);
                        i++;
                        break;
                    case 'N':
                        tabu->tab[SQ(i, j)] = DACOR(CAVALO, BRANCO);
                        i++;
                        break;
                    case 'n':
                        tabu->tab[SQ(i, j)] = DACOR(CAVALO, PRETO);
                        i++;
                        break;
                    case 'P':
                        tabu->tab[SQ(i, j)] = DACOR(PEAO, BRANCO);
                        i++;
                        break;
                    case 'p':
                        tabu->tab[SQ(i, j)] = DACOR(PEAO, PRETO);
                        i++;
                        break;
                    case '/':
                        i = 0;
                        j--;
                        break;
                    default: //numero de casas vazias
                        i += (movinito[k] - '0');
                }
                k++;
            }
            //Cor da vez
            if(testasim)
                strcpy(movinito, color);
            //Veja o que o sono faz no codigo da gente: (apertando ctrl-s para salvar... ainda bem que nao foi ctrl-z)
            //sssssssssss
            else
                scanf2(movinito);
            printdbg(debug, "# xboard: %s\n", movinito);
            if(movinito[0] == 'w')
                tabu->vez = BRANCO;
            else
                tabu->vez = PRETO;
            //int roqueb, roquep: 1:pode para os dois lados. 0:nao pode mais. 3:mexeu TD. So pode K. 2:mexeu TR. So pode Q
            if(testasim)
                strcpy(movinito, castle);
            else
                scanf2(movinito);  //Roque
            printdbg(debug, "# xboard: %s\n", movinito);
            tabu->roqueb = 0; //nao pode mais
            tabu->roquep = 0; //nao pode mais
            if(!strchr(movinito, '-'))   //alguem pode
            {
                if(strchr(movinito, 'K'))
                    tabu->roqueb = 3;                //so pode REI
                if(strchr(movinito, 'Q'))
                {
                    if(tabu->roqueb == 3)
                        tabu->roqueb = 1;                 //pode dos dois
                    else
                        tabu->roqueb = 2;                 //so pode DAMA
                }
                if(strchr(movinito, 'k'))
                    tabu->roquep = 3;
                if(strchr(movinito, 'q'))
                {
                    if(tabu->roquep == 3)
                        tabu->roquep = 1;                //pode dos dois
                    else
                        tabu->roquep = 2;                //so pode DAMA
                }
            }
            if(testasim)
                strcpy(movinito, enpassant);
            else
                scanf2(movinito);  //En passant
            printdbg(debug, "# xboard: %s\n", movinito);
            tabu->peao_pulou = -1; //nao pode
            if(!strchr(movinito, '-'))   //alguem pode
                tabu->peao_pulou = movinito[0] - 'a'; //pulou 2 na coluna dada
            if(testasim)
                strcpy(movinito, halfmove);
            else
                scanf2(movinito);  //Num. de movimentos (ply)
            printdbg(debug, "# xboard: %s\n", movinito);
            tabu->empate_50 = atoi(movinito);  //contador:quando chega a 50, empate.
            if(testasim)
                strcpy(movinito, fullmove);
            else
                scanf2(movinito);  //Num. de lances
            printdbg(debug, "# xboard: %s\n", movinito);
            /* tabu->meionum = 0; //mudou para ply. */
            /* tabu->meionum = (atoi(movinito)-1)+0.3; */

            /* tabu->meionum => 0, 1, 2, 3, 4, 5
             * FEN          => 1, 1, 2, 2, 3, 3 */
            tabu->meionum = (atoi(movinito) -1) * 2 + (tabu->vez == PRETO ? 1 : 0);
            //inicia no 0.3 para indicar 0
            /* ultimo resultado: - tudo certo, y troca vez */
            /* ultimo_resultado[0] = 'y'; //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.) */
            ultimo_resultado[0] = '\0'; //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.)
            res = situacao(*tabu);
            switch(res)
            {
                case 'a': //afogado
                case 'p': //perpetuo
                case 'i': //insuficiencia
                case '5': //50 lances
                case 'r': //repeticao
                    tabu->situa = 1;
                    break;
                case 'x': //xeque
                    tabu->situa = 2;
                    break;
                case 'M': //Brancas perderam por Mate
                    tabu->situa = 3;
                    break;
                case 'm': //Pretas perderam por mate
                    tabu->situa = 4;
                    break;
                case 'T': //Brancas perderam por Tempo
                    tabu->situa = 5;
                    break;
                case 't': //Pretas perderam  por tempo
                    tabu->situa = 6;
                    break;
                case '*': //sem resultado * {Game was unfinished}
                    tabu->situa = 7;
                    break;
                default: //'-' nada...
                    tabu->situa = 0;
            }
            USALIVRO = 0; // TODO: UCI usa setboard para mover Baseado somente nos movimentos, nao na posicao.
            primeiro = 'h';
            segundo = 'h';
            tab_insere(*tabu);
            imptab(*tabu);
            tente = 1;
            continue;
        }
        /*      if(!strcmp (movinito, "ping"))
                {
                    scanf ("%s", movinito);
                    printf2("pong %s",movinito);
                    printdbg(debug, "# pong %s\n", movinito);
                    tente = 1;
                    continue;
                }*/
        if(movinito[0] == '{')
        {
            char completo[256];
            strcpy(completo, movinito);
            while(!strchr(movinito, '}'))
            {
                scanf2(movinito);
                strcat(completo, " ");
                strcat(completo, movinito);
            }
            printdbg(debug, "# xboard: %s\n", completo);
            tente = 1;
            continue;
        }

        if(!strcmp(movinito, "ping"))
        {
            scanf2(movinito);
            pong = atoi(movinito);
            printf2("pong %d\n", pong);
            printdbg(debug, "# xboard ping %d : xadreco pong %d\n", pong, pong);
            tente = 1;
            continue;
        }

        if(!strcmp(movinito, "san") || !strcmp(movinito, "usermove")
                || !strcmp(movinito, "sigint") || !strcmp(movinito, "sigterm")
                || !strcmp(movinito, "reuse") || !strcmp(movinito, "myname")
                || !strcmp(movinito, "variants") || !strcmp(movinito, "colors")
                || !strcmp(movinito, "ics") || !strcmp(movinito, "name")
                || !strcmp(movinito, "pause") || !strcmp(movinito, "xboard")
                || !strcmp(movinito, "protover") || !strcmp(movinito, "2")
                || !strcmp(movinito, "accepted") || !strcmp(movinito, "done")
                || !strcmp(movinito, "random") || !strcmp(movinito, "hard")
                || !strcmp(movinito, ".") || !strcmp(movinito, "computer")
                || !strcmp(movinito, "easy") || !strcmp(movinito, "aborted")
                || !strcmp(movinito, "result") || !strcmp(movinito, "1-0")
                || !strcmp(movinito, "0-1") || !strcmp(movinito, "1/2-1/2")
                || !strcmp(movinito, "illegal") || !strcmp(movinito, "*")
                || !strcmp(movinito, "?"))
        {
            tente = 1;
            printdbg(debug, "# xboard: ignoring a valid command %s\n", movinito);
            continue;
        }
        if(!strcmp(movinito, "t"))
            testajogo(movinito, tabu->meionum);
        //retorna um lance do jogo de teste (em movinito)
        //e vai la embaixo jogar ele
        if(!movi2lance(&de, &pa, movinito))   //se nao existe este lance
        {
//            printdbg(debug, "# xadreco : Error (unknown command): %s\n", movinito);
            printf2("Error (unknown command): %s\n", movinito);
            tente = 1;
            continue;
        }
        if(!valido(*tabu, de, pa, &mval))  //de/pa preenche mval
        {
//            printdbg(debug, "# xadreco : Illegal move: %s\n", movinito);
            printf2("Illegal move: %s\n", movinito);
            tente = 1;
            continue;
        }
    }
    while(tente);    // BUG checar tempo dentro do laco e dar abort se demorar, ou flag...

    if(mval.especial >= 4 && mval.especial <= 7)  //promocao do peao 4,5,6,7: Dama, Cavalo, Torre, Bispo
    {
        switch(movinito[4])  //exemplo: e7e8q
        {
            case 'q': //dama
                mval.especial = 4;
                break;
            case 'n': //cavalo
                mval.especial = 5;
                break;
            case 'r': //torre
                mval.especial = 6;
                break;
            case 'b': //bispo
                mval.especial = 7;
                break;
            default: //se erro, vira dama
                mval.especial = 4;
                break;
        }
    }
    //joga_em e calcula tempo //humano joga
    res = joga_em(tabu, mval, 1);

    //a funcao joga_em deve inserir no listab cod==1
    if(analise == 1)  // analise outro movimento
        disc = analisa(tabu);
    return (res);
    //vez da outra cor jogar. retorna a situacao.
} //fim do huma_joga---------------

int valido(tabuleiro tabu, int de, int pa, movimento *result)
{
    IFDEBUG("valido()");

    arena aval;
    lista *llval = NULL;
    no *n;
    movimento *m;

    arena_inicia(&aval, 64 * 1024);
    if(lst_cria(&aval, &llval))
        msgsai("# Erro arena cheia em valido() lst_cria", 39);
    geramov(tabu, llval, de); // de (0-63) como geramodo: gera apenas desta casa

    n = llval->cabeca;
    while(n)
    {
        m = (movimento *)n->info;
        if(m->de == de && m->pa == pa)
        {
            *result = *m;
            arena_destroi(&aval);
            return 1;
        }
        n = n->prox;
    }
    arena_destroi(&aval);
    return 0;
} //fim da valido

char situacao(tabuleiro tabu)
{
    // pega o tabuleiro e retorna: M,m,a,p,i,5,r,T,t,x
    // tabu.situa:      retorno da situacao:
    //       0                              -               tudo certo... nada a declarar
    //       1                              a:              empate por afogamento
    //       1                              p:              empate por xeque-perpetuo
    //       1                              i:              empate por insuficiencia de material
    //       1                              5:              empate apos 50 lances sem movimento de peao e captura
    //       1                              r:              empate apos repetir uma mesma posicao 3 vezes
    //       2                              x:              xeque
    //      3,4                             M,m:            xeque-mate (nas Brancas e nas Pretas respec.)
    //      5,6                             T,t:            o tempo acabou (das Brancas e das Pretas respec.)
    //      7                               *:              {Game was unfinished}
    int insuf_branca = 0, insuf_preta = 0;
    int i, j;
    if(tabu.empate_50 >= 50.0)  //empate apos 50 lances sem captura ou movimento de peao
        return ('5');
    for(i = 0; i < 8; i++)  //insuficiencia de material
        for(j = 0; j < 8; j++)
        {
            switch(TIPO(tabu.tab[SQ(i, j)]))
            {
                case DAMA:
                case TORRE:
                case PEAO:
                    if(EHBRANCA(tabu.tab[SQ(i, j)]))
                        insuf_branca += 3;
                    else
                        insuf_preta += 3;
                    break;
                case BISPO:
                    if(EHBRANCA(tabu.tab[SQ(i, j)]))
                        insuf_branca += 2;
                    else
                        insuf_preta += 2;
                    break;
                case CAVALO:
                    if(EHBRANCA(tabu.tab[SQ(i, j)]))
                        insuf_branca++;
                    else
                        insuf_preta++;
                    break;
            }
            if(insuf_branca > 2 || insuf_preta > 2)
                break;
        }
    if(insuf_branca < 3 && insuf_preta < 3)  //os dois estao com insuficiencia de material
        return ('i');
    //repeticao: detectada em tab_insere(), chamada por joga_em()
    if(!geramov(tabu, NULL, GERA_UNICO))  //Sem lances: Mate ou Afogamento.
    {
        if(!xeque_rei_das(tabu.vez, tabu))
            return ('a'); //empate por afogamento.
        else
            if(tabu.vez == BRANCO)
                return ('M'); //as brancas estao em xeque-mate
            else
                return ('m'); //as pretas estao em xeque-mate
    }
    else //Tem lances
    {
        if(xeque_rei_das(tabu.vez, tabu))
            return ('x');
    }
    return ('-'); //nada
}

// ------------------------------- jogo do computador -----------------------
char compjoga(tabuleiro *tabu)
{
    IFDEBUG("compjoga()");
    // declaracao de variaveis locais ---------------------------------------
    char res;
    int i;
    int nv = 1;
    int melhorvalor1;
    int melhorou;
    int engine_val;
    int moveto;
    movimento *succ;
    int val;
    no *n;
    limpa_pensa();  //limpa algumas variaveis para iniciar a ponderacao
    // valores absolutos: positivo=bom para brancas
    val = 0; // neutro: loop while(abs(val) < XEQUEMATE) precisa comecar abaixo de XEQUEMATE
    if(tabu->vez == BRANCO)
        melhorvalor1 = -LIMITE; // brancas quer o maximo
    else
        melhorvalor1 = +LIMITE; // pretas quer o minimo

    // debug e inicializacoes -----------------------------------------------
    if(debug == 2)  //nivel extra de debug
    {
        fmini = fopen("minimax.log", "w");
        if(fmini == NULL)
            debug = 1;
    }

    // checar se usa livro --------------------------------------------------
    if(USALIVRO && tabu->meionum < 52 && setboard != 1 && !randomchess)
    {
        usalivro(*tabu);
        if(melhor.tamanho == 0)
            USALIVRO = 0;
        melhorvalor1 = melhor.valor;
    }

    // se nao livro: random ou minimax -------------------------------------
    if(melhor.tamanho == 0)
    {
        //busca com aprofundamento iterativo: nv controla a profundidade, nivel so para display
        nv = 1;
        lst_recria(&plmov);
        geramov(*tabu, plmov, GERA_TUDO);  //gera os sucessores
        totalnodo = 0;
        //primeiro lance: joga rapido, metade do tempo, maximo 10s
        if(tabu->meionum <= 1)
        {
            tempomovclock /= 2.0;
            if(tempomovclock > 8.0)
                tempomovclock = 8.0;
            if(tempomovclock < 0.5)
                tempomovclock = 0.5;
        }
        //randomchess: sorteia um lance da lista
        if(randomchess)
        {
            n = plmov->cabeca;
            val = 0;
            moveto = (int)(rand() % plmov->qtd);  //sorteia um lance possivel da lista de lances
            for(i = 0; i < moveto; ++i)
                if(n != NULL)
                    n = n->prox; //escolhe este lance como o que sera jogado

            if(n != NULL)
            {
                succ = (movimento *)n->info;
                melhor.linha[0] = *succ;
                melhor.tamanho = 1;
                succ->valor_estatico = 0;
                val = 0;
                melhorvalor1 = val;
            } //if n
        } //end if randomchess
        else
        {
            while(abs(val) < XEQUEMATE) // enquanto nao achou mate para nenhum lado
            {
                totalnodonivel = 0;
                profflag = 1;
                if(debug == 2)  //nivel extra de debug
                {
                    fprintf(fmini, "#\n#\n# *************************************************************");
                    fprintf(fmini, "#\n# minimax(*tabu, prof=0, alfa=%d, beta=%d, nv=%d)", -LIMITE, LIMITE, nv);
                }
                val = minimax(*tabu, 0, -LIMITE, +LIMITE, nv);
                if(mel[0].tamanho == 0)
                {
                    //sem lances, pode ser que queira avancar apos mate.
                    break;
                }
                if(difclocks() < tempomovclock)
                {
                    // salva melhor se iteracao completa melhorou
                    melhorou = (tabu->vez == BRANCO) ? (val > melhorvalor1) : (val < melhorvalor1);
                    if(melhorou || melhor.tamanho == 0)
                    {
                        memcpy(melhor.linha, mel[0].linha, mel[0].tamanho * sizeof(movimento));
                        melhor.tamanho = mel[0].tamanho;
                        melhor.valor = val;
                        melhorvalor1 = val;
                    }
                }
                else
                    if(melhor.tamanho > 0) /* time exceeded and we have a move: stop now */
                        break;
                totalnodo += totalnodonivel;
                lst_ordem(plmov);  //ordena lista de movimentos
                if(debug == 2)
                {
                    fprintf(fmini, "#\n# val: %+.2f totalnodo: %d\n# pv: ", val / 100.0, totalnodo);
                    if(!mostrapensando || abs(val) == FIMTEMPO || abs(val) == LIMITE)
                        imprime_linha(&mel[0], 1, 2);
                }
                if(mostrapensando && abs(val) != FIMTEMPO && abs(val) != LIMITE)
                {
                    // XBoard: score do ponto de vista da engine (positivo=engine ganhando)
                    printf("%3d %+6d %3d %7d ", nv, (tabu->vez == BRANCO) ? val : -val, (int)difclocks(), totalnodo);
                    imprime_linha(&mel[0], tabu->meionum + 1, ADV(tabu->vez));
                }
                // termino do laco infinito baseado no tempo
                if((difclocks() > tempomovclock && debug != 2) || (debug == 2 && nv == 5))
                {
                    if(mel[0].tamanho == 0)
                    {
                        printdbg(debug, "# compjoga: Sem lances; difclocks()>tempomovclock; mel[0].tamanho==0; (!break);\n");
                    }
                    else
                        break;
                }
                nv++; // busca em amplitude: aumenta um nivel.
            } //while val < XEQUEMATE
        } // else (not randomchess)
    } // fim do se nao usou livro
    melhor.valor = melhorvalor1;
    //nivel extra de debug
    if(debug == 2)
        fclose(fmini);
    //Utilizado: ABANDONA==-2730, alterado quando contra outra engine
    //resigna se estiver perdendo muito (absoluto: brancas negativo, pretas positivo)
    if((tabu->vez == BRANCO && melhor.valor < ABANDONA) || (tabu->vez == PRETO && melhor.valor > -ABANDONA))
    {
        printdbg(debug, "# xadreco : resign. value: %+.2f\n", melhor.valor / 100.0);
        --ofereci;
        printf2("resign\n");
    }
    //pode oferecer empate duas vezes (caso !randomchess). Uma assim que perder 2 peoes. Outra apos 60 lances e com menos de 2 peoes
//    if(!randomchess)
//    {
//        if (tabu->meionum >= MOVE_EMPATE2 && ofereci == 0)
//            ofereci = 1;
//    }
//    else // randomchess
    if(tabu->meionum > MOVE_EMPATE1 && ofereci > 0 && randomchess) //posso oferecer empates
    {
        if((int)(rand() % 2)) //sorteio 50% de chances
        {
            /* printf2("offer draw\n"); */
            OFERECEREMPATE = 1;
            --ofereci;
        }
    }

    //oferecer empate: valor absoluto, engine perde se brancas negativo ou pretas positivo
    engine_val = (tabu->vez == BRANCO) ? melhor.valor : -melhor.valor; // engine perspective
    if(engine_val < QUANTO_EMPATE1 && (tabu->meionum > MOVE_EMPATE1 && tabu->meionum < MOVE_EMPATE2) && ofereci > 0)
    {
        printdbg(debug, "# xadreco : offer draw (1) value: %+.2f\n", melhor.valor / 100.0);
        /* printf2("offer draw\n"); */
        OFERECEREMPATE = 1;
        --ofereci;
    }
    if(engine_val < QUANTO_EMPATE2 && tabu->meionum >= MOVE_EMPATE2 && ofereci > 0)
    {
        printdbg(debug, "# xadreco : offer draw (2) value: %+.2f\n", melhor.valor / 100.0);
        /* printf2("offer draw\n"); */
        OFERECEREMPATE = 1;
        --ofereci;
    }
    //Nova definicao: sem lances, pode ser que queira avancar apos mate.
    //algum problema ocorreu que esta sem lances
    if(melhor.tamanho == 0)
    {
        res = randommove(tabu);
        if(res == 'e')
            return res; //vazio mesmo! Nem aleatorio foi!
        printdbg(debug, "# xadreco : Error. I don't know what to play... Playing a random move (compjoga)!\n");
    }
    assert(melhor.tamanho > 0 && "melhor.tamanho == 0 before joga_em in compjoga");
    res = joga_em(tabu, melhor.linha[0], 1);  // computador joga
    return (res);
} //fim da compjoga

char analisa(tabuleiro *tabu)
{
    IFDEBUG("analisa()");
    tabuleiro tanalise;
    int nv = 0;
    int val;
    // lances calc. em maior nivel tem mais importancia?
    //mudou para logo abaixo do scanf de humajoga
    val = (tabu->vez == BRANCO) ? -LIMITE : +LIMITE;
    copitab(&tanalise, tabu);
    limpa_pensa();
    if(debug == 2)
        //nivel extra de debug
    {
        fmini = fopen("minimax.log", "w");
        if(fmini == NULL)
            debug = 1;
    }
    //busca com aprofundamento iterativo: nv controla a profundidade
    if(USALIVRO && tabu->meionum < 50 && setboard != 1)
        usalivro(*tabu);
    if(melhor.tamanho > 0)
    {
        printf("%3d %+6d %3d %7d ", nv, (tabu->vez == BRANCO) ? melhor.valor : -melhor.valor, (int)difclocks(), totalnodo);
        imprime_linha(&melhor, tabu->meionum + 1, ADV(tabu->vez));
    }
    else
    {
        nv = 1;
        lst_recria(&plmov);
        geramov(*tabu, plmov, GERA_TUDO);  //gera os sucessores
        totalnodo = 0;
        while(abs(val) < XEQUEMATE) // enquanto nao achou mate para nenhum lado
        {
            totalnodonivel = 0;
            profflag = 1;
            val = minimax(*tabu, 0, -LIMITE, LIMITE, nv);
            totalnodo += totalnodonivel;
            lst_ordem(plmov);
            if(abs(val) != FIMTEMPO && abs(val) != LIMITE)
            {
                // XBoard: score do ponto de vista da engine
                printf("%3d %+6d %3d %7d ", nv, (tabu->vez == BRANCO) ? val : -val, (int)difclocks(), totalnodo);
                imprime_linha(&mel[0], tabu->meionum + 1, ADV(tabu->vez));
            }
            if(debug == 2)
            {
                fprintf(fmini, "#\n# val: %+.2f totalnodo: %d\npv: ", val / 100.0, totalnodo);
                imprime_linha(&mel[0], 1, 2);
            }
            if((difclocks() > tempomovclock && debug != 2) || (debug == 2 && nv == 5))
                break;
            if(mel[0].tamanho == 0)
                break;
            nv++;
        }
        memcpy(melhor.linha, mel[0].linha, mel[0].tamanho * sizeof(movimento));
        melhor.tamanho = mel[0].tamanho;
        melhor.valor = val;
    }
    if(debug == 2)          //nivel extra de debug
        fclose(fmini);
    if(melhor.tamanho == 0)
        //...apos o termino da partida, so pode-se usar edicao, undo, etc.
        //algum problema ocorreu que esta sem lances
        return ('e');
    //erro 35! computador sem lances!? nivel deve ser > zero
    return ('-');
    //tudo certo... (situa)
}

//--------------------------------------------------------------------------
//tabuleiro atual, profundidade zero, limite maximo de estatico (beta ou uso), limite minimo de estatico (alfa ou passo), nivel da busca
//Finding the rotten fish in the second bag was like exceeding beta.
//If there had been no fish in the bag, determining that the six-pack of pop
//bag was better than the sandwich bag would have been like exceeding alpha (one ply back).
//source: http://www.seanet.com/~brucemo/topics/alphabeta.htm
int minimax(tabuleiro atual, int prof, int alfa, int beta, int niv)
{
    movimento *succ;
    int novo_valor, child_val, contamov = 0;
    tabuleiro tab;
    char m[8];
    no *n;
    lista *llmov = NULL;
    size_t saved;

    assert(prof >= 0 && alfa <= beta && "Invalid minimax parameters");
    mel[prof].tamanho = 0; // inicializa PV vazio (evita dados stale de iteracao anterior)

    if(profsuf(atual, prof, alfa, beta, niv, &child_val))
    {
        //profsuf preencheu mel[prof] e child_val
        return child_val;
    }
    if(debug == 2)
    {
        fprintf(fmini, "#\n#----------------------------------------------Minimax prof: %d", prof);
        //fprintf(fmini, "\nalfa= %d     beta= %d", alfa, beta);
    }
    if(prof == 0)
        n = plmov->cabeca; // lista de lances do tabuleiro real
    else
    {
        saved = plmov->a->usado;  //bookmark
        if(lst_cria(plmov->a, &llmov))
            msgsai("# Erro arena cheia em minimax lst_cria", 39);
        geramov(atual, llmov, GERA_TUDO); // gerar lista de lances para tabuleiro imaginario
        n = llmov->cabeca;
    }

    if(!n)
    {
        //entao o estatico refletira isso: afogamento
        mel[prof].tamanho = 0;
        child_val = estatico(atual, 0, prof, alfa, beta);
        if(debug == 2)
            fprintf(fmini, "#NULL ");
        if(prof != 0)
            plmov->a->usado = saved;  //rewind arena
        return child_val;
    }
    // minimax classico: brancas maximizam, pretas minimizam
    // alfa = lower bound (melhor para brancas), beta = upper bound (melhor para pretas)
    // valores absolutos: positivo = bom para brancas
    novo_valor = (atual.vez == BRANCO) ? -LIMITE : +LIMITE; // best inicial
    while(n)
    {
        succ = (movimento *)n->info;
        copitab(&tab, &atual);
        disc = (char) joga_em(&tab, *succ, 1);
        //joga o lance atual, a funcao joga_em deve inserir no listab
        totalnodonivel++;
        profflag = succ->flag_50 + 1;	//se for zero, fim da busca.
        //flag_50:0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu;
        //flag_50== 2 ou 3 : houve captura :Liberou
        //tab.situa:0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.) 7: sem resultado
        switch(tab.situa)
        {
            case 0:  //0:nada... Quem decide eh flag_50;
                break;
            case 2:  //2:Xeque!  Liberou
                profflag = 4;
                break;
            default: //situa: 1=Empate, 3,4=Mate, 5,6=Tempo. 7=sem resultado. Nao passar o nivel
                profflag = 0;
        }
        if(debug == 2)
        {
            lance2movi(m, succ->de, succ->pa, succ->especial);
            fprintf(fmini, "#\n# nivel %d, %d-lance %s (%d%d%d%d):", prof, totalnodonivel, m, COL(succ->de), ROW(succ->de), COL(succ->pa), ROW(succ->pa));
        }
        child_val = minimax(tab, prof + 1, alfa, beta, niv); // sem inversao de janela
        lst_remove(pltab);  //retira o ultimo tabuleiro da lista
        if(atual.vez == BRANCO) // MAXIMIZA
        {
            if(child_val > novo_valor)
            {
                novo_valor = child_val;
                mel[prof].linha[0] = *succ;
                memcpy(&mel[prof].linha[1], &mel[prof + 1].linha[0], mel[prof + 1].tamanho * sizeof(movimento));
                mel[prof].tamanho = mel[prof + 1].tamanho + 1;
            }
            if(novo_valor > alfa)
                alfa = novo_valor;
        }
        else // MINIMIZA
        {
            if(child_val < novo_valor)
            {
                novo_valor = child_val;
                mel[prof].linha[0] = *succ;
                memcpy(&mel[prof].linha[1], &mel[prof + 1].linha[0], mel[prof + 1].tamanho * sizeof(movimento));
                mel[prof].tamanho = mel[prof + 1].tamanho + 1;
            }
            if(novo_valor < beta)
                beta = novo_valor;
        }
        // corte alfa-beta (comum a ambos)
        if(alfa >= beta)
        {
            if(debug == 2)
            {
                lance2movi(m, succ->de, succ->pa, succ->especial);
                fprintf(fmini, "#\n# succ: alfa>=beta (%+.2f>=%+.2f) %s Corte!", alfa / 100.0, beta / 100.0, m);
            }
            break;
        }
        if(debug == 2 && prof == 0)
        {
            lance2movi(m, succ->de, succ->pa, succ->especial);
            fprintf(fmini, "#\n# child_val=%+.2f best=%+.2f", child_val / 100.0, novo_valor / 100.0);
            if(mel[prof].tamanho > 0)
            {
                fprintf(fmini, "#\n# melhor_caminho=");
                imprime_linha(&mel[prof], 1, 2);
            }
        }
        if(prof == 0)
            succ->valor_estatico = (atual.vez == BRANCO) ? child_val : -child_val; // para lst_ordem (descending)
        n = n->prox;
        contamov++;
        if(prof != 0 && contamov > llmov->qtd * PORCENTO_MOVIMENTOS + 1)
            break;
    } //while(n)
    if(prof != 0)
        plmov->a->usado = saved;  //rewind arena
    if(debug == 2)
        fprintf(fmini, "#\n#------------------------------------------END Minimax prof: %d", prof);
    return novo_valor;
}

int profsuf(tabuleiro atual, int prof, int alfa, int beta, int niv, int *valor)
{
    char input;

    //limite absoluto de profundidade: protege mel[prof] de overflow
    if(prof >= MAX_PROF)
    {
        mel[prof - 1].tamanho = 0;
        *valor = estatico(atual, 0, prof, alfa, beta);
        return 1;
    }
    //se tem captura ou xeque... liberou
    //se ja passou do nivel estipulado, pare a busca incondicionalmente
    if(prof >= niv)
    {
        mel[prof].tamanho = 0;
        *valor = estatico(atual, 0, prof, alfa, beta);
        //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. Prof: qual nivel estao analisando?)
        return 1;
    }
    //retorna sem analisar... Deve desconsiderar o lance
    if(difclocks() >= tempomovclock && debug != 2)
    {
        mel[prof].tamanho = 0;
        *valor = estatico(atual, 1, prof, alfa, beta);	//-FIMTEMPO;//
        return 1;
    }

    if(difftime(tatual, tultimoinput) >= 1) //faz poll uma vez por segundo apenas
    {
        // printf("(%d) ",clock2 - inputcheckclock);
        if(pollinput())
        {
            do
            {
                input = getc(stdin);
                // printf("input: %c", input);
            }
            while((input == '\n' || input == '\r') && pollinput());
            // printf ("%c\n", input);
            // ungetc (input, stdin);
            if(input == '?')
            {
                mel[prof].tamanho = 0;
                *valor = estatico(atual, 1, prof, alfa, beta);  //-FIMTEMPO;//
                ungetc(input, stdin);
                // teminterroga=1;
                // printf("teminterroga: %d",teminterroga);
                return 1;
            }
            else
            {
                ungetc(input, stdin);
                // inputcheckclock = clock2;
                tultimoinput = time(NULL);
                // return 0;
            }
        }
    }

    if(!profflag) //nao liberou profflag==0 retorna
    {
        mel[prof].tamanho = 0;
        *valor = estatico(atual, 0, prof, alfa, beta); //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. Prof: qual nivel estao analisando?)
        return 1; //a profundidade ja eh sufuciente
    }
    return 0; //se OU Nem-Chegou-no-Nivel OU Liberou, pode ir fundo
}

char joga_em(tabuleiro *tabu, movimento movi, int cod)
{
    char res;

    if(movi.especial == 7)  //promocao do peao: BISPO
        tabu->tab[movi.de] = DACOR(BISPO, tabu->vez);
    if(movi.especial == 6)  //promocao do peao: TORRE
        tabu->tab[movi.de] = DACOR(TORRE, tabu->vez);
    if(movi.especial == 5)  //promocao do peao: CAVALO
        tabu->tab[movi.de] = DACOR(CAVALO, tabu->vez);
    if(movi.especial == 4)  //promocao do peao: DAMA
        tabu->tab[movi.de] = DACOR(DAMA, tabu->vez);
    if(movi.especial == 3)  //comeu en passant
        tabu->tab[SQ(tabu->peao_pulou, ROW(movi.de))] = VAZIA;
    if(movi.especial == 2)  //roque grande
    {
        tabu->tab[SQ(0, ROW(movi.de))] = VAZIA;
        tabu->tab[SQ(3, ROW(movi.de))] = DACOR(TORRE, tabu->vez);
    }
    if(movi.especial == 1)  //roque pequeno
    {
        tabu->tab[SQ(7, ROW(movi.de))] = VAZIA;
        tabu->tab[SQ(5, ROW(movi.de))] = DACOR(TORRE, tabu->vez);
    }
    if(movi.flag_50)  //empate de 50 lances sem mover peao ou capturar
        tabu->empate_50 = 0;
    else
        tabu->empate_50 += .5;
    if(tabu->vez == BRANCO)
        switch(movi.roque)  //avalia o que este lance causa para futuros roques
        {
            case 0: //mexeu o rei
                tabu->roqueb = 0;
                break;
            case 1: //nao fez lance que interfere o roque
                break;
            case 2: //mexeu torre do rei
                if(tabu->roqueb == 1 || tabu->roqueb == 2)
                    tabu->roqueb = 2;
                else
                    tabu->roqueb = 0;
                break;
            case 3: //mexeu torre da dama
                if(tabu->roqueb == 1 || tabu->roqueb == 3)
                    tabu->roqueb = 3;
                else
                    tabu->roqueb = 0;
                break;
        }
    else //vez das pretas
        switch(movi.roque)  //avalia o que este lance causa para futuros roques
        {
            case 0: //mexeu o rei
                tabu->roquep = 0;
                break;
            case 1: //nao fez lance que interfere o roque
                break;
            case 2: //mexeu torre do rei
                if(tabu->roquep == 1 || tabu->roquep == 2)
                    tabu->roquep = 2;
                else
                    tabu->roquep = 0;
                break;
            case 3: //mexeu torre da dama
                if(tabu->roquep == 1 || tabu->roquep == 3)
                    tabu->roquep = 3;
                else
                    tabu->roquep = 0;
                break;
        }
    tabu->peao_pulou = movi.peao_pulou;
    tabu->tab[movi.pa] = tabu->tab[movi.de];
    tabu->tab[movi.de] = VAZIA;
    if(TIPO(tabu->tab[movi.pa]) == REI)
        tabu->rei_pos[ICOR(tabu->vez)] = movi.pa; // vez ainda nao inverteu
    tabu->de = movi.de;
    tabu->pa = movi.pa;
    tabu->meionum++;
    //conta os movimentos de cada peao (e chamado de dentro de funcao recursiva!)
    tabu->vez = ADV(tabu->vez);
    tabu->especial = movi.especial;
    if(cod)
    {
        int rep = tab_insere(*tabu);
        if(rep >= 3)
        {
            tabu->situa = 1;
            return 'r';
        }
    }
    res = situacao(*tabu);
    switch(res)
    {
        case 'a': //afogado
        case 'p': //perpetuo
        case 'i': //insuficiencia
        case '5': //50 lances
        case 'r': //repeticao
            tabu->situa = 1;
            break;
        case 'x': //xeque
            tabu->situa = 2;
            break;
        case 'M': //Brancas perderam por Mate
            tabu->situa = 3;
            break;
        case 'm': //Pretas perderam por mate
            tabu->situa = 4;
            break;
        case 'T': //Brancas perderam por Tempo
            tabu->situa = 5;
            break;
        case 't': //Pretas perderam  por tempo
            tabu->situa = 6;
            break;
        case '*': //sem resultado * {Game was unfinished}
            tabu->situa = 7;
            break;
        default: //'-' nada...
            tabu->situa = 0;
    }
    return (res);
} //fim da joga_em

//retorna o valor tatico e estrategico de um tabuleiro. Absoluto: positivo = bom para brancas
//cod: 1: acabou o tempo, 0: ou eh avaliacao normal?
//niv: qual a distancia do tabuleiro real para a copia tabu avaliada?
int estatico(tabuleiro tabu, int cod, int niv, int alfa, int beta)
{
    if(cod) //time expired: skip evaluation, abandon this branch
        return alfa;

    int totb = 0, totp = 0;
    int i, j, k, cor, peca;
    int peaob, peaop, isob, isop, pecab = 0, pecap = 0, material;
    int menorb = 0, menorp = 0;
    int qtdb, qtdp;
    int peca_movida;
    int ordem[64][7];
    //coloca todas pecas do tabuleiro em ordem de valor
    //64 casas, cada uma com 7 info: 0:i, 1:j, 2:peca,
    //3: qtdataquebranco, 4: menorb, 5: qtdataquepreto, 6: menorp
    //levando em conta a situacao do tabuleiro
    switch(tabu.situa)
    {
        case 3: //Brancas tomaram mate. Sempre negativo (ruim para brancas).
            return -XEQUEMATE + 20 * (niv + 1);
        case 4: //Pretas tomaram mate. Sempre positivo (bom para brancas).
            return +XEQUEMATE - 20 * (niv + 1);
        case 1: //Empatou
            return 0;
        //valor de uma posicao empatada.
        case 2: //A posicao eh de XEQUE!
            //analisada abaixo. nao precisa mais desse.
            //        if (tabu.meionum > 40)
            //xeque no meio-jogo e final vale mais que na abertura
            if(tabu.vez == BRANCO)
                //                totp += 30;
                //            else
                //                totb += 30;
                //        else if (tabu.vez == BRANCO)
                totp += 10;		//10 centi-peoes
            else
                totb += 10;
    }

    //coloca todas pecas do tabuleiro em ordem de valor
    //64 casas, cada uma com 7 info: 0:i, 1:j, 2:peca,
    //3: qtdataquebranco, 4: menorb, 5: qtdataquepreto, 6: menorp

    //levando em conta o valor material de cada peca,
    //acha os reis.
    k = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(TIPO(peca) != REI)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(BRANCO, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(PRETO, i, j, tabu, &ordem[k][6]);
            if(EHBRANCA(peca))
                //caso REI branco
            {
                totp += ordem[k][5] * 20;
                //                pecab+=peca; //nao soma mais o REI!
            }
            //pretas ganham 5 pontos por ataque ao REI
            else
            {
                totb += ordem[k][3] * 20;
                //                pecap+=peca; //nao soma mais o REI!
            }
            k++;
            if(k > 1)
                break;
        }
        if(k > 1)
            break;
    }
    //acha damas.
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(TIPO(peca) != DAMA)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(BRANCO, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(PRETO, i, j, tabu, &ordem[k][6]);
            if(EHBRANCA(peca))
            {
                //caso peca branca
                //pretas ganham 5 pontos por ataque nela
                totp += ordem[k][5] * 20;
                //dama branca no ataque ganha bonus
                if(j > 4 && tabu.meionum > 30)
                    totb += 90;
                pecab += val[TIPO(peca)];
            }
            else
            {
                totb += ordem[k][3] * 20;
                if(j < 3 && tabu.meionum > 30)
                    totp += 90;
                pecap += val[TIPO(peca)];
            }
            k++;
        }
    //acha torres.
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(TIPO(peca) != TORRE)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(BRANCO, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(PRETO, i, j, tabu, &ordem[k][6]);
            if(EHBRANCA(peca))
            {
                //caso peca branca
                //pretas ganham 5 pontos por ataque nela
                totp += ordem[k][5] * 20;
                //torre branca no ataque ganha bonus
                if(j > 4)
                    totb += 90;
                pecab += val[TIPO(peca)];
            }
            else
            {
                totb += ordem[k][3] * 20;
                if(j < 3)
                    totp += 90;
                pecap += val[TIPO(peca)];
            }
            k++;
        }
    // acha bispos
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(TIPO(peca) != BISPO)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(BRANCO, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(PRETO, i, j, tabu, &ordem[k][6]);
            if(EHBRANCA(peca))
            {
                //caso peca branca
                totp += ordem[k][5] * 10;
                pecab += val[TIPO(peca)];
            }
            //pretas ganham pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 10;
                pecap += val[TIPO(peca)];
            }
            k++;
        }
    //acha cavalos.
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(TIPO(peca) != CAVALO)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(BRANCO, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(PRETO, i, j, tabu, &ordem[k][6]);
            if(EHBRANCA(peca))
            {
                //caso peca branca
                totp += ordem[k][5] * 10;
                pecab += val[TIPO(peca)];
            }
            //pretas ganham 5 pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 10;
                pecap += val[TIPO(peca)];
            }
            k++;
        }
    //acha peoes.
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(TIPO(peca) != PEAO)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(BRANCO, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(PRETO, i, j, tabu, &ordem[k][6]);
            if(EHBRANCA(peca))
            {
                //caso peca branca
                totp += ordem[k][5] * 10;
                pecab += val[TIPO(peca)];
            }
            //pretas ganham 5 pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 10;
                pecap += val[TIPO(peca)];
            }
            k++;
        }
    if(k < 64)
        ordem[k][0] = -1;
    //sinaliza o fim
    //as pecas estao em ordem de valor

    //--------------------------- lazy evaluation
    //ponto de vista branco (branco eh positivo, preto eh negativo). Absoluto: positivo = bom para brancas.
    material = (int)((1.0 + 75.0 / (float)(pecab + pecap)) * (float)(pecab - pecap));

    //hanging piece: se a peca que acabou de mover esta atacada, penaliza
    peca_movida = TIPO(tabu.tab[tabu.pa]);
    assert(COR(tabu.tab[tabu.pa]) == ADV(tabu.vez) && "peca no destino deve ser de quem moveu");
    if(peca_movida != REI && peca_movida != 0
       && ataca(tabu.vez, COL(tabu.pa), ROW(tabu.pa), tabu))
    {
        // peca pendurada: quem moveu perde valor. Absoluto: positivo = bom para brancas.
        if(EHBRANCA(tabu.tab[tabu.pa]))
            material -= val[peca_movida]; // peca branca pendurada, ruim para brancas
        else
            material += val[peca_movida]; // peca preta pendurada, bom para brancas
    }

    //lazy: material even with best positional bonus can't beat alfa
    if(material + MARGEM_PREGUICA <= alfa)
    {
        if(debug == 2)
            fprintf(fmini, "lazy: material+margin <= alfa (%+.2f <= %+.2f)\n", (material + MARGEM_PREGUICA) / 100.0, alfa / 100.0);
        return (material + MARGEM_PREGUICA);
    }
    //lazy: material even with worst positional penalty still beats beta
    if(material - MARGEM_PREGUICA >= beta)
    {
        if(debug == 2)
            fprintf(fmini, "lazy: material-margin >= beta (%+.2f >= %+.2f)\n", (material - MARGEM_PREGUICA) / 100.0, beta / 100.0);
        return (material - MARGEM_PREGUICA);
    }

    //usar o 'material'
    //totb+=pecab;
    //totp+=pecap;

    //e a quantidade de ataques nelas
    for(k = 0; k < 64; k++)
    {
        if(ordem[k][0] == -1)
            break;
        i = ordem[k][0];
        j = ordem[k][1];
        peca = ordem[k][2];
        if(EHBRANCA(peca))
        {
            //se peca branca:
            //totb += abs(peca);
            //qtdp = qataca(PRETO, i, j, tabu, &menorp);
            //se uma peca preta ja comeu, ela nao pode contar de novo como atacante! (corrigir)
            if(ordem[k][5] == 0)
                continue;
            //qtdb = qataca(BRANCO, i, j, tabu, &menorb);
            //defesas
            if(ordem[k][5] > ordem[k][3])
                totp += (ordem[k][5] - ordem[k][3]) * 10;
            else
                totb += (ordem[k][3] - ordem[k][5] + 1) * 10;
            if(ordem[k][6] < val[TIPO(peca)])
                totp += 50;
        }
        else
        {
            //se peca preta:
            //totp += peca;
            //qtdb = qataca(BRANCO, i, j, tabu, &menorb);
            if(ordem[k][3] == 0)
                continue;
            //qtdp = qataca(PRETO, i, j, tabu, &menorp);
            if(ordem[k][3] > ordem[k][5])
                totb += (ordem[k][3] - ordem[k][5]) * 10;
            else
                totp += (ordem[k][5] - ordem[k][3] + 1) * 10;
            if(ordem[k][4] < val[TIPO(peca)])
                totb += 50;
        }
    }

    //falta explicar como contar defesas com pecas cravadas
    //falta explicar que o peao passado vale muito!
    //e passado protegido de peao vale mais
    //no final, cercar o rei
    //nao esta vendo 2 defesas por exemplo com P....RQ na mesma linha. O peao tem
    //defesa da torre e da dama.

    //incluindo peca cravada
    for(k = 2; k < 64; k++)
        //tirando os dois reis da jogada k=2
    {
        if(ordem[k][0] == -1)
            //todas pecas podem ser cravadas, exceto Rei
            break;
        i = ordem[k][0];
        //as pecas estao em ordem de valor
        j = ordem[k][1];
        peca = ordem[k][2];
        //        if (tabu.vez == BRANCO && peca > 0)
        //nao eh meu prejuizio a peca cravada do adversario
        //            continue;
        //        if (tabu.vez == PRETO && peca < 0)
        //nao eh meu prejuizio a peca cravada do adversario
        //            continue;
        tabu.tab[SQ(i, j)] = VAZIA;
        //imagina se essa peca nao existisse?
        if(EHBRANCA(peca))
        {
            qtdp = qataca(PRETO, i, j, tabu, &menorp);
            if(qtdp > ordem[k][5] && menorp < val[TIPO(peca)])
                totb -= (val[TIPO(peca)] / 7);
        }
        else
        {
            qtdb = qataca(BRANCO, i, j, tabu, &menorb);
            if(qtdb > ordem[k][3] && menorb < val[TIPO(peca)])
                totp -= (val[TIPO(peca)] / 7);
        }
        //perde uma fracao do valor da peca cravada;
        tabu.tab[SQ(i, j)] = peca;
        //recoloca a peca no lugar.
    }
    //avaliando os peoes avancados
    for(k = 0; k < 64; k++)
    {
        if(ordem[k][0] == -1)
            break;
        if(TIPO(ordem[k][2]) != PEAO)
            continue;
        i = ordem[k][0];
        j = ordem[k][1];
        if(ordem[k][2] == DACOR(PEAO, BRANCO))  //Peao branco (white pawn)
        {
            //faltando 4 casas ou menos para promover ganha +1
            if(j > 2)
                totb += 10;
            //faltando 3 casas ou menos para promover ganha +1+1
            if(j > 3)
                totb += 20;
            //faltando 2 casas ou menos para promover ganha +1+1+3
            if(j > 4)
                totb += 30;
            //faltando 1 casas ou menos para promover ganha +1+1+3+3
            if(j > 5)
                totb += 40;
        }
        else //Peao preto (black pawn)
        {
            //faltando 4 casas ou menos para promover ganha +1
            if(j < 5)
                totp += 10;
            //faltando 3 casas ou menos para promover ganha +1+1
            if(j < 4)
                totp += 20;
            //faltando 2 casas ou menos para promover ganha +1+1+3
            if(j < 3)
                totp += 30;
            //faltando 1 casas ou menos para promover ganha +1+1+3+3
            if(j < 2)
                totp += 40;
        }
    }
    //peao dobrado e isolado.
    isob = 1;
    isop = 1;
    // Maquina de Estados de isob
    for(i = 0; i < 8; i++)
        //for(j=1;j<7;j++)
        // isob| achou peao na col | nao achou
    {
        //  0  |   isob = 0        | isob = 1
        peaob = 0;
        //inicio: 1  |   isob = 2        | isob = 1
        peaop = 0;
        //  2  |   isob = 0        | isob = 3
        for(j = 1; j < 7; j++)
            //for(i=0;i<8;i++)
            //PEAO ISOLADO! 3  |   isob = 2        | isob = 1
        {
            if(tabu.tab[SQ(i, j)] == VAZIA)
                continue;
            if(tabu.tab[SQ(i, j)] == DACOR(PEAO, BRANCO))
                peaob++;
            if(tabu.tab[SQ(i, j)] == DACOR(PEAO, PRETO))
                peaop++;
        }
        //peaob e peaop tem o total de peoes na coluna (um so, dobrado, trip...)
        if(peaob != 0)
            totb -= ((peaob - 1) * 30);
        //penalidade: para um so=0, para dobrado=3, trip=6!
        if(peaop != 0)
            totp -= ((peaop - 1) * 30);
        switch(isob)
        {
            case 0:
                if(!peaob)
                    //se nao achou, peaob==0
                    isob++;
                //isob=1, se achou isob continua 0
                break;
            case 1:
                if(peaob)
                    //se achou, peaob !=0
                    isob++;
                //isob=2, se nao achou isob continua 1
                break;
            case 2:
                if(peaob)
                    //se achou...
                    isob = 0;
                //isob volta a zero
                else
                    //senao...
                    isob++;
                //isob=3 => PEAO ISOLADO!
                break;
        }
        if(isob == 3)
        {
            totb -= 40; //penalidade para peao isolado
            isob = 1;
        }
        switch(isop)
        {
            case 0:
                if(!peaop)
                    //se nao achou, peaop==0
                    isop++;
                break;
            case 1:
                if(peaop)
                    //se achou, peaop !=0
                    isop++;
                break;
            case 2:
                if(peaop)
                    //se achou...
                    isop = 0;
                else
                    //senao...
                    isop++;
                //isop=3 => PEAO ISOLADO!
                break;
        }
        if(isop == 3)
        {
            totp -= 40; //penalidade para peao isolado
            isop = 1;
        }
    }
    if(isob == 2)  //o PTR branco esta isolado
        totb -= 40;
    if(isop == 2)  //o PTR preto esta isolado
        totp -= 40;

    //controle do centro
    for(cor = BRANCO; cor <= PRETO; cor += PRETO)
        for(i = 2; i < 6; i++)             //c,d,e,f
            for(j = 2; j < 6; j++)                 //3,4,5,6
                if(ataca(cor, i, j, tabu))
                {
                    if(cor == BRANCO)
                    {
                        totb += 10;
                        if((i == 3 || i == 4) && (j == 3 || j == 4))
                            totb += 20;                         //mais pontos para d4,d5,e4,e5
                    }
                    else
                    {
                        totp += 10;
                        if((i == 3 || i == 4) && (j == 3 || j == 4))
                            totp += 20;                        //mais pontos para d4,d5,e4,e5
                    }
                }
    //bonificacao para quem nao mexeu a dama na abertura
    //TODO usar flag para lembrar se ja mexeu, senao vai e volta
    if(tabu.meionum < 32 && setboard != 1)
    {
        if(tabu.tab[SQ(3, 0)] == DACOR(DAMA, BRANCO))
            totb += 50;
        if(tabu.tab[SQ(3, 7)] == DACOR(DAMA, PRETO))
            totp += 50;
    }
    //bonificacao para quem fez roque na abertura
    //TODO usar flag para saber se fez roque mesmo
    if(tabu.meionum < 32 && setboard != 1)
    {
        if(tabu.tab[SQ(6, 0)] == DACOR(REI, BRANCO) && tabu.tab[SQ(5, 0)] == DACOR(TORRE, BRANCO)) //brancas com roque pequeno
            totb += 70;
        if(tabu.tab[SQ(2, 0)] == DACOR(REI, BRANCO) && tabu.tab[SQ(3, 0)] == DACOR(TORRE, BRANCO)) //brancas com roque grande
            totb += 50;
        if(tabu.tab[SQ(6, 7)] == DACOR(REI, PRETO) && tabu.tab[SQ(5, 7)] == DACOR(TORRE, PRETO)) //pretas com roque pequeno
            totp += 70;
        if(tabu.tab[SQ(2, 7)] == DACOR(REI, PRETO) && tabu.tab[SQ(3, 7)] == DACOR(TORRE, PRETO)) //pretas com roque grande
            totp += 50;
    }
    //bonificacao para rei protegido na abertura com os peoes do Escudo Real
    if(tabu.meionum < 60 && setboard != 1)
    {
        if(tabu.tab[SQ(6, 0)] == DACOR(REI, BRANCO) &&
                //brancas com roque pequeno
                tabu.tab[SQ(6, 1)] == DACOR(PEAO, BRANCO) &&
                tabu.tab[SQ(7, 1)] == DACOR(PEAO, BRANCO) && tabu.tab[SQ(7, 0)] == VAZIA)
            //apenas peoes g e h
            totb += 60;
        if(tabu.tab[SQ(2, 0)] == DACOR(REI, BRANCO) &&
                //brancas com roque grande
                tabu.tab[SQ(0, 1)] == DACOR(PEAO, BRANCO) &&
                tabu.tab[SQ(1, 1)] == DACOR(PEAO, BRANCO) &&
                tabu.tab[SQ(2, 1)] == DACOR(PEAO, BRANCO) &&
                tabu.tab[SQ(0, 0)] == VAZIA && tabu.tab[SQ(1, 0)] == VAZIA)
            //peoes a, b e c
            totb += 50;
        if(tabu.tab[SQ(6, 7)] == DACOR(REI, PRETO) &&
                //pretas com roque pequeno
                tabu.tab[SQ(6, 6)] == DACOR(PEAO, PRETO) &&
                tabu.tab[SQ(7, 6)] == DACOR(PEAO, PRETO) && tabu.tab[SQ(7, 7)] == VAZIA)
            //apenas peoes g e h
            totp += 60;
        if(tabu.tab[SQ(2, 7)] == DACOR(REI, PRETO) &&
                //pretas com roque grande
                tabu.tab[SQ(0, 6)] == DACOR(PEAO, PRETO) &&
                tabu.tab[SQ(1, 6)] == DACOR(PEAO, PRETO) &&
                tabu.tab[SQ(2, 6)] == DACOR(PEAO, PRETO) &&
                tabu.tab[SQ(0, 7)] == VAZIA && tabu.tab[SQ(1, 7)] == VAZIA)
            //peoes a, b e c
            totp += 50;
    }
    //penalidade se mexer o peao do bispo, cavalo ou da torre no comeco da abertura!
    if(tabu.meionum < 16 && setboard != 1)
    {
        //caso das brancas------------------
        if(tabu.tab[SQ(5, 1)] != DACOR(PEAO, BRANCO))
            //PBR
            totb -= 50;
        if(tabu.tab[SQ(6, 1)] != DACOR(PEAO, BRANCO))
            //PCR
            totb -= 40;
        if(tabu.tab[SQ(7, 1)] != DACOR(PEAO, BRANCO))
            //PTR
            totb -= 30;
        if(tabu.tab[SQ(0, 1)] != DACOR(PEAO, BRANCO))
            //PTD
            totb -= 30;
        if(tabu.tab[SQ(1, 1)] != DACOR(PEAO, BRANCO))
            //PCD
            totb -= 40;
        //              if(tabu.tab[SQ(2,1)]==VAZIA)
        //PBD   nao eh penalizado!
        //                      totb-=10;
        //caso das pretas-------------------
        if(tabu.tab[SQ(5, 6)] != DACOR(PEAO, PRETO))
            //PBR
            totp -= 50;
        if(tabu.tab[SQ(6, 6)] != DACOR(PEAO, PRETO))
            //PCR
            totp -= 40;
        if(tabu.tab[SQ(7, 6)] != DACOR(PEAO, PRETO))
            //PTR
            totp -= 30;
        if(tabu.tab[SQ(0, 6)] != DACOR(PEAO, PRETO))
            //PTD
            totp -= 30;
        if(tabu.tab[SQ(1, 6)] != DACOR(PEAO, PRETO))
            //PCD
            totp -= 40;
        //PBD   nao eh penalizado!
    }
    //prepara o retorno: absoluto, positivo = bom para brancas
    return material + totb - totp;
}

//para voltar um lance
void volta_lance(tabuleiro *tabu)
{
    if(!pltab || !pltab->cabeca || !pltab->cauda)
        return;
    if(pltab->cabeca == pltab->cauda)  //ja esta na posicao inicial
        return;
    lst_remove(pltab);
    tabuleiro *t = (tabuleiro *)pltab->cauda->info;
    *tabu = *t;
    if(tabu->meionum < 50 && setboard < 1)
        USALIVRO = 1; //nao usar abertura em posicoes de setboard
}

int qataca(int cor, int col, int lin, tabuleiro tabu, int *menor)
{
    //retorna o numero de ataques que a "cor" faz na casa(col,lin)
    //cor==brancas   => brancas atacam casa(col,lin)
    //cor==pretas    => pretas  atacam casa(col,lin)
    //menor == e a menor peca da "cor" que ataca a casa (less valued piece attacking the square)
    int icol, ilin, casacol, casalin;
    int total = 0;
    int p; // peca encontrada na direcao
    *menor = val[REI];
    //torre ou dama atacam a casa...
    for(icol = col - 1; icol >= 0; icol--)  //desce coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(icol, lin)] == DACOR(TORRE, cor) || tabu.tab[SQ(icol, lin)] == DACOR(DAMA, cor))
        {
            total++;
            p = TIPO(tabu.tab[SQ(icol, lin)]);
            if(val[p] < *menor)
                *menor = val[p];
        }
        break;
    }
    for(icol = col + 1; icol < 8; icol++)  //sobe coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(icol, lin)] == DACOR(TORRE, cor) || tabu.tab[SQ(icol, lin)] == DACOR(DAMA, cor))
        {
            total++;
            p = TIPO(tabu.tab[SQ(icol, lin)]);
            if(val[p] < *menor)
                *menor = val[p];
        }
        break;
    }
    for(ilin = lin + 1; ilin < 8; ilin++)  // direita na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(col, ilin)] == DACOR(TORRE, cor) || tabu.tab[SQ(col, ilin)] == DACOR(DAMA, cor))
        {
            total++;
            p = TIPO(tabu.tab[SQ(col, ilin)]);
            if(val[p] < *menor)
                *menor = val[p];
        }
        break;
    }
    for(ilin = lin - 1; ilin >= 0; ilin--)  // esquerda na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(col, ilin)] == DACOR(TORRE, cor) || tabu.tab[SQ(col, ilin)] == DACOR(DAMA, cor))
        {
            total++;
            p = TIPO(tabu.tab[SQ(col, ilin)]);
            if(val[p] < *menor)
                *menor = val[p];
        }
        break;
    }
    // cavalo ataca casa...
    for(icol = -2; icol < 3; icol++)
        for(ilin = -2; ilin < 3; ilin++)
        {
            if(abs(icol) == abs(ilin) || icol == 0 || ilin == 0)
                continue;
            if(col + icol < 0 || col + icol > 7 || lin + ilin < 0
                    || lin + ilin > 7)
                continue;
            if(tabu.tab[SQ(col + icol, lin + ilin)] == DACOR(CAVALO, cor))
            {
                total++;
                if(val[CAVALO] < *menor)
                    *menor = val[CAVALO];
            }
        }
    // bispo ou dama atacam casa...
    for(icol = -1; icol < 2; icol += 2)
        for(ilin = -1; ilin < 2; ilin += 2)
        {
            casacol = col; //para cada diagonal, comece na casa origem.
            casalin = lin;
            do
            {
                casacol = casacol + icol;
                casalin = casalin + ilin;
                if(casacol < 0 || casacol > 7 || casalin < 0 || casalin > 7)
                    break;
            }
            while(tabu.tab[SQ(casacol, casalin)] == 0);

            if(casacol >= 0 && casacol <= 7 && casalin >= 0 && casalin <= 7)
                if(tabu.tab[SQ(casacol, casalin)] == DACOR(BISPO, cor) || tabu.tab[SQ(casacol, casalin)] == DACOR(DAMA, cor))
                {
                    total++;
                    p = TIPO(tabu.tab[SQ(casacol, casalin)]);
                    if(val[p] < *menor)
                        *menor = val[p];
                }
        } // proxima diagonal
    // ataque de rei...
    // nao preciso colocar como *menor, pois e a maior e ja comeca com ele
    for(icol = col - 1; icol <= col + 1; icol++)
        for(ilin = lin - 1; ilin <= lin + 1; ilin++)
        {
            if(icol == col && ilin == lin)
                continue;
            if(icol < 0 || icol > 7 || ilin < 0 || ilin > 7)
                continue;
            if(tabu.tab[SQ(icol, ilin)] == DACOR(REI, cor))
            {
                total++;
                break;
            }
        }
    // ataque de peao
    if(cor == BRANCO)
        // peao branco ataca para baixo (lin-1)
    {
        if(lin > 1)
        {
            ilin = lin - 1;
            if(col - 1 >= 0)
                if(tabu.tab[SQ(col - 1, ilin)] == DACOR(PEAO, BRANCO))
                {
                    if(val[PEAO] < *menor)
                        *menor = val[PEAO];
                    total++;
                }
            if(col + 1 <= 7)
                if(tabu.tab[SQ(col + 1, ilin)] == DACOR(PEAO, BRANCO))
                {
                    if(val[PEAO] < *menor)
                        *menor = val[PEAO];
                    total++;
                }
        }
    }
    else //peao preto ataca para cima (lin+1)
    {
        if(lin < 6)
        {
            ilin = lin + 1;
            if(col - 1 >= 0)
                if(tabu.tab[SQ(col - 1, ilin)] == DACOR(PEAO, PRETO))
                {
                    if(val[PEAO] < *menor)
                        *menor = val[PEAO];
                    total++;
                }
            if(col + 1 <= 7)
                if(tabu.tab[SQ(col + 1, ilin)] == DACOR(PEAO, PRETO))
                {
                    if(val[PEAO] < *menor)
                        *menor = val[PEAO];
                    total++;
                }
        }
    }
    if(total == 0)  //ninguem ataca (no attacks)
        *menor = 0;
    return total;
}

// ----------------------------------------------------------------------------
// Livro de aberturas
//pega a lista de tabuleiros e cria uma string de movimentos
void listab2string(char *strlance)
{
    no *nd;
    tabuleiro *t;
    char m[8];
    int n = 0, i;
    if(!pltab || !pltab->cabeca)
    {
        strlance[0] = '\0';
        return;
    }
    nd = pltab->cabeca->prox;
    //o primeiro tabuleiro nao tem lancex
    while(nd)
    {
        t = (tabuleiro *)nd->info;
        lance2movi(m, t->de, t->pa, t->especial);
        m[4] = '\0';
        for(i = 0; i < 4; i++)
            strlance[n + i] = m[i];
        n += 4;
        strlance[n] = ' ';
        n++;
        nd = nd->prox;
    }
    strlance[n] = '\0';
    return;
}

//le linha do livro de aberturas e preenche melhor.linha/melhor.tamanho
//com inicio no lance da vez (mnum).
void livro_linha(int mnum, char *linha)
{
    char m[8];
    int n = 0, de, pa, i, conta = 0;
    movimento mval;
    //posicao inicial
    tabuleiro tab = TAB_INICIO;
    melhor.tamanho = 0;
    while(linha[n] != '\0')
    {
        n++;
        conta++;
    }
    n = 0;
    while(n + 5 <= conta)
    {
        for(i = 0; i < 4; i++)
            m[i] = linha[n + i];
        m[4] = '\0';
        movi2lance(&de, &pa, m);
        if(!valido(tab, de, pa, &mval))
            break;
        disc = (char) joga_em(&tab, mval, 0);
        if(n / 5 >= mnum) //chegou na posicao atual! comeca inserir
            melhor.linha[melhor.tamanho++] = mval;
        n += 5;
    }
}

//retorna verdadeiro se o jogo atual casa com a linha do livro atual
int igual_strlances_strlinha(char *strlances, char *strlinha)
{
    int i = 0;
    if(strlances[0] == '\0')
        return 1;
    while(strlances[i] != '\0')
    {
        if(strlances[i] != strlinha[i])
            return 0;
        i++;
    }
    return 1;
}

//preenche melhor com uma variante do livro, avaliando candidatos
void usalivro(tabuleiro tabu)
{
    IFDEBUG("usalivro()");
    char linha[256], strlance[256], nextmove[5];
    FILE *flivro;
    char *p;
    int ncands, i, j, pool, sorteio, ll, lli, dup, de, pa;
    tabuleiro temp;
    movimento mval;
    typedef struct { char move[5]; char linha[256]; int score; } candidato;
    candidato cands[128];
    candidato tmp;

    ncands = 0;
    flivro = fopen(bookfname, "r");
    if(!flivro)
    {
        melhor.tamanho = 0;
        return;
    }

    listab2string(strlance);
    ll = strlen(strlance);

    // Phase 1: collect distinct next-move candidates
    while(1)
    {
        fgets(linha, 256, flivro);
        if((p = strchr(linha, '\n')) != NULL) *p = ' '; // newline to space
        if(feof(flivro))
            break;
        if(linha[0] == '#')
            continue;

        if(!igual_strlances_strlinha(strlance, linha))
            continue;

        lli = strlen(linha);
        if(ll + 5 > lli)
            continue;

        strncpy(nextmove, linha + ll, 4);
        nextmove[4] = '\0';

        dup = 0;
        for(i = 0; i < ncands; i++)
            if(!strcmp(cands[i].move, nextmove))
            {
                dup = 1;
                break;
            }
        if(dup)
            continue;

        strcpy(cands[ncands].move, nextmove);
        strcpy(cands[ncands].linha, linha);
        cands[ncands].score = 0;
        ncands++;
        if(ncands >= 128)
            break;
    }
    fclose(flivro);

    if(ncands == 0)
    {
        melhor.tamanho = 0;
        return;
    }

    printdbg(debug, "# livro: %d candidatos para '%s'\n", ncands, strlance);

    // Phase 2: evaluate each candidate with minimax
    for(i = 0; i < ncands; i++)
    {
        copitab(&temp, &tabu);
        movi2lance(&de, &pa, cands[i].move);
        if(valido(temp, de, pa, &mval))
        {
            joga_em(&temp, mval, 0);
            lst_recria(&plmov);
            geramov(temp, plmov, GERA_TUDO);
            cands[i].score = minimax(temp, 0, -LIMITE, LIMITE, 2);
        }
        if(debug >= 2)
            printdbg(debug, "# livro: cand[%d] = %s score=%d linha: %s\n",
                     i, cands[i].move, cands[i].score, cands[i].linha);
    }

    // Phase 3: sort by score (best for engine first)
    for(i = 0; i < ncands - 1; i++)
        for(j = i + 1; j < ncands; j++)
        {
            if((tabu.vez == BRANCO && cands[j].score > cands[i].score) ||
               (tabu.vez == PRETO  && cands[j].score < cands[i].score))
            {
                tmp = cands[i];
                cands[i] = cands[j];
                cands[j] = tmp;
            }
        }

    // Phase 4: pick random from top half
    pool = (ncands + 1) / 2;
    sorteio = irand_minmax(0, pool);

    printdbg(debug, "# livro: pool=%d, sorteado=%d, move=%s, score=%d\n",
             pool, sorteio, cands[sorteio].move, cands[sorteio].score);

    livro_linha(tabu.meionum, cands[sorteio].linha);
    melhor.valor = cands[sorteio].score;
}

//pegar caracter (linux e windows)
char pega(char *no, char *msg)
{
    int p;
    printf("%s", msg);
    p = getchar();
    while(!strchr(no, p))
    {
        printf("?");
        p = getchar();
    }
    return (char) p;
}

/* retorna x2 pertencente a (min2,max2) equivalente a x1 pertencente a (min1,max1) */
float mudaintervalo(float min1, float max1, float min2, float max2, float x1)
{
    if((min1 - max1) == 0.0) // erro! divisao por zero
        return 0.0;
    return (x1 * ((min2 - max2) / (min1 - max1)) + max2 - max1 * ((min2 - max2) / (min1 - max1)));
}

/* retorna valor inteiro aleatorio entre [min,max[ */
int irand_minmax(int min, int max)
{
    static int first = 1;
    if(first) /* inicializa semente */
    {
        srand(time(NULL));
        first = 0;
    }
    return (int)mudaintervalo(0.0, RAND_MAX, (float)min, (float)max, (float)rand());
}

//termina o programa
void sai(int error)
{
    printdbg(debug, "# xadreco : sai ( %d )\n", error);
    if(plmov)
        arena_destroi(plmov->a);  //libera arena de movimentos
    if(pltab)
        arena_destroi(pltab->a);  //libera arena de tabuleiros
    exit(error);
}

//inicializa variaveis do programa. (new game)
void inicia(tabuleiro *tabu)
{
    *tabu = TAB_INICIO; // tabuleiro completo: pecas + estado inicial
    // globais do jogo:
    pausa = 'n';
    melhor.tamanho = 0;
    melhor.valor = 0;
    lst_recria(&plmov);
    lst_recria(&pltab);
    ofereci = 1; //computador pode oferecer 1 empate
    USALIVRO = 1; // usalivro checks if file exists
    setboard = 0;
    ultimo_resultado[0] = '\0';
    primeiro = 'h'; //humano inicia, com comandos para acertar detalhes do jogo
    segundo = 'c'; //computador espera para saber se jogara de brancas ou pretas
    nivel = 3; // usado apenas no display do post (XBoard)
    ABANDONA = -2430; // volta o abandona para jogar contra humanos...
    COMPUTER = 0; // jogando contra outra engine?
    mostrapensando = 0; //post and nopost commands
    analise = 0; //analyze and exit commands
    tempomovclock = 3.0;	//em segundos
    tempomovclockmax = 120.0; // max allowed - no correpondence bot
    tbrancasac = 0.0; //tempo acumulado
    tpretasac = 0.0; //acumulated time
    tinijogo = 0;
    tinimov = 0;
    tatual = time(NULL);
    tdifs = 0.0;
    tultimoinput = time(NULL); //pausa para nao fazer muito poll seguido
    totalnodo = 0;
    totalnodonivel = 0;
    OFERECEREMPATE = 0;
}

//zera pecas do tabuleiro (para setboard preencher via FEN)
void zera_pecas(tabuleiro *tabu)
{
    memset(tabu->tab, 0, 64 * sizeof(int));
    tabu->rei_pos[0] = 0;
    tabu->rei_pos[1] = 0;
    setboard = 1;
}

//limpa algumas variaveis para iniciar ponderacao
void limpa_pensa(void)
{
    IFDEBUG("limpa_pensa()");
    melhor.tamanho = 0;
    melhor.valor = 0; // neutro; compjoga/analisa inicializa baseado na cor
    profflag = 1;
    totalnodonivel = 0;
}

//preenche a estrutura movimento usando arena e lst_insere
void enche_lmovi(lista *lmov, int de, int pa, int pp, int rr, int ee, int ff)
{
    if(!lmov)
        return;
    movimento *m = (movimento *)arena_aloca(lmov->a, sizeof(movimento));
    if(!m)
        msgsai("# Erro arena cheia em enche_lmovi", 37);
    m->de = de;
    m->pa = pa;
    m->peao_pulou = pp;
    m->roque = rr;
    m->especial = ee;
    m->flag_50 = ff;

    m->valor_estatico = 0;
    if(lst_insere(lmov, m, sizeof(movimento)))
        msgsai("# Erro arena cheia em enche_lmovi lst_insere", 40);
}

void msgsai(char *msg, int error)  //aborta programa por falta de memoria
{
    printdbg(debug, "# xadreco : %s\n", msg);
    sai(error);
}

//cuidado para nao estourar o limite da cadeia, teclando muito <t>
//be carefull to do not overflow the string limit, pressing to much <t>
//retorna um lance do jogo de teste
void testajogo(char *movinito, int mnum)
{
    //      char *jogo1="e2e4 e7e5 g1f3 b8c6 b1c3 g8f6 f1b5 a7a6 b5c6 d7c6 d2d4 e5d4 f3d4 f8c5 c1e3 d8e7 e1g1 f6e4 d1f3 c5d4 e3d4 e4d2 f3g3 d2f1 g3g7 h8f8 a1f1 c8f5 d4f6 e7c5 f6d4 c5d6 d4e5 d6b4 a2a3 b4b6 f1c1 f7f6 g7c7 b6c5 e5d6 f8f7 c1e1 c5e5 e1e5 f6e5 c7b6 f5c2 d6c7 a8c8 c7e5 f7e7 b6d4 c8d8";
    //      char *jogo1="e2e4 e7e5 f1c4 b8c6 d1h5 g8f6 h5f7";
    //mate do pastor
    //      char *jogo1="g1f3 g8f6 c2c4 b7b6 g2g3 c8b7 f1g2 d4d5 e1g1 d4c4 d1a4 c7c6 a4c4 b7a6 c4d4 a6e2 f1e1";
    //problema de travamento
    //ultimo lance:
    //36. e1c1 b2c1  ou
    //36. Tc1 Dc1+
    //proximo lance:
    //37.e2e1 c1e1++ ou
    //37. Te1 Dxe1++
    char *jogo1 =
        "g1f3 g8f6 c2c4 b7b6 g2g3 c8b7 f1g2 d7d5 e1g1 d5c4 d1a4 c7c6 a4c4 b7a6 c4d4 a6e2 f1e1";
    //travou computador
    char move[5]; // a1xb2Q#??\0
    int i;

    move[0] = '\0';
    for(i = 0; i < 4; i++)
        //int(mnum)*5; i<int(mnum)*5+4; i++)
        // 0.3 0.8 1.3 1.8
        move[i] = jogo1[mnum * 5 + i];
    strcpy(movinito, move);
    printf("%d:%s\n", (mnum + 2) / 2, move);
}

//testa posicao
void testapos(char *pieces, char *color, char *castle, char *enpassant, char *halfmove, char *fullmove)
{
    //      char *p1="
    //      char *p1="r3k2r/ppp3pp/4p3/2Bq4/3P2n1/6Q1/bK2PPPP/3R1B1R b kq - 0 18";
    //      char *p1="8/3k4/2p5/3p4/1B1P3P/3K4/4Q3/8 b - - 0 51";
    //      char *p1="8/4Q3/k1p5/2Bp4/3P3P/1K6/8/8 w - - 0 6";
    //      char *p1="rnbqk2r/p3bppp/2p1pn2/1p4B1/3P4/1QN2N2/PP2PPPP/R3KB1R b KQkq - 0 8";
    //      char *p1="rQbqk2r/p3bppp/2p1pn2/6B1/3P4/2p2N2/PP2PPPP/2KR1B1R b kq - 0 3";
    //      char *p1="2r1k3/1p2r2p/p1p5/4B3/3Q4/P1N5/1Pb2PPP/6K1 b - - 0 27";
    //      char *p1="1b3k2/Ppp4p/8/r5pR/6P1/r6P/1P3P2/5K2 w - - 0 1";
    //      char *p1="1b3k2/Ppp3Rp/8/r5p1/6P1/r4q1P/1P3P2/5K2 b - - 0 1";
    //      char *p1="7k/bp5P/1P6/8/8/8/8/7K w - - 0 1";
    //      char *p1="7k/7P/8/1p6/b4pr1/7K/8/N7 w - - 0 1";
    //      char *p1="7k/7P/8/1p6/b7/5pr1/7K/N7 w - - 0 1";
    //      char *p1="7k/7P/8/6pK/8/8/8/8 w - - 0 1";
    //erro na avaliacao! perde partida empatada
    //      char *p1="8/7k/7P/8/8/6pK/8/8 b - - 0 1";
    //    char           *p1 = "8/7k/7P/8/8/6pK/8/8 w - - 0 1";
    //fiz a arvore completa
    //      char *p1="8/8/8/8/8/k1Q5/2K5/8 b - - 0 1";
    //mate em 5, com possibilidades de afogamento e de mate em 1.
    char *p1 = "8/8/8/8/4BB2/1K6/7p/k7 w - - 0 1";
    //2 bispos... mate em 1, ou em 2, ou em 3... Se comer peca, afoga
    //      char *p1="k7/2K5/1P6/2B5/3p4/8/8/8 w - - 0 1";
    //mate de peao em 1 ply ou em 3 ply. se comer peca, afoga
    //      char *p1="4k3/P6P/RP4PR/1NPPPPN1/2BQKB2/8/8/8 w - - 0 1";
    //esta se sentindo sozinho?
    //      char *p1="r1bqkb1r/1p3pp1/p1nppn1p/6B1/3NP3/2N5/PPPQ1PPP/2KR1B1R w kq - 0 9";
    //Move o rei na abertura? erro!
    //      char *p1="r2q1rk1/1Pp2p1p/3pb1p1/p3p3/4PPn1/2P2N2/P1P1B1PP/R2QK1BR b KQ - 0 12";
    //      char *p1="r1b1kb1r/1p1n1ppp/4pn1B/1p1p4/1PPN4/2p5/P3PPPq/1R2KBR1 w kq - 0 12";
    //g2g3, sendo que podia salvar a torre com d4f3
    //    char *p1="6k1/p2rnppp/Rp1pr3/1Pp1p1PP/2P1P1K1/2RQ4/B4P2/4N3 w - - 50 80";
    //      char *p1="8/3p4/4k3/8/4P3/2K5/2P5/BB6 w - - 50 1"; //procurou o mate
    //      char *p1="r1bq1bnr/pp3ppp/8/2P1N3/R4P1k/2pB2P1/1PPPQ2P/2B1K2R b K - 0 16";

    sscanf(p1, "%s %s %s %s %s %s", pieces, color, castle, enpassant, halfmove,
           fullmove);
}

//imprime uma sequencia de lances armazenada em resultado
//tabuvez==2 para pular numeracao em debug==2
void imprime_linha(resultado *res, int mnum, int tabuvez)
{
    int i, num, vez;
    char m[80];

    num = (int)((mnum + 1.0) / 2.0);
    vez = tabuvez;
    printdbg(debug, "# ");
    for(i = 0; i < res->tamanho; i++)
    {
        lance2movi(m, res->linha[i].de, res->linha[i].pa, res->linha[i].especial);
        if(vez == BRANCO)  //jogou anterior as pretas
        {
            if(tabuvez == BRANCO && num == (int)((mnum + 1.0) / 2.0))
            {
                printf("%d. ... %s ", num, m);  //primeiro lance da variante
                if(debug == 2) fprintf(fmini, "%d. ... %s ", num, m);
            }
            else
            {
                if(tabuvez != 2)
                    printf("%s ", m);
                if(debug == 2) fprintf(fmini, "%s ", m);
            }
            num++;
        }
        else // jogou anterior as brancas
        {
            if(tabuvez == 2)  //codigo so para log, nao imprimir na tela.
            {
                if(debug == 2) fprintf(fmini, "%s ", m);
            }
            else //vez das pretas, normal. Tela e log.
            {
                printf("%d. %s ", num, m);
                if(debug == 2) fprintf(fmini, "%d. %s ", num, m);
            }
        }
        vez = ADV(vez);
    }
    printf("\n");
}

// retorna verdadeiro se existe algum caracter no buffer para ser lido
int pollinput(void)
{
    static fd_set readfds;
    struct timeval tv;
    int ret;

    FD_ZERO(&readfds);
    FD_SET(fileno(stdin), &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    select(16, &readfds, NULL, NULL, &tv);
    ret = FD_ISSET(fileno(stdin), &readfds);
    return (ret);
}

// retorna tempo decorrido desde o inicio do lance, em segundos
double difclocks(void)
{
    tatual = time(NULL);
    tdifs = difftime(tatual, tinimov);
    return tdifs;
}

char randommove(tabuleiro *tabu)
{
    IFDEBUG("randommove()");
    int moveto;
    movimento *succ;
    no *n;

    limpa_pensa();  //limpa plance para iniciar a ponderacao
    lst_recria(&plmov);
    geramov(*tabu, plmov, GERA_TUDO);  //gera os sucessores
    if(plmov->qtd == 0)
    {
        printdbg(debug, "# empty from randommove - geramov() gave 0 moves back\n");
        return 'e';
    }
    moveto = (int)(rand() % plmov->qtd);  //sorteia um lance possivel da lista de lances
    n = plmov->cabeca;
    while(n != NULL && moveto-- > 0)
        n = n->prox; //escolhe este lance como o que sera jogado

    if(n != NULL)
    {
        succ = (movimento *)n->info;
        succ->valor_estatico = 0;
        melhor.linha[0] = *succ;
        melhor.tamanho = 1;
        melhor.valor = 0;
        return '-'; //ok
    }
    printdbg(debug, "# empty from randommove - BUG\n");
    return 'e'; // really empty!
}


/* ---------------------------------------------------------------------- */
/**
 * @ingroup GroupUnique
 * @brief Prints help information and exit
 * @details Prints help information (usually called by opt -h)
 * @return Void
 * @author Ruben Carlo Benante
 * @version 20180622.184843
 * @date 2018-06-22
 *
 */
void help(void)
{
    IFDEBUG("help()");
    printf("Xadreco - %s, by Dr. Beco\n", VERSION);
    printf("\nUsage: xadreco [-h|-v] [-r] [-x] [-b path/bookfile.txt]\n");
    printf("\nOptions:\n");
    printf("\t-h,  --help\n\t\tShow this help.\n");
    printf("\t-V,  --version\n\t\tShow version and copyright information.\n");
    printf("\t-v,  --verbose\n\t\tSet verbose level (cumulative).\n");
    printf("\t-r seed,  --random seed\n\t\tXadreco plays random moves. Initializes seed. If seed=0, 'true' random.\n");
    printf("\t-x,  --xboard\n\t\tGives 'xboard' keyword immediately\n");
    printf("\t-b path/bookfile.txt,  --book\n\t\tSets the path for book file\n");
    printf("\nExit status:\n\t0 if ok.\n\t1 some error occurred.\n");
    printf("\nTodo:\n\tLong options not implemented yet.\n");
    printf("\nAuthor:\n\tWritten by %s <%s>\n\n", "Ruben Carlo Benante", "rcb@beco.cc");
    exit(EXIT_FAILURE);
}

/* ---------------------------------------------------------------------- */
/**
 * @ingroup GroupUnique
 * @brief Prints version and copyright information and exit
 * @details Prints version and copyright information (usually called by opt -V)
 * @return Void
 * @author Ruben Carlo Benante
 * @version 20180622.184843
 * @date 2018-06-22
 *
 */
void copyr(void)
{
    IFDEBUG("copyr()");
    printf("%s - Version %s, build %s\n", "Xadreco", VERSION, BUILD);
    printf("\nCopyright (C) 1994-%d %s <%s>, GNU GPL version 2 <http://gnu.org/licenses/gpl.html>. This  is  free  software: you are free to change and redistribute it. There is NO WARRANTY, to the extent permitted by law. USE IT AS IT IS. The author takes no responsability to any damage this software may inflige in your data.\n\n", 2026, "Ruben Carlo Benante", "rcb@beco.cc");
    printf("Arenas: tab=%dKB mov=%dKB. PV triangular: mel=%zuKB melhor=%zuKB\n\n", ARENA_TAB/1024, ARENA_MOV/1024, sizeof(mel)/1024, sizeof(melhor)/1024);
    if(debug > 3) printf("copyr(): Verbose: %d\n", debug); /* -vvvv */
    exit(EXIT_FAILURE);
}

/* print debug information  */
void printf2(char *fmt, ...)
{
    va_list args;
    char sdbg[256];

    if(DEBUG)
    {
        va_start(args, fmt);
        sprintf(sdbg, "# %s", fmt);
        vfprintf(XDEBUGFOUT, sdbg, args);
        va_end(args);
    }

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* print debug information  */
void printdbg(int dbg, ...)
{
    va_list args;
    char *fmt;

    va_start(args, dbg);
    fmt = va_arg(args, char *);
    /* if(dbg>DEBUG) */
    if(dbg)
        vfprintf(XDEBUGFOUT, fmt, args);
    va_end(args);
}

/* le entrada padrao */
void scanf2(char *movinito)
{
    scanf("%s", movinito);
    printdbg(debug, "# scanf: %s\n", movinito);
}

/* arena gerenciamento de memoria --------------------------------------- */

/* arena_inicia : unica chamada para malloc() */
void arena_inicia(arena *a, size_t capa)
{
    a->usado = 0;
    a->destrutor = NULL;
    a->ptr = malloc(capa);
    if(!a->ptr)
        a->total = 0;
    else
        a->total = capa;
}

/* callback para limpar pltab */
void arena_destrutor(arena *a, void (*destrutor)(arena *))
{
    a->destrutor = destrutor;
}

/* arena_destroi : unica chamada de free() */
void arena_destroi(arena *a)
{
    if(a->destrutor)
        a->destrutor(a);
    if(a->ptr)
    {
        free(a->ptr);
        a->ptr = NULL;
    }
    a->usado = 0;
    a->total = 0;
}

/* arena_aloca : reserva tam bytes da arena (simula malloc) */
char *arena_aloca(arena *a, size_t tam)
{
    char *alocado;
    if(a->usado + tam > a->total)
        return NULL; // arena cheia
    alocado = a->ptr + a->usado;
    a->usado += tam;
    return alocado;
}

/* arena_zera : libera toda area reservada na arena (simula loop free all) */
void arena_zera(arena *a)
{
    a->usado = 0; // basta apontar para o inicio do bloco
}

/* arena_libera : libera apenas um item, o ultimo item, alocado (simula 1 free) */
void arena_libera(arena *a, size_t tam)
{
    if(a->usado >= tam)
        a->usado -= tam;
}

/* lista generica usando arena --------------------------------------- */

// cria uma lista alocada em uma arena
int lst_cria(arena *a, lista **pl)
{
    lista *l;
    if(!pl)
        return -1;
    l = (lista *)arena_aloca(a, sizeof(lista));
    if(!l)
    {
        *pl = NULL;
        return -1;
    }
    l->pl = pl;
    *pl = l;
    l->cabeca = NULL;
    l->cauda = NULL;
    l->qtd = 0;
    l->a = a;
    return 0;
}

// libera reserva de uma lista alocada em uma arena
void lst_zera(lista *l)
{
    l->cabeca = NULL; // sem free, a vantagem da arena
    l->cauda = NULL; // assim, duma vez
    l->qtd = 0; // zerou numero de items
}

// insere um item ao final de uma lista (append). retorna 0 ok, -1 falha
int lst_insere(lista *l, void *i, size_t tam)
{
    no *n = (no *)arena_aloca(l->a, sizeof(no));

    if(!n)
        return -1;

    n->info = i;
    n->tam = tam;
    n->prox = NULL;
    n->ant = l->cauda;
    if(l->cauda)
        l->cauda->prox = n;
    else
        l->cabeca = n;
    l->cauda = n;
    l->qtd++;
    return 0;
}

// remove o ultimo item da lista
void lst_remove(lista *l)
{
    no *n = l->cauda;
    if(!n)
        return;
    l->cauda = n->ant;
    if(l->cauda)
        l->cauda->prox = NULL;
    else
        l->cabeca = NULL;
    l->qtd--;
    arena_libera(l->a, sizeof(no) + n->tam);
}

// limpa pointeiro externo da lista
void lst_limpa(arena *a)
{
    lista *l = (lista *)a->ptr;
    *(l->pl) = NULL; // limpa ponteiro externo
}

// conta o numero de elementos em uma lista
int lst_conta(lista *l)
{
    return l->qtd;
}

// desencaixa no e reinsere na cabeca da lista (fura a fila)
void lst_furafila(lista *l, no *n)
{
    if(n == l->cabeca)
        return;
    n->ant->prox = n->prox;
    if(n->prox)
        n->prox->ant = n->ant;
    else
        l->cauda = n->ant;
    n->prox = l->cabeca;
    n->ant = NULL;
    l->cabeca->ant = n;
    l->cabeca = n;
}

// zera arena e recria lista do inicio
void lst_recria(lista **pl)
{
    IFDEBUG("lst_recria()");
    arena *a;
    if(!*pl)
        return;
    a = (*pl)->a;
    arena_zera(a);
    if(lst_cria(a, pl))
        msgsai("# Erro arena cheia em lst_recria (impossivel)", 39);
}

// particiona: capturas e especiais primeiro
void lst_parte(lista *l)
{
    no *n, *next;
    movimento *m;
    n = l->cabeca;
    while(n)
    {
        next = n->prox;
        m = (movimento *)n->info;
        //flag_50: 0=nada, 1=Moveu peao, 2=Comeu, 3=Peao comeu. Zera empate_50.
        //especial: 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque. 9=captura
        if(m->flag_50 > 1 || m->especial)
            lst_furafila(l, n);
        n = next;
    }
}

// ordena por valor_estatico decrescente (melhor primeiro)
void lst_ordem(lista *l)
{
    int total, sorted, i;
    int val, worst_val;
    no *n, *worst, *scan;
    if(!l->cabeca || !l->cabeca->prox)
        return;
    total = l->qtd;
    for(sorted = 0; sorted < total; sorted++)
    {
        n = l->cabeca;
        for(i = 0; i < sorted; i++)
            n = n->prox; // pula os que ja foram comparados
        worst = n;
        worst_val = ((movimento *)worst->info)->valor_estatico;
        scan = n->prox;
        while(scan)
        {
            val = ((movimento *)scan->info)->valor_estatico;
            if(val < worst_val)
            {
                worst = scan;
                worst_val = val;
            }
            scan = scan->prox;
        }
        lst_furafila(l, worst);
    }
}

/* historico de tabuleiros usando arena ------------------------------- */

//insere tabuleiro na arena pltab. Retorna contagem de repeticao (>=3 empate)
int tab_insere(tabuleiro tabu)
{
    tabuleiro *t = (tabuleiro *)arena_aloca(pltab->a, sizeof(tabuleiro));
    if(!t)
    {
        msgsai("# Erro arena cheia em tab_insere", 34);
        return 0;
    }
    *t = tabu;
    if(lst_insere(pltab, t, sizeof(tabuleiro)))
        msgsai("# Erro arena cheia em tab_insere lst_insere", 40);

    int count = 1;
    no *n = pltab->cauda->ant;
    while(n)
    {
        tabuleiro *t2 = (tabuleiro *)n->info;
        if(t2->vez == t->vez
                && t2->roqueb == t->roqueb
                && t2->roquep == t->roquep
                && memcmp(t2->tab, t->tab, 64 * sizeof(int)) == 0)
        {
            count++;
            if(count >= 3)
                break;
        }
        n = n->ant;
    }
    return count;
}

/* ---------------------------------------------------------------------- */
/* vi: set ai et ts=4 sw=4 tw=0 wm=0 fo=coql  : C config for Vim modeline */
/* Template by Dr. Beco <rcb at beco dot cc>      Version 20250309.153530 */

