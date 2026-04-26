/***************************************************************************
 *   Xadreco version 8.x Chess engine                                      *
 *   with UCI Protocol (C)                                                 *
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
 * web page: http://xadreco.beco.cc/                                       *
 ***************************************************************************/
//--------------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%% Jogo de xadrez: xadreco
//%% Arquivo xadreco.c
//%% Tecnica: MiniMax com corte Alfa-Beta
//%% Criacao: 19/08/1994
//%% Atualizacao: 27/03/1997
//%% Atualizacao: 10/06/1999
//%% Atualizacao para XBoard: 26/09/2004
//%% Atualizacao versao 6.1: 2026-04-15
//%% Atualizacao edicao v7.x: 2026-04-15
//%% Atualizacao UCI v8.x: 2026-04-23
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------------

/* ---------------------------------------------------------------------- */
/* includes */
#ifdef __linux /* funcoes do SO sleep e getpid */
    #include <unistd.h>
#else
    #include <windows.h>
#endif

#include <time.h> /* hora do sistema */
#include <string.h> /* comparacoes de strings */
#include <stdio.h> /* entrada e saida padrao */
#include <stdlib.h> /* macros da biblioteca padrao */
#include <getopt.h> /* opcoes e flags vindas da funcao main argc/argv */
#include <stdarg.h> /* funcoes com lista de argumentos variaveis */

#include <pthread.h> /* programacao paralela */

/* ---------------------------------------------------------------------- */
/* definicoes de compilacao e debug */

#ifdef __linux /* dormir um pouco */
    #define waitms(ms) usleep((ms)*1000)
#else
    #define waitms(ms) Sleep((ms))
#endif

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

/* ---------------------------------------------------------------------- */
/* definicoes de jogo */

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

// Funcao geramov(..., geramodo): gera todos, gera unico (confere afogado), gera deste (0-63)
#define GERA_TODOS  -1 /* laco normal, gera lista de todos os lances validos */
#define GERA_UNICO -2 /* acha um unico lance valido, indicativo de empate por afogamento */
#define GERA_CAPTU -3 /* gera apenas lances de captura ou xeque */
// GERA_DESTE variavel geramodo 0-63: gera apenas lances da casa 'deste' (otimiza valido)

// profundidade maxima de busca (limita mel[] e melhor)
#define MAX_PROF  64
#define BIGBUFF   4096
#define SMALLBUFF 256
#define TINYBUFF  32
#define TABSIZE   64
#define SQINFO    7

// Cores: brancas=0, pretas=bit3. Pecas: brancas 1-6, pretas 9-14, vazia 0.
#define SQ(c, r)  ((c) + (r) * 8)            // dados i,j, calcula sq (0-63)
#define COL(sq)   ((sq) & 7)                 // dado sq, calcula coluna i
#define ROW(sq)   ((sq) >> 3)                // dado sq, calcula linha j
#define BRANCO 0                             // cor branca
#define PRETO  8                             // cor preta (bit 3)
#define EHBRANCA(p) ((p) != 0 && !((p) & 8)) // peca eh branca? (1-6)
#define EHPRETA(p)  ((p) & 8)                // peca eh preta? (9-14)
#define EHVAZIA(p)  ((p) == 0)               // casa vazia?
#define TIPO(p)     ((p) & 7)                // tipo da peca: PEAO=1..REI=6
#define COR(p)      ((p) & 8)                // cor da peca: 0=branca, 8=preta
#define DACOR(p, c) ((p) | (c))              // monta peca com tipo p e cor c
#define ICOR(c)     ((c) >> 3)               // indice da cor: 0=branca, 1=preta
#define CORI(k)     ((k) << 3)               // cor do indice: 0=BRANCO, 1=PRETO
#define ADV(v)      ((v) ^ 8)                // adversario: troca branco/preto

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
    int tab[TABSIZE]; //contem as pecas: brancas 1-6, pretas 9-14, vazia 0. SQ(col,lin)=col+lin*8
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

// contexto da busca (estado do aprofundamento iterativo)
typedef struct sbusca
{
    tabuleiro *tabu;     // ponteiro para o tabuleiro do jogo (nao copia)
    int nv;              // nivel atual do aprofundamento iterativo
    int val;             // valor retornado pela ultima iteracao
    int max_depth;       // profundidade maxima (sd N)
    int melhorvalor;     // melhor valor encontrado
} busca;

// entrada de dados em threads
typedef struct sentra
{
    char barbante[BIGBUFF]; // string de entrada
    int ocupada; // flag de consumo
} filenta;

// identidade das pecas (mesma para brancas e pretas)
enum piece_identity
{
    VAZIA = 0, PEAO = 1, CAVALO = 2, BISPO = 3, TORRE = 4, DAMA = 5, REI = 6, NULA = 7
};

static int val[] = {0, 100, 300, 325, 500, 900, 10000}; // centipawn lookup by TIPO index

/* ---------------------------------------------------------------------- */
/* globals */

// listas em arenas ----------------------------------
static lista *pltab = NULL; // ponteiro para lista de tabuleiros
static lista *plmov = NULL; // ponteiro para a primeira lista de movimentos de uma sequencia de niveis a ser analisadas (antiga succ_geral)
static resultado mel[MAX_PROF]; // array triangular de PV: mel[prof] = melhor linha na profundidade prof
static resultado melhor; // melhor linha salva entre iteracoes (o pote de mel)
// posicao inicial do tabuleiro (constante unica, evita duplicacao)
static const tabuleiro TAB_INICIO =
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
//log file for debug==2
static FILE *fmini; //minimax log file for debug==2
static int ulivro = 0; //0: sem livro. 1: usa livro de aberturas (opcao -b)
static int usando_livro; //1:consulta o livro de aberturas livro.txt. 0:nao consulta
static int randomchess = 0; //0: pensa para jogar. 1: joga ao acaso
static int setboard = 0; //0: posicao normal. 1: posicao FEN carregada
// busca: estado compartilhado entre minimax/profsuf/difclocks/xadreco_continua
static int busca_profflag = 1; //flag de captura ou xeque para liberar mais um nivel em profsuf
static int pula_vez = 0; //flag para evitar null-move recursivo
static int busca_totalnodo = 0; //total de nodos analisados para fazer um lance
static int busca_totalnodonivel = 0; //total de nodos analisados em um nivel da arvore
static time_t busca_tinimov; //tempo de inicio do lance
static double busca_tempo_move; //tempo por lance em segundos

static char bookfname[SMALLBUFF] = "livro.txt"; //nome do arquivo de aberturas
static int debug = 0; //0:sem debug, 1:-v debug, 2:-vv debug minimax
static filenta entra; //fila da string de entrada (thread)

/* ---------------------------------------------------------------------- */
/* prototipos gerais */ /* general prototypes */
void mostra_tabu(tabuleiro tabu);
//transforma lances int 0077 em char tipo a1h8
void lance2movi(char *m, int de, int pa, int especial);
//faz o contrario: char b1c3 em int 1022. Retorna falso se nao existe.
int movi2lance(int *de, int *pa, char *m);
//retorna o adversario: ADV(v) macro em defines
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
// le um token de uma linha ja em memoria, avanca *pos
int tokenizer(char *line, int *pos, char *token);
// monta o tabuleiro a partir dos 6 campos FEN na linha
void monta_fen(char *line, int *pos, tabuleiro *tabu);
// processa um comando do protocolo. retorna: 0=quit, 1=tratado, -1=nao e comando
int comando_proto(char *movinito, tabuleiro *tabu, int *buscando, busca *ctx);
// tenta jogar um lance do usuario (e.g., "e2e4", "e7e8q"). retorna 1=jogou, 0=invalido
int joga_movinito(char *movinito, tabuleiro *tabu);
// processa opcoes de linha de comando (getopt)
void opcoes(int argc, char *argv[]);
// handshake do protocolo: "xboard" ou "quit". Retorna 1=ok, 0=quit
int cumprimento(char *line);
// busca: trio de funcoes para aprofundamento iterativo
void xadreco_inicia(busca *ctx, tabuleiro *tabu, int max_depth, double max_time);
int  xadreco_continua(busca *ctx); // retorna 1=continuar, 0=terminou
void xadreco_para(busca *ctx);     // cleanup (sem output de lance)

// entrada de dados nao-bloqueante para Linux e Windows
void *tem_entrada(void *);

// apoio xadrez -----------------------------------------------------
//retorna 1 se "cor" ataca casa(col,lin) no tabuleiro tabu
int ataca(int cor, int col, int lin, tabuleiro tabu);
//retorna o numero de ataques da "cor" na casa(col,lin) de tabu, *menor e a menor peca que ataca.
int qataca(int cor, int col, int lin, tabuleiro tabu, int *menor);
//retorna 1 se o rei de cor "cor" esta em xeque
int xeque_rei_das(int cor, tabuleiro tabu);
//para voltar um movimento. Use duas vezes para voltar um lance inteiro.
void volta_lance(tabuleiro *tabu);
//procura nos movimentos de geramov se o lance em questao eh valido. Retorna o *movimento preenchido. Se nao, retorna NULL.
int valido(tabuleiro tabu, int de, int pa, movimento *result);
//retorna char que indica a situacao do tabuleiro, como mate, empate, etc...
char situacao(tabuleiro tabu);
// livro --------------------------------------------------------------
//preenche melhor com uma variante do livro, baseado na posicao do tabuleiro tabu
void usa_livro(tabuleiro tabu);
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
int minimax(tabuleiro atual, int prof, int alfax, int betin, int niv);
//retorna verdadeiro se (prof>=niv) ou (prof>=MAX_PROF) ou (tempo estourou)
int profsuf(tabuleiro atual, int prof, int alfax, int betin, int niv, int *valor);
//retorna um valor estatico que avalia uma posicao do tabuleiro, fixa. Cod==1: tempo estourou no meio da busca. Niv: o nivel de distancia do tabuleiro real para a copia examinada
int estatico(tabuleiro tabu, int cod, int niv, int alfax, int betin);
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

/* ---------------------------------------------------------------------- */
/* codigo principal - main code */
int main(int argc, char *argv[])
{
    // variaveis locais base do jogo
    tabuleiro tabu;
    char linha[BIGBUFF]; // bug: linha as a string line for input or as a chess moves line for a variant
    int ligado; // jogo ligado, rodando
    int buscando; // procurando lances
    busca ctx; // contexto para xadreco_inicia(), xadreco_continua() e xadreco_para()
    pthread_t penta;

    // gerenciamento de memoria com arenas
    arena atab; // historico de posicoes de tabuleiro do jogo
    arena_inicia(&atab, ARENA_TAB);
    arena_destrutor(&atab, lst_limpa); // callback para limpar pltab
    arena amov; // movimentos gerados para analise
    arena_inicia(&amov, ARENA_MOV);
    arena_destrutor(&amov, lst_limpa); // callback para limpar plmov

    // listas duplamente ligadas em arenas
    if(lst_cria(&atab, &pltab)) msgsai("# Erro arena cheia em main lst_cria atab", 39);
    if(lst_cria(&amov, &plmov)) msgsai("# Erro arena cheia em main lst_cria amov", 39);

    opcoes(argc, argv); // laco do getopt

    //------------------------------------------------------------------------------
    // inicializando o leitor paralelo de entrada
    entra.ocupada=0; // pede entrada
    pthread_create(&penta, NULL, tem_entrada, NULL);
    while(!entra.ocupada) // aguarda entrada em entra.barbante
        waitms(10);

    // handshake do protocolo
    /* fgets(linha, sizeof(linha), stdin); */
    printdbg(debug, "# GUI: %s", entra.barbante);
    if(!cumprimento(entra.barbante)) msgsai("# Ciao", 0);
    entra.ocupada=0; // comeu entrada, pede nova

    //------------------------------------------------------------------------------
    // novo jogo
    ligado = 1;
    buscando = 0;

    inicia(&tabu);
    tab_insere(tabu);
    assert(pltab != NULL && pltab->cabeca != NULL && "board history null");

    //------------------------------------------------------------------------------
    // laco de jogo (event loop)

    while(ligado)
    {
        if(entra.ocupada) // confere se tem entrada disponivel
        {
            printdbg(debug, "# GUI: %s", entra.barbante);
            ligado = comando_proto(entra.barbante, &tabu, &buscando, &ctx);
            entra.ocupada=0; // comeu entrada
        }

        // --- busca: uma iteracao do aprofundamento iterativo ---
        if(ligado && buscando)
        {
            buscando = xadreco_continua(&ctx);
            if(!buscando)
            {
                xadreco_para(&ctx);
                if(melhor.tamanho > 0)
                {
                    joga_em(&tabu, melhor.linha[0], 1);
                    lance2movi(linha, tabu.de, tabu.pa, tabu.especial); // linha ou barbante?
                    printf2("bestmove %s\n", linha);
                    if(debug >= 2) mostra_tabu(tabu);
                }
            }
        }

        // --- idle: confere entrada uma vez cada 10 milisegundos ---
        if(!buscando && !entra.ocupada)
            waitms(10);

    } //while (ligado)

    msgsai("# out of main loop...\n", 0);
    return EXIT_SUCCESS;
}

//----------------------------------------------
// processa um comando do protocolo xboard
// retorna: 0=quit, 1=tratado, -1=nao e comando
// processa opcoes de linha de comando
void opcoes(int argc, char *argv[])
{
    int opt;
    int verflag = 0;
    int seed = 0;
    FILE *f;

    srand(time(NULL) + getpid());

    /* Usage: xadreco [-h|-v] [-V|-VV] [-r seed] [-b bookfile] */
    opterr = 0;
    while((opt = getopt(argc, argv, "vhVr:b:")) != EOF)
        switch(opt)
        {
            case 'h':
                help();
                break;
            case 'V':
                verflag++;
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
            case 'b':
                strcpy(bookfname, optarg);
                ulivro = 1;
                break;
            case '?':
            default:
                printf("Type\n\t$man %s\nor\n\t$%s -h\nfor help.\n\n", argv[0], argv[0]);
                exit(EXIT_FAILURE);
        }

    if(verflag >= 2)
    {
        printf("%s\n", VERSION);
        exit(EXIT_SUCCESS);
    }
    if(verflag == 1)
        copyr();

    printdbg(debug, "# xadreco: DEBUG MACRO compiled value: %d\n", DEBUG);
    printdbg(debug, "# xadreco: verbose level set at: %d\n", debug);
    printdbg(debug, "# xadreco: random: %s. seed: -r %d\n", randomchess ? "yes" : "no", seed);
    printdbg(debug, "# xadreco: book: -b %s\n", bookfname);
    if(ulivro && (f = fopen(bookfname, "r")))
        fclose(f);
    else if(ulivro)
    {
        ulivro = 0;
        printdbg(debug, "# xadreco: book not found, ulivro=0\n");
    }

    // turn off buffers. Immediate input/output.
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
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
// geramodo:
//           -1   GERA_TODOS: todos lances validos
//           -2   GERA_UNICO: acha o primeiro e para (confere empate por afogamento)
//           -3   GERA_CAPTU: gera apenas capturas e xeques
//        0..63   GERA_DESTE: confere legalidade de um movimento
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
                                if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                    enche_lmovi(lmov, /*de*/SQ(i,j), /*pa*/SQ(col,lin), /*peao*/pp, /*roque*/rr, /*especial*/ee, /*flag50*/ff);
                                if(geramodo == GERA_UNICO) // 1 - rei anda
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
                                        ee = 1;
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(4,0), SQ(6,0), /*pp*/ -1, /*rr*/0, /*ee*/ee, /*ff*/0); //BUG flag50 zera no roque? consultar FIDE
                                        if(geramodo == GERA_UNICO) // 2 - roque pequeno branco
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
                                        ee = 2;
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(4,0), SQ(2,0), /*peao*/ -1, /*roque*/0, /*ee*/ee, /*flag50*/0);
                                        if(geramodo == GERA_UNICO) // 3 - roque grande branco
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
                                        ee = 1;
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(4,7), SQ(6,7), /*peao*/ -1, /*roque*/0, /*ee*/ee, /*flag50*/0);
                                        if(geramodo == GERA_UNICO) // 4 - roque pequeno preto
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
                                        ee = 2;
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(4,7), SQ(2,7), /*peao*/ -1, /*roque*/0, /*ee*/ee, /*flag50*/0);
                                        if(geramodo == GERA_UNICO) // 5 - roque grande preto
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
                                        ee = 3;
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(tabu.peao_pulou,5), /*peao*/ -1, /*roque*/1, /*ee*/ee, /*flag50*/3);
                                        if(geramodo == GERA_UNICO) // 6 - peao branco comeu en passant
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
                                        ee = 3;
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(tabu.peao_pulou,2), /*peao*/ -1, /*roque*/1, /*ee*/ee, /*flag50*/3);
                                        if(geramodo == GERA_UNICO) // 7 - peao preto comeu enpassant
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
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                                enche_lmovi(lmov, SQ(i,j), SQ(i,j+1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    else //nao promoveu
                                    {
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(i,j+1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    if(geramodo == GERA_UNICO) // 8 - peao branco andou uma casa
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
                                            if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                                enche_lmovi(lmov, SQ(i,j), SQ(i,j-1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    else //nao promoveu
                                    {
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(i,j-1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    if(geramodo == GERA_UNICO) // 9 - peao preto andou uma casa
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
                                    if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                        enche_lmovi(lmov, SQ(i,1), SQ(i,3), /*peao*/i, /*roque*/1, /*especial*/ee, /*flag50*/1);
                                    if(geramodo == GERA_UNICO) // 10 - peao branco andou duas casas
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
                                    if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                        enche_lmovi(lmov, SQ(i,6), SQ(i,4), /*peao*/i, /*roque*/1, /*especial*/ee, /*flag50*/1);
                                    if(geramodo == GERA_UNICO) // 11 - peao preto andou duas casas
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
                                            if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                                enche_lmovi(lmov, SQ(i,j), SQ(k,j+1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3);
                                    }
                                    else //comeu sem promover
                                    {
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0) ee = 9; // captura (ff=3)
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(k,j+1), /*peao*/ -1, /*roque*/1, /*especial*/ee, /*flag50*/3);
                                    }
                                    if(geramodo == GERA_UNICO) // 12 - peao branco comeu normalmente
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
                                            if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                                enche_lmovi(lmov, SQ(i,j), SQ(k,j-1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3);
                                    }
                                    else //comeu sem promover
                                    {
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0) ee = 9; // captura (ff=3)
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(k,j-1), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3);
                                    }
                                    if(geramodo == GERA_UNICO) // 13 - peao preto comeu normalmente
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
                                if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                    enche_lmovi(lmov, SQ(i,j), SQ(col+i,lin+j), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/ff);
                                if(geramodo == GERA_UNICO) // 14 - cavalo anda ou captura
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
                                            ff = 2; //dama ou torre capturou peca adversaria.
                                            l = 1;
                                        }
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0 && ff > 1) ee = 9; // captura
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(col,j), /*peao*/ -1, /*roque*/rr, /*especial*/ ee, /*flag50*/ff);
                                        if(geramodo == GERA_UNICO) // 15 - dama ou torre andou ou capturou na linha
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
                                            ff = 2; //dama ou torre capturou peca adversaria.
                                            m = 1;
                                        }
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0 && ff > 1) ee = 9; // captura
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(i,lin), /*peao*/ -1, /*roque*/rr, /*especial*/ ee, /*flag50*/ff);
                                        if(geramodo == GERA_UNICO) // 16 - dama ou torre andou ou capturou na coluna
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
                                            ff = 2; //dama ou bispo capturou peca adversaria.
                                            flag = 1;
                                        }
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo. 8=xeque
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        ee = xeque_rei_das(ADV(tabu.vez), tabaux) * 8; // deu xeque
                                        if(ee == 0 && ff > 1) ee = 9; // captura
                                        if(geramodo != GERA_CAPTU || (geramodo == GERA_CAPTU && ee)) // adiciona apenas lances especiais
                                            enche_lmovi(lmov, SQ(i,j), SQ(col,lin), /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/ff);
                                        if(geramodo == GERA_UNICO) // 17 - dama ou bispo andou ou capturou
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

    if(geramodo == GERA_UNICO) // 18 - se chegou ate aqui no GERA_UNICO, entao nao gerou nada
        return 0;
    if(lmov && lmov->qtd > 1)
        lst_parte(lmov); //particiona: capturas e especiais primeiro
    return (lmov ? lmov->qtd > 0 : 0);
} //fim de   ----------- int geramov(tabuleiro tabu, lista *lmov, int geramodo)

// confere se uma cor ataca uma casa
int ataca(int cor, int col, int lin, tabuleiro tabu)
{
    //retorna verdadeiro (1) ou falso (0)
    //cor==brancas   => brancas atacam casa(col,lin)
    //cor==pretas    => pretas  atacam casa(col,lin)
    int icol, ilin, casacol, casalin;
    //torre ou dama atacam a casa...
    for(icol = col - 1; icol >= 0; icol--) //desce coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(icol, lin)] == DACOR(TORRE, cor) || tabu.tab[SQ(icol, lin)] == DACOR(DAMA, cor))
            return (1);
        break;
    }
    for(icol = col + 1; icol < 8; icol++) //sobe coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(icol, lin)] == DACOR(TORRE, cor) || tabu.tab[SQ(icol, lin)] == DACOR(DAMA, cor))
            return (1);
        break;
    }
    for(ilin = lin + 1; ilin < 8; ilin++) // direita na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(tabu.tab[SQ(col, ilin)] == DACOR(TORRE, cor) || tabu.tab[SQ(col, ilin)] == DACOR(DAMA, cor))
            return (1);
        break;
    }
    for(ilin = lin - 1; ilin >= 0; ilin--) // esquerda na linha
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
            if(col + icol < 0 || col + icol > 7 || lin + ilin < 0 || lin + ilin > 7)
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
                if(tabu.tab[SQ(casacol, casalin)] == DACOR(BISPO, cor) || tabu.tab[SQ(casacol, casalin)] == DACOR(DAMA, cor))
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
    if(cor == BRANCO) // ataque de peao branco: peao branco ataca para baixo (lin-1)
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
    else //ataque de peao preto: peao preto ataca para cima (lin+1)
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

// confere se um rei de dada cor esta em xeque
int xeque_rei_das(int cor, tabuleiro tabu)
{
    int sq = tabu.rei_pos[ICOR(cor)];

    return ataca(ADV(cor), COL(sq), ROW(sq), tabu);
}

// handshake do protocolo UCI
// retorna 1=ok, 0=quit
int cumprimento(char *line)
{
    char movinito[SMALLBUFF];
    int pos = 0;

    if(!tokenizer(line, &pos, movinito))
        return 0;
    if(!strcmp(movinito, "quit"))
        return 0;
    if(!strcmp(movinito, "uci"))
    {
        printf2("id name Xadreco %s\n", VERSION);
        printf2("id author Dr. Beco\n");
        printf2("option name Book type check default false\n");
        printf2("option name BookFile type string default livro.txt\n");
        printf2("uciok\n");
        printdbg(debug, "# xadreco: handshake done\n");
        return 1;
    }
    return 0;
}

// processa um comando do protocolo UCI
// retorna: 0=quit, 1=tratado
int comando_proto(char *line, tabuleiro *tabu, int *buscando, busca *ctx)
{
    int pos;
    int wtime, btime, winc, binc, nivel, movetime, infinite, movestogo, moves_left;
    double mytime, myinc, tempo_move_max;
    char movinito[SMALLBUFF];

    pos = 0;
    if(!tokenizer(line, &pos, movinito))
        return 1;  // linha vazia

    // --- lifecycle ---

    if(!strcmp(movinito, "quit"))
        msgsai("# Thanks for playing Xadreco.", 0);
    if(!strcmp(movinito, "ucinewgame"))
    {
        *buscando = 0;
        inicia(tabu);
        tab_insere(*tabu);
        printdbg(debug, "# xadreco: board reset\n");
        return 1;
    }
    if(!strcmp(movinito, "isready"))
    {
        printf2("readyok\n");
        printdbg(debug, "# xadreco: readyok\n");
        return 1;
    }
    if(!strcmp(movinito, "stop"))
    {
        if(*buscando)
        {
            xadreco_para(ctx);
            *buscando = 0;
        }
        printdbg(debug, "# xadreco: search stopped\n");
        return 1;
    }
    if(!strcmp(movinito, "debug"))
    {
        tokenizer(line, &pos, movinito);
        debug = !strcmp(movinito, "on") ? 1 : 0;
        printdbg(debug, "# xadreco: debug %s\n", movinito);
        return 1;
    }

    // --- position ---

    if(!strcmp(movinito, "position"))
    {
        tokenizer(line, &pos, movinito);
        if(!strcmp(movinito, "startpos"))
        {
            *tabu = TAB_INICIO;
            setboard = 0;
        }
        else if(!strcmp(movinito, "fen"))
        {
            inicia(tabu);
            monta_fen(line, &pos, tabu);
        }
        if(tokenizer(line, &pos, movinito))
        {
            if(!strcmp(movinito, "moves"))
                while(tokenizer(line, &pos, movinito))
                    joga_movinito(movinito, tabu);
            else
                printdbg(debug, "# xadreco: position: unexpected '%s' (expected 'moves')\n", movinito);
        }
        tab_insere(*tabu);
        if(debug >= 2) mostra_tabu(*tabu);
        printdbg(debug, "# xadreco: position set\n");
        return 1;
    }

    // --- go ---

    if(!strcmp(movinito, "go"))
    {
        wtime = 0; btime = 0; winc = 0; binc = 0;
        nivel = 0; movetime = 0; infinite = 0; movestogo = 0;

        while(tokenizer(line, &pos, movinito))
        {
            if(!strcmp(movinito, "wtime"))     { tokenizer(line, &pos, movinito); wtime = atoi(movinito); }
            else if(!strcmp(movinito, "btime")) { tokenizer(line, &pos, movinito); btime = atoi(movinito); }
            else if(!strcmp(movinito, "winc"))  { tokenizer(line, &pos, movinito); winc = atoi(movinito); }
            else if(!strcmp(movinito, "binc"))  { tokenizer(line, &pos, movinito); binc = atoi(movinito); }
            else if(!strcmp(movinito, "depth")) { tokenizer(line, &pos, movinito); nivel = atoi(movinito); }
            else if(!strcmp(movinito, "movetime")) { tokenizer(line, &pos, movinito); movetime = atoi(movinito); }
            else if(!strcmp(movinito, "movestogo")) { tokenizer(line, &pos, movinito); movestogo = atoi(movinito); }
            else if(!strcmp(movinito, "infinite")) infinite = 1;
        }

        if(movetime > 0)
            busca_tempo_move = movetime / 1000.0;
        else if(wtime > 0 || btime > 0)
        {
            mytime = (tabu->vez == BRANCO) ? wtime / 1000.0 : btime / 1000.0;
            myinc = (tabu->vez == BRANCO) ? winc / 1000.0 : binc / 1000.0;
            moves_left = movestogo > 0 ? movestogo : TOTAL_MOVIMENTOS - tabu->meionum / 2;
            if(moves_left < 10) moves_left = 10;
            busca_tempo_move = mytime / moves_left + myinc;
            tempo_move_max = mytime / 4.0;
            if(busca_tempo_move > tempo_move_max) busca_tempo_move = tempo_move_max;
            if(busca_tempo_move < 0.5) busca_tempo_move = 0.5;
        }

        nivel = nivel > 0 ? nivel : MAX_PROF;
        if(infinite) busca_tempo_move = 999999.0;

        xadreco_inicia(ctx, tabu, nivel, busca_tempo_move);
        *buscando = 1;
        printdbg(debug, "# xadreco: go depth=%d time=%.1fs\n", nivel, busca_tempo_move);
        return 1;
    }

    // --- setoption ---

    if(!strcmp(movinito, "setoption"))
    {
        // setoption name Book value true
        // setoption name BookFile value livro.txt
        printdbg(debug, "# xadreco: setoption (stub)\n");
        return 1;
    }

    // --- unknown ---
    printdbg(debug, "# xadreco: unknown command (ignored)\n");
    return 1;
}

//----------------------------------------------
// tenta jogar um lance do usuario (e.g., "e2e4", "e7e8q")
// retorna 1 se jogou, 0 se invalido
int joga_movinito(char *movinito, tabuleiro *tabu)
{
    int de, pa;
    movimento mval;
    char sit;

    if(!movi2lance(&de, &pa, movinito))
        return 0;
    if(!valido(*tabu, de, pa, &mval))
        return 0;

    // promocao: sufixo do lance
    switch(movinito[4])
    {
        case 'q': mval.especial = 4; break;
        case 'n': mval.especial = 5; break;
        case 'r': mval.especial = 6; break;
        case 'b': mval.especial = 7; break;
        default: break;
    }

    sit = joga_em(tabu, mval, 1);

    // verifica mate/empate apos o lance (joga_em retorna situacao)
    switch(sit)
    {
        case 'M': printf2("0-1 {Black mates}\n"); break;
        case 'm': printf2("1-0 {White mates}\n"); break;
        case 'a': printf2("1/2-1/2 {Stalemate}\n"); break;
        case 'i': printf2("1/2-1/2 {Draw by insufficient mating material}\n"); break;
        case '5': printf2("1/2-1/2 {Draw by fifty moves rule}\n"); break;
        case 'r': printf2("1/2-1/2 {Draw by triple repetition}\n"); break;
        default: break;
    }

    return 1;
}

int valido(tabuleiro tabu, int de, int pa, movimento *result)
{
    IFDEBUG("valido()");

    arena aval;
    lista *llval = NULL;
    no *n;
    movimento *m;
    const int GERA_DESTE = de;

    arena_inicia(&aval, 64 * 1024);
    if(lst_cria(&aval, &llval))
        msgsai("# Erro arena cheia em valido() lst_cria", 39);
    geramov(tabu, llval, GERA_DESTE); // de (0-63) como geramodo: gera apenas desta casa

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
    int casa;
    if(tabu.empate_50 >= 50.0)  //empate apos 50 lances sem captura ou movimento de peao
        return ('5');
    for(casa = 0; casa < TABSIZE; casa++)  //insuficiencia de material
    {
        switch(TIPO(tabu.tab[casa]))
        {
            case DAMA:
            case TORRE:
            case PEAO:
                if(EHBRANCA(tabu.tab[casa]))
                    insuf_branca += 3;
                else
                    insuf_preta += 3;
                break;
            case BISPO:
                if(EHBRANCA(tabu.tab[casa]))
                    insuf_branca += 2;
                else
                    insuf_preta += 2;
                break;
            case CAVALO:
                if(EHBRANCA(tabu.tab[casa]))
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

// ------------------------------- busca: trio xadreco_inicia/continua/para --

// prepara a busca: livro, geramov, inicializa contexto
void xadreco_inicia(busca *ctx, tabuleiro *tabu, int max_depth, double max_time)
{
    int i, moveto;
    movimento *succ;
    no *n;

    melhor = (resultado){0};
    busca_tempo_move = max_time;
    busca_tinimov = time(NULL);

    ctx->tabu = tabu;
    ctx->nv = 1;
    ctx->val = 0;
    ctx->max_depth = max_depth;
    if(tabu->vez == BRANCO)
        ctx->melhorvalor = -LIMITE; // brancas quer o maximo
    else
        ctx->melhorvalor = +LIMITE; // pretas quer o minimo

    if(debug == 2)  //nivel extra de debug
    {
        fmini = fopen("minimax.log", "w");
        if(fmini == NULL)
            debug = 1;
    }

    // livro de aberturas
    if(ulivro && usando_livro && tabu->meionum < 52 && setboard != 1 && !randomchess)
    {
        usa_livro(*tabu);
        if(melhor.tamanho == 0)
            usando_livro = 0;
        ctx->melhorvalor = melhor.valor;
    }

    // se nao livro: random ou minimax
    if(melhor.tamanho == 0)
    {
        lst_recria(&plmov);
        geramov(*tabu, plmov, GERA_TODOS);
        busca_totalnodo = 0;

        // primeiro lance: joga rapido, metade do tempo, maximo 10s
        if(tabu->meionum <= 1)
        {
            busca_tempo_move /= 2.0;
            if(busca_tempo_move > 8.0) busca_tempo_move = 8.0;
            if(busca_tempo_move < 0.5) busca_tempo_move = 0.5;
        }

        // randomchess: sorteia um lance da lista
        if(randomchess)
        {
            n = plmov->cabeca;
            moveto = (int)(rand() % plmov->qtd);  //sorteia um lance possivel da lista de lances
            for(i = 0; i < moveto; ++i)
                if(n != NULL)
                    n = n->prox;
            if(n != NULL)
            {
                succ = (movimento *)n->info;
                melhor.linha[0] = *succ;
                melhor.tamanho = 1;
                succ->valor_estatico = 0;
                ctx->val = 0;
                ctx->melhorvalor = 0;
            }
        }
    }
}

// executa UMA iteracao do aprofundamento iterativo
// retorna 1 = continuar buscando, 0 = terminou
int xadreco_continua(busca *ctx)
{
    int i;
    char movinito[SMALLBUFF];

    // livro ou randomchess ja encontrou lance
    if(melhor.tamanho > 0 && ctx->nv == 1)
        return 0;

    busca_totalnodonivel = 0;
    busca_profflag = 1;
    if(debug == 2)
    {
        fprintf(fmini, "#\n#\n# *************************************************************");
        fprintf(fmini, "#\n# minimax(*tabu, prof=0, alfax=%d, betin=%d, nv=%d)", -LIMITE, LIMITE, ctx->nv);
    }
    ctx->val = minimax(*ctx->tabu, 0, -LIMITE, +LIMITE, ctx->nv);
    if(mel[0].tamanho == 0)
        return 0;
    if(difclocks() < busca_tempo_move)
    {
        // salva resultado da iteracao completa (mais profunda = mais precisa)
        memcpy(melhor.linha, mel[0].linha, mel[0].tamanho * sizeof(movimento));
        melhor.tamanho = mel[0].tamanho;
        melhor.valor = ctx->val;
        ctx->melhorvalor = ctx->val;
    }
    else
        if(melhor.tamanho > 0)
            return 0;
    busca_totalnodo += busca_totalnodonivel;
    lst_ordem(plmov);  //ordena lista de movimentos
    if(abs(ctx->val) != FIMTEMPO && abs(ctx->val) != LIMITE)
    {
        printf("info depth %d score cp %d time %d nodes %d pv ",
               ctx->nv, (ctx->tabu->vez == BRANCO) ? ctx->val : -ctx->val,
               (int)(difclocks() * 1000), busca_totalnodo);
        for(i = 0; i < mel[0].tamanho; i++)
        {
            lance2movi(movinito, mel[0].linha[i].de, mel[0].linha[i].pa, mel[0].linha[i].especial);
            printf("%s ", movinito);
        }
        printf("\n");
    }
    if((difclocks() > busca_tempo_move && debug != 2) || (debug == 2 && ctx->nv == 5))
    {
        if(mel[0].tamanho == 0)
            printdbg(debug, "# xadreco: sem lances; tempo estourado\n");
        else
            return 0;
    }
    if(abs(ctx->val) >= XEQUEMATE)
        return 0;
    if(ctx->nv >= ctx->max_depth)
        return 0;
    ctx->nv++; // busca em amplitude: aumenta um nivel.
    return 1;
}

// cleanup da busca (sem output de lance — responsabilidade do caller)
void xadreco_para(busca *ctx)
{
    melhor.valor = ctx->melhorvalor;
    if(debug == 2)
        fclose(fmini);
}

//--------------------------------------------------------------------------
//tabuleiro atual, profundidade zero, limite maximo de estatico (betin ou uso), limite minimo de estatico (alfax ou passo), nivel da busca
// Max quer mais, e tem piso alfa garantido. Nao aceita menos. Se o presente < alfa, corta.
// Min quer menos, e tem teto beta garantido. Nao aceita mais. Se o presente > beta, corta.
int minimax(tabuleiro atual, int prof, int alfax, int betin, int niv)
{
    movimento *succ;
    int novo_valor, child_val, contamov = 0;
    int valull;
    tabuleiro tab, tabull;
    char m[TINYBUFF];
    no *n;
    lista *llmov = NULL;
    size_t saved;

    assert(prof >= 0 && alfax <= betin && "Invalid minimax parameters");
    mel[prof].tamanho = 0; // inicializa PV vazio (evita dados stale de iteracao anterior)

    if(profsuf(atual, prof, alfax, betin, niv, &child_val))
        return child_val;

    // null-move pruning: passo a vez; se ainda assim excede betin/alfax, corta
    if(prof > 0 && !pula_vez && !xeque_rei_das(atual.vez, atual))
    {
        pula_vez = 1;
        copitab(&tabull, &atual);
        tabull.vez = ADV(tabull.vez);
        tabull.peao_pulou = -1;
        valull = minimax(tabull, prof + 1, alfax, betin, niv - 2);
        pula_vez = 0;
        if(prof <= 3) // DEBUG-NULL
            printdbg(debug, "# null-move: prof=%d vez=%s alfax=%d betin=%d valull=%d %s\n", // DEBUG-NULL
                     prof, atual.vez == BRANCO ? "B" : "P", alfax, betin, valull, // DEBUG-NULL
                     valull >= betin ? "CUT" : "no-cut"); // DEBUG-NULL
        if(valull >= betin)
            return betin;
    }

    if(debug == 2)
    {
        fprintf(fmini, "#\n#----------------------------------------------Minimax prof: %d", prof);
        //fprintf(fmini, "\nalfax= %d     betin= %d", alfax, betin);
    }
    if(prof == 0)
        n = plmov->cabeca; // lista de lances do tabuleiro real
    else
    {
        saved = plmov->a->usado;  //bookmark
        if(lst_cria(plmov->a, &llmov))
            msgsai("# Erro arena cheia em minimax lst_cria", 39);
        geramov(atual, llmov, GERA_TODOS); // gerar lista de lances para tabuleiro imaginario
        n = llmov->cabeca;
    }

    if(!n)
    {
        //entao o estatico refletira isso: afogamento
        mel[prof].tamanho = 0;
        child_val = estatico(atual, 0, prof, alfax, betin);
        if(debug == 2)
            fprintf(fmini, "#NULL ");
        if(prof != 0)
            plmov->a->usado = saved;  //rewind arena
        return child_val;
    }
    // minimax classico: brancas maximizam, pretas minimizam
    // alfax = lower bound (melhor para brancas), betin = upper bound (melhor para pretas)
    // valores absolutos: positivo = bom para brancas
    novo_valor = (atual.vez == BRANCO) ? -LIMITE : +LIMITE; // best inicial
    while(n)
    {
        succ = (movimento *)n->info;
        copitab(&tab, &atual);
        (void) joga_em(&tab, *succ, 1);
        //joga o lance atual, a funcao joga_em deve inserir no listab
        busca_totalnodonivel++;
        busca_profflag = succ->flag_50 + 1;	//se for zero, fim da busca.
        //flag_50:0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu;
        //flag_50== 2 ou 3 : houve captura :Liberou
        //tab.situa:0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.) 7: sem resultado
        switch(tab.situa)
        {
            case 0:  //0:nada... Quem decide eh flag_50;
                break;
            case 2:  //2:Xeque!  Liberou
                busca_profflag = 4;
                break;
            default: //situa: 1=Empate, 3,4=Mate, 5,6=Tempo. 7=sem resultado. Nao passar o nivel
                busca_profflag = 0;
        }
        if(debug == 2)
        {
            lance2movi(m, succ->de, succ->pa, succ->especial);
            fprintf(fmini, "#\n# nivel %d, %d-lance %s (%d%d%d%d):", prof, busca_totalnodonivel, m, COL(succ->de), ROW(succ->de), COL(succ->pa), ROW(succ->pa));
        }
        child_val = minimax(tab, prof + 1, alfax, betin, niv); // sem inversao de janela
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
            if(novo_valor > alfax)
                alfax = novo_valor;
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
            if(novo_valor < betin)
                betin = novo_valor;
        }
        // corte alfax-betin (comum a ambos)
        if(alfax >= betin)
        {
            if(debug == 2)
            {
                lance2movi(m, succ->de, succ->pa, succ->especial);
                fprintf(fmini, "#\n# succ: alfax>=betin (%+.2f>=%+.2f) %s Corte!", alfax / 100.0, betin / 100.0, m);
            }
            break;
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

int profsuf(tabuleiro atual, int prof, int alfax, int betin, int niv, int *valor)
{

    //limite absoluto de profundidade: protege mel[prof] de overflow
    if(prof >= MAX_PROF)
    {
        mel[prof - 1].tamanho = 0;
        *valor = estatico(atual, 0, prof, alfax, betin);
        return 1;
    }
    //se tem captura ou xeque... liberou
    //se ja passou do nivel estipulado, pare a busca incondicionalmente
    if(prof >= niv)
    {
        mel[prof].tamanho = 0;
        *valor = estatico(atual, 0, prof, alfax, betin); //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. Prof: nivel analisando?)
        return 1;
    }
    //retorna sem analisar... Deve desconsiderar o lance
    if(difclocks() >= busca_tempo_move && debug != 2)
    {
        mel[prof].tamanho = 0;
        *valor = estatico(atual, 1, prof, alfax, betin); //-FIMTEMPO;
        return 1;
    }

    if(entra.ocupada) // tem entrada? confere se eh stop
    {
        if(!strncmp(entra.barbante, "stop", 4))
        {
            mel[prof].tamanho = 0;
            *valor = estatico(atual, 1, prof, alfax, betin);
            return 1;
        }
    }

    if(!busca_profflag) //nao liberou busca_profflag==0 retorna
    {
        mel[prof].tamanho = 0;
        *valor = estatico(atual, 0, prof, alfax, betin); //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. Prof: qual nivel estao analisando?)
        return 1; //a profundidade ja eh sufuciente
    }
    return 0; //se OU Nem-Chegou-no-Nivel OU Liberou, pode ir fundo
}

char joga_em(tabuleiro *tabu, movimento movi, int cod)
{
    char res;
    int repete;

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
        repete = tab_insere(*tabu);
        if(repete >= 3)
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
int estatico(tabuleiro tabu, int cod, int niv, int alfax, int betin)
{
    if(cod) //time expired: skip evaluation, abandon this branch
        return alfax;

    int i, j, k, peca; // indices e peca atualmente analisada
    int pecab = 0, pecap = 0, material;
    int totb = 0, totp = 0; // pontuacao total das brancas e pretas
    int pena = 0, cor, r; // penalidade geral agnostica de cor, cor e rank
    int total_peao = 0, // total de peoes
        qtd_peaob, qtd_peaop, // qtd de peao na coluna analisada
        qtd_torreb, qtd_torrep, // qtd de torre na coluna analisada
        isob, isop, // indice do peao mais avancado na coluna vizinha
        j_peaob, j_peaop, // indice do peao mais avancado na coluna analisada
        invertido; // se os peoes estao de costas um para o outro
    int centrao, centrim; // coordenadas das casas do centro grande e pequeno
    int vazia; // flag verd/falso casa vazia
    int menorb = 0, menorp = 0;
    int par_bispob = 0, par_bispop = 0; // conta par de bispos
    int par_cavalob = 0, par_cavalop = 0; // conta par de cavalos
    int qtdb, qtdp;
    int peca_movida;
    int outpost, jj; // posto avancado de cavalo
    int no_rei, vizim_reib, pertim_reib, vizim_reip, pertim_reip; // ataques perto do rei, distancia de Manhattan
    int gapb, gapp, gap_aberto=0, gap_semib=0, gap_semip=0; // ilhas de peoes

    //coloca todas pecas do tabuleiro em ordem de valor
    int ordem[TABSIZE][SQINFO]; //64 casas, cada uma com 7 info: 0:casa, 1:reservado, 2:peca 3: qtdataquebranco, 4: menorb, 5: qtdataquepreto, 6: menorp

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
        case 2: //A posicao eh de XEQUE! ganha 10cp
            if(tabu.vez == BRANCO)
                totp += 10;
            else
                totb += 10;
    }

    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c situacao: totb=%d totp=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp); // DEBUG-EVAL

    //levando em conta o valor material de cada peca,

    //acha os reis.
    k = 0;
    for(k = 0; k < 2; k++)
    {
        i = COL(tabu.rei_pos[k]);
        j = ROW(tabu.rei_pos[k]);
        ordem[k][0] = i;
        ordem[k][1] = j;
        ordem[k][2] = DACOR(REI, k * PRETO);
        ordem[k][3] = qataca(BRANCO, i, j, tabu, &ordem[k][4]);
        ordem[k][5] = qataca(PRETO, i, j, tabu, &ordem[k][6]);
        if(!k) //caso REI branco, 5 pontos por ataque
            totp += ordem[k][5] * 15; // pecab+=peca; //nao soma mais o REI!
        else //pretas ganham 5 pontos por ataque ao REI
            totb += ordem[k][3] * 15; // pecap+=peca; //nao soma mais o REI!
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
                totp += ordem[k][5] * 15;
                //dama branca no ataque ganha bonus
                if(j > 4 && tabu.meionum > 30)
                    totb += 50;
                pecab += val[TIPO(peca)];
            }
            else
            {
                totb += ordem[k][3] * 15;
                if(j < 3 && tabu.meionum > 30)
                    totp += 50;
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
                totp += ordem[k][5] * 15;
                //torre branca no ataque ganha bonus
                if(j > 4)
                    totb += 50;
                pecab += val[TIPO(peca)];
            }
            else
            {
                totb += ordem[k][3] * 15;
                if(j < 3)
                    totp += 50;
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
                par_bispob++;
            }
            //pretas ganham pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 10;
                pecap += val[TIPO(peca)];
                par_bispop++;
            }
            k++;
        }

    //acha cavalos
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
                par_cavalob++;
            }
            //pretas ganham 5 pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 10;
                pecap += val[TIPO(peca)];
                par_cavalop++;
            }
            k++;
        }

    //acha peoes
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
                totp += ordem[k][5] * 5;
                pecab += val[TIPO(peca)];
            }
            //pretas ganham 5 pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 5;
                pecap += val[TIPO(peca)];
            }
            k++;
        }

    if(k < 64)
        ordem[k][0] = -1; //sinaliza o fim
    //as pecas agora estao em ordem de valor --------------

    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c pecas: totb=%d totp=%d pecab=%d pecap=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp, pecab, pecap); // DEBUG-EVAL

    //--------------------------- lazy evaluation START ---------------------------

    //hanging piece: se a peca que acabou de mover esta atacada, penaliza
    peca_movida = TIPO(tabu.tab[tabu.pa]);
    assert(COR(tabu.tab[tabu.pa]) == ADV(tabu.vez) && "peca no destino deve ser de quem moveu");
    if(peca_movida != REI && peca_movida != 0 && ataca(tabu.vez, COL(tabu.pa), ROW(tabu.pa), tabu))
    {
        // peca pendurada: quem moveu perde valor. Absoluto: positivo = bom para brancas.
        if(EHBRANCA(tabu.tab[tabu.pa]))
            pecab -= val[peca_movida]/7; // peca branca pendurada, ruim para brancas
        else
            pecap -= val[peca_movida]/7; // peca preta pendurada, bom para brancas
    }

    //-------------------------------------------------------------------------------------------------------
    //ponto de vista branco (branco eh positivo, preto eh negativo). Absoluto: positivo = bom para brancas.
    material = (int)((1.0 + 75.0 / (float)(pecab + pecap)) * (float)(pecab - pecap));
    //-------------------------------------------------------------------------------------------------------

    //lazy: material even with best positional bonus can't beat alfax
    if(material + MARGEM_PREGUICA <= alfax)
    {
        if(debug == 2)
            fprintf(fmini, "lazy: material+margin <= alfax (%+.2f <= %+.2f)\n", (material + MARGEM_PREGUICA) / 100.0, alfax / 100.0);
        return (material + MARGEM_PREGUICA);
    }
    //lazy: material even with worst positional penalty still beats betin
    if(material - MARGEM_PREGUICA >= betin)
    {
        if(debug == 2)
            fprintf(fmini, "lazy: material-margin >= betin (%+.2f >= %+.2f)\n", (material - MARGEM_PREGUICA) / 100.0, betin / 100.0);
        return (material - MARGEM_PREGUICA);
    }
    //--------------------------- lazy evaluation END ---------------------------
    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c material+hanging+lazy: material=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', material); // DEBUG-EVAL

    // conta mobilidade --------------------------------------------
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            centrao = (i > 1 && i < 6 && j > 1 && j < 6);
            centrim = (i == 3 || i == 4) && (j == 3 || j == 4);
            vazia = tabu.tab[SQ(i, j)] == VAZIA;
            vizim_reib = abs(i - COL(tabu.rei_pos[0])) + abs(j - ROW(tabu.rei_pos[0])) <= 1; // ataque casa vizinha ou direto ao rei inimigo
            pertim_reib = abs(i - COL(tabu.rei_pos[0])) + abs(j - ROW(tabu.rei_pos[0])) <= 2; // ataque perto do rei inimigo
            vizim_reip = abs(i - COL(tabu.rei_pos[1])) + abs(j - ROW(tabu.rei_pos[1])) <= 1; // ataque casa vizinha ou direto ao rei inimigo
            pertim_reip = abs(i - COL(tabu.rei_pos[1])) + abs(j - ROW(tabu.rei_pos[1])) <= 2; // ataque perto do rei inimigo
            no_rei = vizim_reib || pertim_reib || vizim_reip || pertim_reip;
            if(!vazia && !centrao && !no_rei)
                continue;

            if(ataca(BRANCO, i, j, tabu))
            {
                if(vazia) totb += centrao? 3 : 2;
                if(centrao) totb += centrim ? 2 : 1;
                if(pertim_reip) totb += 2; // ataque perto do rei inimigo
                if(vizim_reip) totb += 4; // ataque casa vizinha ou direto ao rei inimigo
            }
            if(ataca(PRETO, i, j, tabu))
            {
                if(vazia) totp += centrao? 3 : 2;
                if(centrao) totp += centrim ? 2 : 1;
                if(pertim_reib) totp += 2; // ataque perto do rei inimigo
                if(vizim_reib) totp += 4; // ataque casa vizinha ou direto ao rei inimigo
            }
        }

    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c mobilidade+centro+rei: totb=%d totp=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp); // DEBUG-EVAL

    //usar o 'material' e a quantidade de ataques nelas
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
                totp += 30;
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
                totb += 30;
        }
    }

    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c ataques/defesas: totb=%d totp=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp); // DEBUG-EVAL

    //falta explicar como contar defesas com pecas cravadas
    //no final, cercar o rei
    //nao esta vendo 2 defesas por exemplo com P....RQ na mesma linha. O peao tem
    //defesa da torre e da dama.

    //incluindo peca cravada
    for(k = 2; k < 64; k++) //tirando os dois reis da jogada k=2
    {
        if(ordem[k][0] == -1) //todas pecas podem ser cravadas, exceto Rei
            break;
        i = ordem[k][0]; //as pecas estao em ordem de valor
        j = ordem[k][1];
        peca = ordem[k][2];
        tabu.tab[SQ(i, j)] = VAZIA; //imagina se essa peca nao existisse?
        if(EHBRANCA(peca))
        {
            qtdp = qataca(PRETO, i, j, tabu, &menorp);
            if(qtdp > ordem[k][5] && menorp < val[TIPO(peca)])
                totb -= (val[TIPO(peca)] / 7); //perde uma fracao do valor da peca cravada;
        }
        else
        {
            qtdb = qataca(BRANCO, i, j, tabu, &menorb);
            if(qtdb > ordem[k][3] && menorb < val[TIPO(peca)])
                totp -= (val[TIPO(peca)] / 7); //perde uma fracao do valor da peca cravada;
        }
        tabu.tab[SQ(i, j)] = peca; //recoloca a peca no lugar.
    }
    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c peca cravada: totb=%d totp=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp); // DEBUG-EVAL
    //avaliando os peoes avancados
    k = 0;
    while(k < 64 && ordem[k][0] != -1 && TIPO(ordem[k][2]) != PEAO)
        k++;
    for(; k < 64 && ordem[k][0] != -1 ; k++)
    {
        i = ordem[k][0];
        j = ordem[k][1];
        if(ordem[k][2] == DACOR(PEAO, BRANCO))  //Peao branco
        {
            if(j > 2) totb += 5; //faltando 4 casas ou menos para promover ganha +1
            if(j > 3) totb += 15; //faltando 3 casas ou menos para promover ganha +1+1
            if(j > 4) totb += 25; //faltando 2 casas ou menos para promover ganha +1+1+3
            if(j > 5) totb += 35; //faltando 1 casas ou menos para promover ganha +1+1+3+3
        }
        else //Peao preto
        {
            if(j < 5) totp += 5; //faltando 4 casas ou menos para promover ganha +1
            if(j < 4) totp += 15; //faltando 3 casas ou menos para promover ganha +1+1
            if(j < 3) totp += 25; //faltando 2 casas ou menos para promover ganha +1+1+3
            if(j < 2) totp += 35; //faltando 1 casas ou menos para promover ganha +1+1+3+3
        }
    }

    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c peoes avancados: totb=%d totp=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp); // DEBUG-EVAL
    // explicado que o peao passado vale muito, e passado protegido de peao vale mais! (feito 2026-04-25)
    // Procura Peao Dobrado ou Isolado
    // Procura torres (dobradas ou nao) em colunas abertas ou semi-abertas
    // Peao atrasado: nao pode ser defendido por peoes amigos
    // Maquina de Estados de isob
    for(i = 0; i < 8; i++)
    {
        qtd_peaob = 0; // qtd de peao branco na coluna i
        qtd_peaop = 0; // qtd de peao preto na coluna i
        j_peaob = 0; // indice do peao branco mais avancado
        j_peaop = 0; // indice do peao preto mais avancado
        qtd_torreb = 0; // qtd de torre branca na coluna i
        qtd_torrep = 0; // qtd de torre preta na coluna i
        isob = 0; // peao branco vizinho mais atrasado
        isop = 0; // peao preto vizinho mais atrasado
        invertido = 0; // tem peao de cores opostas invertidas na mesma coluna
        // procurando peoes dobrados ou triplicados: varre a mesma coluna
        // e peoes isolados ou passados: varre ambas colunas adjacentes procurando parceiro
        for(j = 0; j < 8; j++) //FIXA o peao escolhido. Inicia em zero ate sete para contar tambem torres
        {
            if(tabu.tab[SQ(i, j)] == DACOR(PEAO, BRANCO))
            {
                qtd_peaob++; // mais um na mesma coluna
                total_peao++; // total de peoes brancos
                if(qtd_peaop)
                    invertido = 1; // mas ja ficou para tras
                j_peaob = j; // salva o indice, sobrescreve para pegar o mais alto
            }
            if(tabu.tab[SQ(i, j)] == DACOR(PEAO, PRETO))
            {
                qtd_peaop++; // mais um na mesma coluna
                total_peao++; // total de peoes pretos
                invertido = 0; // preto linha de cima nao eh invertido
                if(!j_peaop) j_peaop = j; // salva o indice, evita sobrescrever para pegar o mais baixo
            }
            if(tabu.tab[SQ(i, j)] == DACOR(TORRE, BRANCO)) qtd_torreb++; // achou mais uma torre branca
            if(tabu.tab[SQ(i, j)] == DACOR(TORRE, PRETO)) qtd_torrep++; // achou mais uma torre preta
            if(!isob && i>0 && tabu.tab[SQ(i-1, j)] == DACOR(PEAO, BRANCO)) isob=j; // menor peao branco vizinho anterior
            if(!isob && i<7 && tabu.tab[SQ(i+1, j)] == DACOR(PEAO, BRANCO)) isob=j; // menor peao branco vizinho posterior
            if(i>0 && tabu.tab[SQ(i-1, j)] == DACOR(PEAO, PRETO)) isop=j; // maior peao preto vizinho anterior
            if(i<7 && tabu.tab[SQ(i+1, j)] == DACOR(PEAO, PRETO)) isop=j; // maior peao preto vizinho posterior
        }

        // torres dobradas e em colunas abertas e semi-abertas
        if(qtd_torreb >= 2) totb += 40;  // torres dobradas brancas na mesma coluna
        if(qtd_torrep >= 2) totp += 40;  // torres dobradas pretas na mesma coluna

        // torres e ilhas de peoes -------------------------------------
        if(!qtd_peaob && !qtd_peaop) // gap ambos - coluna totalmente aberta
        {
            gap_aberto++;
            if(qtd_torreb)
                totb += 30;  // torre branca em coluna aberta
            if(qtd_torrep)
                totp += 30;  // torre preta em coluna aberta
        }
        if(!qtd_peaob && qtd_peaop) // gap branco - coluna semi-aberta com peao preto
        {
            if(qtd_torreb)
                totb += 15;  // torre branca em coluna semi-aberta
            gap_semib++;
        }
        if(!qtd_peaop && qtd_peaob) // gap preto - coluna semi-aberta com peao branco
        {
            if(qtd_torrep)
                totp += 15;  // torre preta em coluna semi-aberta
            gap_semip++;
        }

        if(qtd_peaob) //se achou branco nesta coluna i
        {
            totb -= ((qtd_peaob - 1) * 30); // penalidade peao dobrado: um so=0, para dobrado=-30, trip=-60!
            if(!isob) totb -= 40; //penalidade para peao isolado //e ele nao tem vizinhos amigos, PEAO ISOLADO
            // PEAO PASSADO --------------------------------------------------------------------------------------
            if((!isop || j_peaob >= isop) && (!qtd_peaop || invertido)) //sem inimigos vizinhos ou na mesma coluna
            {
                totb += 160; // bonus de peao passado
                if(isob) totb += 20; //bonus peao passado protegido
                if(qtd_torreb) totb += 50; //torre amiga na coluna do peao passado
                if(qtd_torrep) totb -= 50; //torre inimiga na coluna do peao passado
            }
            // PEAO ATRASADO ---------------------------------
            if(isob > j_peaob && ataca(PRETO, i, j_peaob+1, tabu))
                totb -= 60; // penalidade peao atrasado
        }
        if(qtd_peaop) //se achou preto nesta coluna i
        {
            totp -= ((qtd_peaop - 1) * 30); // penalidade peao dobrado: um so=0, para dobrado=-30, trip=-60!
            if(!isop) totp -= 40; //ele nao tem vizinhos amigos, PEAO ISOLADO, penalidade
            // PEAO PASSADO --------------------------------------------------------------------------------------
            if((!isob || j_peaop <= isob) && (!qtd_peaob || invertido)) //sem inimigos vizinhos ou na mesma coluna
            {
                totp += 160; // bonus de peao passado
                if(isop) totp += 20; //bonus peao passado protegido
                if(qtd_torrep) totp += 50; //torre amiga na coluna do peao passado
                if(qtd_torreb) totp -= 50; //torre inimiga na coluna do peao passado
            }
            // PEAO ATRASADO ---------------------------------
            if(isop < j_peaop && ataca(BRANCO, i, j_peaop-1, tabu))
                totp -= 60; // penalidade peao atrasado
        }
    } // maquina de estados de peoes

    gapb = gap_aberto + gap_semib;  // white's structural gaps
    gapp = gap_aberto + gap_semip;  // black's structural gaps
    totb -= (gapb - gapp) * 10;  // penalize side with more gaps
    totp -= (gapp - gapb) * 10;

    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c peoes estrutura+torres+gaps: totb=%d totp=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp); // DEBUG-EVAL

    // par de bispos mais forte que cavalos em posicoes abertas
    if(par_bispob>1)
        totb += 20 / (total_peao/5 + 1); // de max 4 a 1
    if(par_bispop>1)
        totp += 20 / (total_peao/5 + 1); // de max 4 a 1
    totb += (par_bispob - par_bispop) * 5;
    totp += (par_bispop - par_bispob) * 5;
    if(par_cavalob>1)
        totb -= 20 / (total_peao/5 + 1); // de max 4 a 1
    if(par_cavalop>1)
        totp -= 20 / (total_peao/5 + 1); // de max 4 a 1

    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c par bispos/cavalos: totb=%d totp=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp); // DEBUG-EVAL

    // cavalos em posto avancado
    for(k = 0; k < 64 && ordem[k][0] != -1; k++)
    {
        if(TIPO(ordem[k][2]) != CAVALO)
            continue;
        i = ordem[k][0];
        j = ordem[k][1];
        if(EHBRANCA(ordem[k][2]))
        {
            // defendido por peao e sem peoes inimigos para atacar
            if(ordem[k][3] > 0 && ordem[k][4] == val[PEAO])  // peao branco defende
            {
                outpost = 1;
                for(jj = j; jj < 7 && outpost; jj++)
                {
                    if(i > 0 && tabu.tab[SQ(i-1, jj)] == DACOR(PEAO, PRETO)) outpost = 0;
                    if(i < 7 && tabu.tab[SQ(i+1, jj)] == DACOR(PEAO, PRETO)) outpost = 0;
                }
                if(outpost)
                    totb += 60;
            }
        }
        else
        {
            // defendido por peao e sem peoes inimigos para atacar
            if(ordem[k][5] > 0 && ordem[k][6] == val[PEAO])
            {
                outpost = 1;
                for(jj = j; jj > 0 && outpost; jj--)
                {
                    if(i > 0 && tabu.tab[SQ(i-1, jj)] == DACOR(PEAO, BRANCO)) outpost = 0;
                    if(i < 7 && tabu.tab[SQ(i+1, jj)] == DACOR(PEAO, BRANCO)) outpost = 0;
                }
                if(outpost)
                    totp += 60;
            }
        }
    }


    if(niv <= 1) printdbg(debug, "# EVAL[%d] %c%c%c%c outpost: totb=%d totp=%d\n", niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', totb, totp); // DEBUG-EVAL

    //bonificacao para quem nao mexeu a dama na abertura
    //TODO usar flag para lembrar se ja mexeu, senao vai e volta
    if(tabu.meionum < 32 && setboard != 1)
    {
        if(tabu.tab[SQ(3, 0)] == DACOR(DAMA, BRANCO))
            totb += 40;
        if(tabu.tab[SQ(3, 7)] == DACOR(DAMA, PRETO))
            totp += 40;
    }
    //bonificacao para quem fez roque na abertura
    //TODO usar flag para saber se fez roque mesmo
    if(tabu.meionum < 32 && setboard != 1)
    {
        if(tabu.tab[SQ(6, 0)] == DACOR(REI, BRANCO) && tabu.tab[SQ(5, 0)] == DACOR(TORRE, BRANCO)) //brancas com roque pequeno
            totb += 60;
        if(tabu.tab[SQ(2, 0)] == DACOR(REI, BRANCO) && tabu.tab[SQ(3, 0)] == DACOR(TORRE, BRANCO)) //brancas com roque grande
            totb += 40;
        if(tabu.tab[SQ(6, 7)] == DACOR(REI, PRETO) && tabu.tab[SQ(5, 7)] == DACOR(TORRE, PRETO)) //pretas com roque pequeno
            totp += 60;
        if(tabu.tab[SQ(2, 7)] == DACOR(REI, PRETO) && tabu.tab[SQ(3, 7)] == DACOR(TORRE, PRETO)) //pretas com roque grande
            totp += 40;
    }

    //bonificacao para rei protegido na abertura com os peoes do Escudo Real
    if(tabu.meionum < 50 && setboard != 1)
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

    // bonus rei ataca no fim de jogo
    if(tabu.meionum > 60 && setboard != 1)
    {
        for(k = 0; k<2; k++)
        {
            i = COL(tabu.rei_pos[k]);
            j = ROW(tabu.rei_pos[k]);
            centrao = (i > 1 && i < 6 && j > 1 && j < 6);
            if(centrao)
            {
                if(!k)
                    totb += 30;
                else
                    totp += 30;
            }
        }
    }

    // desenvolver as pecas menores na abertura
    if(tabu.meionum < 32 && setboard != 1)
    {
        // knights on starting squares
        if(tabu.tab[SQ(1, 0)] == DACOR(CAVALO, BRANCO)) totb -= 15;
        if(tabu.tab[SQ(6, 0)] == DACOR(CAVALO, BRANCO)) totb -= 15;
        if(tabu.tab[SQ(1, 7)] == DACOR(CAVALO, PRETO))  totp -= 15;
        if(tabu.tab[SQ(6, 7)] == DACOR(CAVALO, PRETO))  totp -= 15;
        // bishops on starting squares
        if(tabu.tab[SQ(2, 0)] == DACOR(BISPO, BRANCO))  totb -= 10;
        if(tabu.tab[SQ(5, 0)] == DACOR(BISPO, BRANCO))  totb -= 10;
        if(tabu.tab[SQ(2, 7)] == DACOR(BISPO, PRETO))   totp -= 10;
        if(tabu.tab[SQ(5, 7)] == DACOR(BISPO, PRETO))   totp -= 10;
    }

    // escudo real: penalidade se mexer o peao do bispo, cavalo ou da torre no comeco da abertura!
    if(tabu.meionum < 16 && setboard != 1)
    {
        for(k = 0; k<2; k++)
        {
            pena = 0;
            i = COL(tabu.rei_pos[k]);
            j = ROW(tabu.rei_pos[k]);
            cor = CORI(k);
            r = (k * 5) + 1;
            if(i > 3 && j == k*7) // REI na ala do rei
            {
                if(tabu.tab[SQ(7, r)] != DACOR(PEAO, cor)) pena -= 20; //PTR
                if(tabu.tab[SQ(6, r)] != DACOR(PEAO, cor)) pena -= 30; //PCR
                if(tabu.tab[SQ(5, r)] != DACOR(PEAO, cor)) pena -= 30; //PBR
            }
            if(i < 5 && j == k*7) // REI na ala da dama
            {
                if(tabu.tab[SQ(0, r)] != DACOR(PEAO, cor)) pena -= 20; //PTD
                if(tabu.tab[SQ(1, r)] != DACOR(PEAO, cor)) pena -= 30; //PCD
                if(tabu.tab[SQ(2, r)] != DACOR(PEAO, cor)) pena -= 30; //PBD
            }
            if(!k)
                totb += pena; //caso branco
            else
                totp += pena; //caso preto
        }
    }

    //prepara o retorno: absoluto, positivo = bom para brancas
    if(niv <= 1) // DEBUG-EVAL
        printdbg(debug, "# EVAL[%d] TOTAL %c%c%c%c: material=%d totb=%d totp=%d return=%d\n", // DEBUG-EVAL
                 niv, COL(tabu.de)+'a', ROW(tabu.de)+'1', COL(tabu.pa)+'a', ROW(tabu.pa)+'1', // DEBUG-EVAL
                 material, totb, totp, material + totb - totp); // DEBUG-EVAL
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
        usando_livro = 1; //nao usar abertura em posicoes de setboard
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
    char m[TINYBUFF];
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
    char m[TINYBUFF];
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
        (void) joga_em(&tab, mval, 0);
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
void usa_livro(tabuleiro tabu)
{
    IFDEBUG("usa_livro()");
    char linha[BIGBUFF], strlance[BIGBUFF], nextmove[TINYBUFF];
    FILE *flivro;
    char *p;
    int ncands, i, j, pool, sorteio, ll, lli, dup, de, pa;
    tabuleiro temp;
    movimento mval;
    typedef struct { char move[TINYBUFF]; char linha[BIGBUFF]; int score; } candidato;
    candidato cands[SMALLBUFF];
    candidato tmp;

    ncands = 0;
    flivro = fopen(bookfname, "r");
    if(!flivro)
    {
        usando_livro = 0;
        melhor.tamanho = 0;
        return;
    }

    listab2string(strlance);
    ll = strlen(strlance);

    // Phase 1: collect distinct next-move candidates
    while(1)
    {
        fgets(linha, BIGBUFF, flivro);
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

    printdbg(debug, "# xadreco livro: %d candidatos para '%s'\n", ncands, strlance);

    // Phase 2: evaluate each candidate with minimax
    for(i = 0; i < ncands; i++)
    {
        copitab(&temp, &tabu);
        movi2lance(&de, &pa, cands[i].move);
        if(valido(temp, de, pa, &mval))
        {
            joga_em(&temp, mval, 0);
            lst_recria(&plmov);
            geramov(temp, plmov, GERA_TODOS);
            cands[i].score = minimax(temp, 0, -LIMITE, LIMITE, 2);
        }
        if(debug >= 2)
            printdbg(debug, "# xadreco livro: cand[%d] = %s score=%d linha: %s\n",
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

    printdbg(debug, "# xadreco livro: pool=%d, sorteado=%d, move=%s, score=%d\n",
             pool, sorteio, cands[sorteio].move, cands[sorteio].score);

    livro_linha(tabu.meionum, cands[sorteio].linha);
    melhor.valor = cands[sorteio].score;
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
    printdbg(debug, "# xadreco: sai ( %d )\n", error);
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
    melhor = (resultado){0};
    lst_recria(&plmov);
    lst_recria(&pltab);
    usando_livro = 1;
    setboard = 0;
    busca_tempo_move = 3.0;
    busca_tinimov = 0;
    busca_totalnodo = 0;
    busca_totalnodonivel = 0;
}

//zera pecas do tabuleiro (para setboard preencher via FEN)
void zera_pecas(tabuleiro *tabu)
{
    memset(tabu->tab, 0, 64 * sizeof(int));
    tabu->rei_pos[0] = 0;
    tabu->rei_pos[1] = 0;
    setboard = 1;
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
    printdbg(debug, "# xadreco: %s\n", msg);
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
    char move[TINYBUFF];
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

// entrada de dados nao-bloqueante para Linux e Windows
void *tem_entrada(void *n __attribute__((unused)))
{
    while(1)
    {
        while(entra.ocupada) // aguarda liberar a entrada
            waitms(10);
        fgets(entra.barbante, BIGBUFF, stdin);
        entra.ocupada = 1;
    }

    return NULL;
}

// retorna tempo decorrido desde o inicio do lance, em segundos
double difclocks(void)
{
    time_t tmp = time(NULL);
    return difftime(tmp, busca_tinimov);
}

char randommove(tabuleiro *tabu)
{
    IFDEBUG("randommove()");
    int moveto;
    movimento *succ;
    no *n;

    melhor = (resultado){0};
    lst_recria(&plmov);
    geramov(*tabu, plmov, GERA_TODOS);  //gera os sucessores
    if(plmov->qtd == 0)
    {
        printdbg(debug, "# xadreco: empty randommove - geramov() gave 0 moves back\n");
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
    printdbg(debug, "# xadreco: empty randommove - BUG\n");
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
    printf("\nUsage: xadreco [-h|-v] [-r seed] [-b path/bookfile.txt]\n");
    printf("\nOptions:\n");
    printf("\t-h,  --help\n\t\tShow this help.\n");
    printf("\t-V,  --version\n\t\tShow version and copyright information.\n");
    printf("\t-VV\n\t\tShow only version number.\n");
    printf("\t-v,  --verbose\n\t\tSet verbose level (cumulative).\n");
    printf("\t-r seed,  --random seed\n\t\tXadreco plays random moves. Initializes seed. If seed=0, 'true' random.\n");
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
    exit(EXIT_SUCCESS);
}

/* print debug information  */
void printf2(char *fmt, ...)
{
    va_list args;
    char sdbg[BIGBUFF];

    if(DEBUG)
    {
        va_start(args, fmt);
        sprintf(sdbg, "# %s", fmt);
        vfprintf(stderr, sdbg, args);
        va_end(args);
    }

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
    fflush(stderr);
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
        vfprintf(stderr, fmt, args);
    va_end(args);
}


// le um token de uma linha ja em memoria, avanca *pos
int tokenizer(char *line, int *pos, char *token)
{
    int n;
    if(sscanf(line + *pos, "%s%n", token, &n) != 1)
        return 0;
    *pos += n;
    return 1;
}

// monta o tabuleiro a partir dos 6 campos FEN na linha
void monta_fen(char *line, int *pos, tabuleiro *tabu)
{
    char pieces[SMALLBUFF], color[TINYBUFF], castle[TINYBUFF], enpassant[TINYBUFF], halfmove[TINYBUFF], fullmove[TINYBUFF];
    int i, j, k;

    if(!tokenizer(line, pos, pieces))    return;
    if(!tokenizer(line, pos, color))     return;
    if(!tokenizer(line, pos, castle))    return;
    if(!tokenizer(line, pos, enpassant)) return;
    if(!tokenizer(line, pos, halfmove))  return;
    if(!tokenizer(line, pos, fullmove))  return;

    // campo 1: pecas
    zera_pecas(tabu);
    j = 7;
    i = 0;
    k = 0;
    while(pieces[k] != '\0')
    {
        if(i < 0 || i > 8)
            msgsai("# Posicao FEN incorreta.", 24);
        switch(pieces[k])
        {
            case 'K': tabu->tab[SQ(i, j)] = DACOR(REI, BRANCO); tabu->rei_pos[0] = SQ(i, j); i++; break;
            case 'k': tabu->tab[SQ(i, j)] = DACOR(REI, PRETO);  tabu->rei_pos[1] = SQ(i, j); i++; break;
            case 'Q': tabu->tab[SQ(i, j)] = DACOR(DAMA, BRANCO);   i++; break;
            case 'q': tabu->tab[SQ(i, j)] = DACOR(DAMA, PRETO);    i++; break;
            case 'R': tabu->tab[SQ(i, j)] = DACOR(TORRE, BRANCO);  i++; break;
            case 'r': tabu->tab[SQ(i, j)] = DACOR(TORRE, PRETO);   i++; break;
            case 'B': tabu->tab[SQ(i, j)] = DACOR(BISPO, BRANCO);  i++; break;
            case 'b': tabu->tab[SQ(i, j)] = DACOR(BISPO, PRETO);   i++; break;
            case 'N': tabu->tab[SQ(i, j)] = DACOR(CAVALO, BRANCO); i++; break;
            case 'n': tabu->tab[SQ(i, j)] = DACOR(CAVALO, PRETO);  i++; break;
            case 'P': tabu->tab[SQ(i, j)] = DACOR(PEAO, BRANCO);   i++; break;
            case 'p': tabu->tab[SQ(i, j)] = DACOR(PEAO, PRETO);    i++; break;
            case '/': i = 0; j--; break;
            default:  i += (pieces[k] - '0'); break;
        }
        k++;
    }

    // campo 2: cor da vez
    tabu->vez = (color[0] == 'w') ? BRANCO : PRETO;

    // campo 3: roque
    tabu->roqueb = 0;
    tabu->roquep = 0;
    if(!strchr(castle, '-'))
    {
        if(strchr(castle, 'K')) tabu->roqueb = 3;
        if(strchr(castle, 'Q'))
            tabu->roqueb = (tabu->roqueb == 3) ? 1 : 2;
        if(strchr(castle, 'k')) tabu->roquep = 3;
        if(strchr(castle, 'q'))
            tabu->roquep = (tabu->roquep == 3) ? 1 : 2;
    }

    // campo 4: en passant
    tabu->peao_pulou = -1;
    if(!strchr(enpassant, '-'))
        tabu->peao_pulou = enpassant[0] - 'a';

    // campo 5: halfmove clock (empate 50 lances)
    tabu->empate_50 = atoi(halfmove);

    // campo 6: fullmove number → meionum
    tabu->meionum = (atoi(fullmove) - 1) * 2 + (tabu->vez == PRETO ? 1 : 0);
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

