//-----------------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%% Direitos Autorais segundo normas do codigo-livre: (GNU - GPL)
//%% Universidade Estadual Paulista - UNESP, Universidade Federal de Pernambuco - UFPE
//%% Jogo de xadrez: xadreco
//%% Arquivo xadreco.c
//%% Tecnica: MiniMax
//%% Autor: Ruben Carlo Benante
//%% Contato: rcb@beco.cc
//%% criacao: 10/06/1999
//%% versao para XBoard/WinBoard: 26/09/04
//%% versao 6.1, 2026-04-15
//%% edicao: 2026-04-15
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
//define a qtd de linhas no livro de abertura
//#define LINHASBOAS 483 (fix: trocado para valor dinamico / changed to count it dynamically)
//numero do movimento que a maquina pode pedir empate pela primeira vez
#define MOVE_EMPATE1 52
//numero do movimento que a maquina pode pedir empate pela segunda vez
#define MOVE_EMPATE2 88
// pede empate se tiver menos que 2 PEOES
#define QUANTO_EMPATE1 -200
//pede empate se lances > MOVE_EMPATE2 e tem menos que 0.2 PEAO
#define QUANTO_EMPATE2 20
//flagvf de geramov, so para retornar rapido com um lance valido ou NULL
#define AFOGOU -2
// geramodo de geramov: gera todos, gera unico (confere afogado), gera este (TODO PLAN9)
#define GERA_TUDO  0
#define GERA_UNICO 1
#define GERA_ESTE  2

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

typedef struct stabuleiro
{
    //contem as pecas, sendo [coluna][linha], ou seja: e4
    int tab[64];
    //contem -1 ou 1 para 'branca' ou 'preta'
    int vez;
    //contem coluna do peao adversario que andou duas, ou -1 para 'nao pode comer enpassant'
    int peao_pulou;
    //roque: 0:nao pode mais. 1:pode ambos. 2:mexeu Torre do Rei. Pode O-O-O. 3:mexeu Torre da Dama. Pode O-O.
    int roqueb, roquep;
    //contador: quando chega a 50, empate.
    float empate_50;
    //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.) 7: * sem resultado
    int situa;
    //lance executado originador deste tabuleiro.
    int lancex[4];
    //0:nada. 1:roque pqn. 2:roque grd. 3:comeu en passant.
    //promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
    int especial;
    //meionum: meio-numero (ply/half-move): 0=inicio, 1=e2e4, 2=e7e5, 3=g1f3, 4=b8c6
    int meionum;
}
tabuleiro;

typedef struct smovimento
{
    //lance em notacao inteira
    int lance[4];
    //contem coluna do peao que andou duas neste lance
    int peao_pulou;
    //roque: 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
    int roque;
    //Quando igual a:0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Entao zera empate_50;
    //TODO 4=roque eh irreversivel e deveria recontar o empate_50, 5=promocao (incluida em 1?)
    int flag_50;
    //0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant.
    //promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
    int especial;
    //dado um tabuleiro, este e o valor estatico deste tabuleiro apos a execucao deste movimento, no contexto da analise minimax
    int valor_estatico;
    //flag para dizer se ja foi inserido esse elemento na lista ordenada;
    int ordena;
    //ponteiro para o proximo movimento da lista
    struct smovimento *prox;
}
movimento;

//resultado de uma analise de posicao
typedef struct sresultado
{
    //valor da variante
    int valor;
    //os movimentos da variante
    movimento *plance;
}
resultado;

//valor das pecas (positivo==pretas)
enum piece_values
{
    REI = 10000, DAMA = 900, TORRE = 500, BISPO = 325, CAVALO = 300, PEAO = 100, VAZIA = 0, NULA = -1
};

/* ---------------------------------------------------------------------- */
/* globals */

// listas em arenas ----------------------------------
lista *pltab = NULL; // ponteiro para lista de tabuleiros
lista *plmov = NULL; // ponteiro para lista de movimentos (substitui succ_geral)

// listas --------------------------------------------
//a melhor variante achada (lista movimento)
resultado result;
//ponteiro para a primeira lista de movimentos de uma sequencia de niveis a ser analisadas;
movimento *succ_geral;

// outros globais ------------------------------------
//my rating and opponent rating
int myrating, opprating;
//log file for debug==2
FILE *fmini;
//1:consulta o livro de aberturas livro.txt. 0:nao consulta
int USALIVRO;
// numero de linhas a tentar. Abaixo de "#LINHASRUINS: ..." nao tentar
int LINHASBOAS = 0;
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
//pecas brancas sao negativas
const int brancas = -1;
//pecas pretas sao positivas
const int pretas = 1;
//para mostrar um rostinho sorrindo (sem uso)
unsigned char gira = (unsigned char) 0;
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
//mostra na tela informacoes do jogo e analises
void mostra_lances(tabuleiro tabu);
//transforma lances int 0077 em char tipo a1h8
void lance2movi(char *m, int *l, int especial);
//faz o contrario: char b1c3 em int 1022. Retorna falso se nao existe.
int movi2lance(int *l, char *m);
//retorna o adversario de quem esta na vez
inline int adv(int vez);
//retorna 1, -1 ou 0. Sinal de x
inline int sinal(int x);
//compara dois vetores de lance[4]. Se igual, retorna 1
int igual(int *lance1, int *lance2);
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
//coloca as pecas na posicao inicial
void coloca_pecas(tabuleiro *tabu);
//testa posicao dada. devera ser melhorado.
void testapos(char *pieces, char *color, char *castle, char *enpassant, char *halfmove, char *fullmove);
//retorna um lance do jogo de teste
void testajogo(char *movinito, int mnum);
//limpa algumas variaveis para iniciar a ponderacao
void limpa_pensa(void);
// lista tipo movimento enche_pmovi, op malloc+append, usada por geramov()
//preenche a estrutura da lista de movimento
//pp peao_pulou: contem -1 ou coluna do peao que andou duas neste lance
//rr roque: 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
//ee especial: 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
//ff flag_50: 0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
void enche_pmovi(movimento **cabeca, movimento **pmovi, int c0, int c1, int c2, int c3, int pp, int rr, int ee, int ff, int *nmovi);
//preenche a estrutura movimento usando arena e lst_insere (substitui enche_pmovi)
void enche_lmovi(lista *lmov, int c0, int c1, int c2, int c3, int p, int r, int e, int f);
//mensagem antes de sair do programa (por falta de memoria etc, ou tudo ok)
void msgsai(char *msg, int error);
//imprime uma sequencia de lances armazenada na lista movimento, numerados.
void imprime_linha(movimento *loop, int mnum, int vez);
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
int valido(tabuleiro tabu, int *lanc, movimento *result);
//retorna char que indica a situacao do tabuleiro, como mate, empate, etc...
char situacao(tabuleiro tabu);
//ordena succ_geral
void ordena_succ(int nmov);

// livro --------------------------------------------------------------
//retorna em result.plance uma variante do livro, baseado na posicao do tabuleiro tabu
void usalivro(tabuleiro tabu);
//pega a lista de tabuleiros e cria uma string de movimentos, como "e1e2 e7e5"
void listab2string(char *strlance);
//retorna uma (lista) linha de jogo como se fosse a resposta do minimax
movimento *string2pmovi(int mnum, char *linha);
//retorna verdadeiro se o jogo atual strlances casa com a linha do livro atual strlinha
int igual_strlances_strlinha(char *strlances, char *strlinha);
//retorna o valor estatico de um tabuleiro apos jogada a lista de movimentos cabeca
int estatico_pmovi(tabuleiro tabu, movimento *cabeca);
//dada uma linha, pegue apenas os dois primeiros movimentos (branca e preta)
void pega2moves(char *linha2, char *linha);
/* pega total de lances em strlance + 1 */
void pegaNmoves(char *linha2, char *linha, char *strlance);
/* conta quantas linhas boas tem o livro; para em #LINHASRUINS */
void conta_linhas_livro(void);

// computador joga ----------------------------------------------------------
// lista tipo movimento geramov, modos tudo, um, este; cria lista via enche_pmovi
//retorna lista de lances possiveis, ordenados por xeque e captura. Deveria ser uma ordem melhor aqui.
int geramov(tabuleiro tabu, lista *lmov, int geramodo);
//coloca em result a melhor variante e seu valor.
void minimax(tabuleiro atual, int prof, int alfa, int beta, int niv);
//retorna verdadeiro se (prof>nivel) ou (prof==nivel e nao houve captura ou xeque) ou (houve Empate!)
int profsuf(tabuleiro atual, int prof, int alfa, int beta, int niv);
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

void lst_cria(arena *a, lista **pl); // cria uma lista alocada em uma arena
void lst_zera(lista *l); // libera reserva de uma lista alocada em uma arena
void lst_insere(lista *l, void *info, size_t tam); // insere um item ao final de uma lista (append)
void lst_remove(lista *l); // remove o ultimo item da lista
int lst_conta(lista *l); // conta o numero de elementos em uma lista
void lst_limpa(arena *a); // limpa pointeiro externo da lista
void lst_furafila(lista *l, no *n); // desencaixa no e reinsere na cabeca da lista
void lst_recria(lista **pl); // zera arena e recria lista do inicio
void lst_parte(lista *l); // particiona: capturas e especiais primeiro
void lst_ordem(lista *l); // ordena por valor_estatico decrescente

// prototipos listas dinamicas -----------------------------------------------------------
//retorna nova lista contendo o movimento pmovi mais a sequencia de movimentos plance. (para melhor_caminho)
movimento *copimel(movimento pmovi, movimento *plance);
//copia os itens da estrutura movimento, mas nao copia ponteiro prox. dest=font
void copimov(movimento *dest, movimento *font);
//mantem dest, e copia para dest->prox a lista encadeada font. Assim, a nova lista e (dest+font)
void copilistmovmel(movimento *dest, movimento *font);
//copia uma lista encadeada para outra nova. Retorna cabeca da lista destino
movimento *copilistmov(movimento *font);
//insere tabuleiro na arena pltab. Retorna contagem de repeticao (>=3 empate)
int tab_insere(tabuleiro tabu);
//copia font para dest. dest=font.
void copitab(tabuleiro *dest, tabuleiro *font);
// lista tipo movimento, libera da memoria uma lista encadeada de movimentos
void libera_lances(movimento **cabeca);

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
    arena_inicia(&atab, 1024 * 1024); // inicializa arena com 1Mb para historico de tabuleiros
    arena_destrutor(&atab, lst_limpa); // callback para limpar pltab
    lst_cria(&atab, &pltab); // lista de tabuleiros para historico

    arena amov; // movimentos gerados para analise
    arena_inicia(&amov, 1024 * 1024); // inicializa arena com 1Mb para movimentos
    arena_destrutor(&amov, lst_limpa); // callback para limpar plmov
    lst_cria(&amov, &plmov); // lista de movimentos para busca

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
    conta_linhas_livro(); /* atualiza LINHASBOAS; assume que livro nao cresce durante jogo */
    inicia(&tabu);  // zera variaveis
    assert(tabu.vez == brancas && "board not initialized");
    coloca_pecas(&tabu);  //coloca pecas na posicao inicial
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
            if(tabu.vez == brancas) //iniciando vez das brancas
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

        if(tabu.vez == brancas)  /* jogam as brancas */
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
                // novo jogo
                inicia(&tabu);  // zera variaveis
                coloca_pecas(&tabu);  //coloca pecas na posicao inicial
                tab_insere(tabu);
                break;
            //            continue;
            case '*':
                strcpy(ultimo_resultado, "* {Game was unfinished}");
                printf2("* {Game was unfinished}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'M':
                strcpy(ultimo_resultado, "0-1 {Black mates}");
                printf2("0-1 {Black mates}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'm':
                strcpy(ultimo_resultado, "1-0 {White mates}");
                printf2("1-0 {White mates}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'a':
                strcpy(ultimo_resultado, "1/2-1/2 {Stalemate}");
                printf2("1/2-1/2 {Stalemate}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'p':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by endless checks}");
                printf2("1/2-1/2 {Draw by endless checks}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'c':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by mutual agreement}");  //aceitar empate
                printdbg(debug, "# xadreco : offer draw, draw accepted\n");
                printf2("offer draw\n");
                printf2("1/2-1/2 {Draw by mutual agreement}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'i':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by insufficient mating material}");
                printf2("1/2-1/2 {Draw by insufficient mating material}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case '5':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by fifty moves rule}");
                printf2("1/2-1/2 {Draw by fifty moves rule}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'r':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by triple repetition}");
                printf2("1/2-1/2 {Draw by triple repetition}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'T':
                strcpy(ultimo_resultado, "0-1 {White flag fell}");
                printf2("0-1 {White flag fell}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 't':
                strcpy(ultimo_resultado, "1-0 {Black flag fell}");
                printf2("1-0 {Black flag fell}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'B':
                strcpy(ultimo_resultado, "0-1 {White resigns}");
                printf2("0-1 {White resigns}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'b':
                strcpy(ultimo_resultado, "1-0 {Black resigns}");
                printf2("1-0 {Black resigns}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
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
            //            continue;
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
    printf2("%3d %+6d %3d %7d ", nivel, result.valor, (int)difclocks(), totalnodo);
    //result.valor*10 o xboard divide por 100. centi-peao
//    if (debug) printf ("# xadreco : %d %+.2f %d %d ", nivel, result.valor / 100.0, (int)difclocks(), totalnodo);
    //result.valor/100 para a Dama ficar com valor 9
    imprime_linha(result.plance, tabu.meionum, tabu.vez);
//    fflush(stdout);
}

//fim do mostra_lances
void libera_lances(movimento **cabeca)
{
    movimento *loop, *aux;
    if(*cabeca == NULL)
        return;
    loop = *cabeca;
    while(loop != NULL)
    {
        aux = loop->prox;
        free(loop);
        loop = aux;
    }
    *cabeca = NULL;
}

// imprime o movimento
// funcao intermediaria chamada no intervalo humajoga / compjoga
void imptab(tabuleiro tabu)
{
    char movinito[80];
    movinito[79] = '\0';
    lance2movi(movinito, tabu.lancex, tabu.especial);
    /* imprime o movimento */
    if((tabu.vez == brancas && segundo == 'c') || (tabu.vez == pretas && primeiro == 'c'))
    {
        printf2("move %s\n", movinito);
        if(OFERECEREMPATE == 1)
        {
            printf2("offer draw\n");
            OFERECEREMPATE = 0;
        }
    }
}

//int para char
void lance2movi(char *m, int *l, int espec)
{
    int i;
    for(i = 0; i < 2; i++)
    {
        m[2 * i] = l[2 * i] + 'a';
        m[2 * i + 1] = l[2 * i + 1] + '1';
    }
    m[4] = '\0';
    m[5] = '\0';
    //n[2]='-';
    //colocar um tracinho no meio
    //if(flag_50>1)
    // ou xizinho caso capturou
    //              n[2]='x';
    switch(espec)
        //promocao para 4,5,6,7 == D,C,T,B
    {
        case 4: //promoveu a Dama
            m[4] = 'q';
            break;
        case 5: //promoveu a Cavalo
            m[4] = 'n';
            break;
        case 6: //promoveu a Torre
            m[4] = 'r';
            break;
        case 7: //promoveu a Bispo
            m[4] = 'b';
            break;
    }
}

//transforma char de entrada em int
int movi2lance(int *l, char *m)
{
    int i;
    for(i = 0; i < 2; i++)
        //as coordenadas origem e destino estao no tabuleiro
        if(m[2 * i] < 'a' || m[2 * i] > 'h' || m[2 * i + 1] < '1'
                || m[2 * i + 1] > '8')
            //2i e 2i+1
            return (0);
    for(i = 0; i < 2; i++)
    {
        l[2 * i] = (int)(m[2 * i] - 'a');
        //l[0]=m[0]-'a'   l[2]=m[3]-'a'
        l[2 * i + 1] = (int)(m[2 * i + 1]) - '1';
        //l[1]=m[1]-'1'   l[3]=m[4]-'1'
    }
    return (1);
}

inline int adv(int vez)
{
    return ((-1) * vez);
}

inline int sinal(int x)
{
    if(!x)
        return (0);
    return (abs(x) / x);
}

void copitab(tabuleiro *dest, tabuleiro *font)
{
    int i, j;
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
            dest->tab[SQ(i, j)] = font->tab[SQ(i, j)];
    dest->vez = font->vez;
    dest->peao_pulou = font->peao_pulou;
    dest->roqueb = font->roqueb;
    dest->roquep = font->roquep;
    dest->empate_50 = font->empate_50;
    for(i = 0; i < 4; i++)
        dest->lancex[i] = font->lancex[i];
    dest->meionum = font->meionum;
    dest->especial = font->especial;
    //0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant.
    //promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
}

// gera lista de movimentos legais na lista lmov
// geramodo: GERA_TUDO=todos, GERA_UNICO=para no primeiro, GERA_ESTE=confere legalidade de um movimento
// retorna 1 se ha movimentos, 0 se nao ha
int geramov(tabuleiro tabu, lista *lmov, int geramodo)
{
    tabuleiro tabaux;
    int i, j, k, l, m, flag;
    int col, lin;
    int peca;
    // int pp; //peao pulou
    int rr, ee, ff; //peao pulou, roque, especial e flag50;

    assert(tabu.vez == brancas || tabu.vez == pretas && "Invalid turn in geramov");
    for(i = 0; i < 8; i++)  //#coluna
        for(j = 0; j < 8; j++)  //#linha
        {
            if((tabu.tab[SQ(i, j)]) * (tabu.vez) <= 0) //casa vazia, ou cor errada:
                continue; // (+)*(-)==(-) , (-)*(+)==(-)
            peca = abs(tabu.tab[SQ(i, j)]);
            switch(peca)
            {
                case REI:
                    for(col = i - 1; col <= i + 1; col++)  //Rei anda normalmente
                        for(lin = j - 1; lin <= j + 1; lin++)
                        {
                            if(lin < 0 || lin > 7 || col < 0 || col > 7)
                                continue; //casa de indice invalido
                            if(sinal(tabu.tab[SQ(col, lin)]) == tabu.vez)
                                continue; //casa possui peca da mesma cor ou o proprio rei
                            if(ataca(adv(tabu.vez), col, lin, tabu))
                                continue; //adversario esta protegendo a casa
                            copitab(&tabaux, &tabu);
                            tabaux.tab[SQ(i, j)] = VAZIA;
                            tabaux.tab[SQ(col, lin)] = REI * tabu.vez;
                            if(!xeque_rei_das(tabu.vez, tabaux))
                            {
                                //nao pode mais fazer roque. Mexeu Rei
                                if(tabu.tab[SQ(col, lin)] == VAZIA)
                                    ff = 0; //nao capturou
                                else
                                    ff = 2; //Rei capturou peca adversaria
                                // enche_pmovi (movimento **cabeca, movimento **pmovi, int c0, int c1, int c2, int c3, int peao_pulou, int roque, int especial, int flag50, nummovi)
                                //pp contem -1 ou coluna do peao que andou duas neste lance
                                //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                //ee 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                enche_lmovi(lmov, i, j, col, lin, /*peao*/ -1, /*roque*/0, /*especial*/0, /*flag50*/ff);
                                if(geramodo == GERA_UNICO) //pergunta a geramov se afogou e retorna rapido
                                    return 1;
                                //                          pmovi->prox = (movimento *) malloc (sizeof (movimento));
                                //                          if (pmovi->prox == NULL)
                                //                              msgsai ("# Erro de alocacao de memoria em geramov 2", 2);
                                //                          pmoviant = pmovi;
                                //                          pmovi = pmovi->prox;
                                //                          pmovi->prox = NULL;
                            }
                        }
                    //----------------------------- Roque de Brancas e Pretas
                    if(tabu.vez == brancas)
                        if(tabu.roqueb == 0)  //Se nao pode mais rocar: break!
                            break;
                        else
                        {
                            //roque de brancas
                            if(tabu.roqueb != 2 && tabu.tab[SQ(7, 0)] == -TORRE)
                            {
                                //Nao mexeu TR (e ela esta la Adv poderia ter comido antes de mexer)
                                // roque pequeno
                                if(tabu.tab[SQ(5, 0)] == VAZIA && tabu.tab[SQ(6, 0)] == VAZIA) //f1,g1
                                {
                                    flag = 0;
                                    for(k = 4; k < 7; k++)  //colunas e,f,g
                                        if(ataca(pretas, k, 0, tabu))
                                        {
                                            flag = 1; //casas do roque atacadas
                                            break;
                                        }
                                    if(flag == 0)
                                    {
                                        //nao pode mais fazer roque. Mexeu Rei
                                        //Rei rocou pqn, espec=1. flag=0

                                        // enche_pmovi (pmovi, 4, 0, 6, 0, -1, 0, 1, 0);
                                        // enche_pmovi (movimento **cabeca, movimento **pmovi, int c0, int c1, int c2, int c3, int peao_pulou, int roque, int especial, int flag50, nummovi)
                                        //pp contem -1 ou coluna do peao que andou duas
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        enche_lmovi(lmov, 4, 0, 6, 0, /*pp*/ -1, /*rr*/0, /*ee*/1, /*ff*/0); //BUG flag50 zera no roque? consultar FIDE
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;

                                    }
                                } // roque grande
                            } //mexeu TR
                            if(tabu.roqueb != 3 && tabu.tab[SQ(0, 0)] == -TORRE) //Nao mexeu TD (e a torre existe!)
                            {
                                if(tabu.tab[SQ(1, 0)] == VAZIA && tabu.tab[SQ(2, 0)] == VAZIA && tabu.tab[SQ(3, 0)] == VAZIA) //b1,c1,d1
                                {
                                    flag = 0;
                                    for(k = 2; k < 5; k++)  //colunas c,d,e
                                        if(ataca(pretas, k, 0, tabu))
                                        {
                                            flag = 1; //casas do roque atacadas
                                            break;
                                        }
                                    if(flag == 0)
                                    {
                                        //nao pode mais fazer roque. Mexeu Rei
                                        //Rei rocou grd, espec=2. flag=0
                                        //                                   enche_pmovi (pmovi, 4, 0, 2, 0, -1, 0, 2, 0);
                                        enche_lmovi(lmov, 4, 0, 2, 0, /*peao*/ -1, /*roque*/0, /*especial*/2, /*flag50*/0);
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
                            if(tabu.roquep != 2 && tabu.tab[SQ(7, 7)] == TORRE) //Nao mexeu TR (e a torre nao foi capturada)
                            {
                                // roque pequeno
                                if(tabu.tab[SQ(5, 7)] == VAZIA && tabu.tab[SQ(6, 7)] == VAZIA) //f8,g8
                                {
                                    flag = 0;
                                    for(k = 4; k < 7; k++)  //colunas e,f,g
                                        if(ataca(brancas, k, 7, tabu))
                                        {
                                            flag = 1;
                                            break;
                                        }
                                    if(flag == 0)
                                    {
                                        //nao pode mais fazer roque. Mexeu Rei
                                        //Rei rocou pqn, espec=1. flag=0.
                                        //                                  enche_pmovi (pmovi, 4, 7, 6, 7, -1, 0, 1, 0);
                                        enche_lmovi(lmov, 4, 7, 6, 7, /*peao*/ -1, /*roque*/0, /*especial*/1, /*flag50*/0);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                } // roque grande.
                            } //mexeu TR
                            if(tabu.roquep != 3 && tabu.tab[SQ(0, 7)] == TORRE) //Nao mexeu TD (e a torre esta la)
                            {
                                if(tabu.tab[SQ(1, 7)] == VAZIA && tabu.tab[SQ(2, 7)] == VAZIA && tabu.tab[SQ(3, 7)] == VAZIA) //b8,c8,d8
                                {
                                    flag = 0;
                                    for(k = 2; k < 5; k++)  //colunas c,d,e
                                        if(ataca(brancas, k, 7, tabu))
                                        {
                                            flag = 1;
                                            break;
                                        }
                                    if(flag == 0)
                                    {
                                        //nao pode mais fazer roque. Mexeu Rei
                                        //Rei rocou grd, espec=2. flag=0.
                                        // enche_pmovi (pmovi, 4, 7, 2, 7, -1, 0, 2, 0);
                                        enche_lmovi(lmov, 4, 7, 2, 7, /*peao*/ -1, /*roque*/0, /*especial*/2, /*flag50*/0);
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
                            if(tabu.vez == brancas)
                            {
                                if(j == 4)  //E tambem esta na linha certa. Peao branco vai comer enpassant!
                                {
                                    tabaux.tab[SQ(tabu.peao_pulou, 5)] = -PEAO;
                                    tabaux.tab[SQ(tabu.peao_pulou, 4)] = VAZIA;
                                    tabaux.tab[SQ(i, 4)] = VAZIA;
                                    if(!xeque_rei_das(brancas, tabaux))   //nao deixa rei branco em xeque
                                    {
                                        //                                  enche_pmovi (pmovi, i, j, tabu.peao_pulou, 5, -1, 1, 3, 3);
                                        enche_lmovi(lmov, i, j, tabu.peao_pulou, 5, /*peao*/ -1, /*roque*/1, /*especial*/3, /*flag50*/3);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    }
                                }
                            }
                            else // vez das pretas.
                            {
                                if(j == 3)  // Esta na linha certa. Peao preto vai comer enpassant!
                                {
                                    tabaux.tab[SQ(tabu.peao_pulou, 2)] = PEAO;
                                    tabaux.tab[SQ(tabu.peao_pulou, 3)] = VAZIA;
                                    tabaux.tab[SQ(i, 3)] = VAZIA;
                                    if(!xeque_rei_das(pretas, tabaux))
                                    {
                                        //  enche_pmovi (pmovi, i, j, tabu.peao_pulou, 2, -1, 1, 3, 3);
                                        enche_lmovi(lmov, i, j, tabu.peao_pulou, 2, /*peao*/ -1, /*roque*/1, /*especial*/3, /*flag50*/3);
                                        if(geramodo == GERA_UNICO) //se nao afogou retorna
                                            return 1;
                                    } //deixa rei em xeque
                                } //nao esta na fila correta
                            } //fim do else da vez
                        } //nao esta na coluna adjacente ao peao_pulou
                    } //nao peao_pulou.
                    //peao andando uma casa
                    copitab(&tabaux, &tabu);  //tabaux = tabu;
                    if(tabu.vez == brancas)
                    {
                        if(j + 1 < 8)
                        {
                            if(tabu.tab[SQ(i, j + 1)] == VAZIA)
                            {
                                tabaux.tab[SQ(i, j)] = VAZIA;
                                tabaux.tab[SQ(i, j + 1)] = -PEAO;
                                if(!xeque_rei_das(brancas, tabaux))
                                {
                                    // enche_pmovi (pmovi, i, j, i, j + 1, -1, 1, 0, 1);
                                    if(j + 1 == 7)  //se promoveu
                                    {
                                        //TODO inverter laco e=4, e<8, ee++
                                        for(ee = 4; ee < 8; ee++) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_lmovi(lmov, i, j, i, j + 1, /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    else //nao promoveu
                                        enche_lmovi(lmov, i, j, i, j + 1, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/1);
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
                                tabaux.tab[SQ(i, j - 1)] = PEAO;
                                if(!xeque_rei_das(pretas, tabaux))
                                {
                                    // enche_pmovi (pmovi, i, j, i, j - 1, -1, 1, 0, 1);
                                    if(j - 1 == 0)  //se promoveu
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_lmovi(lmov, i, j, i, j - 1, /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1);
                                    }
                                    else //nao promoveu
                                        enche_lmovi(lmov, i, j, i, j - 1, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/1);
                                    if(geramodo == GERA_UNICO) //se nao afogou retorna
                                        return 1;
                                } //deixa rei em xeque
                            } //casa ocupada
                        } //passou da casa de promocao e caiu do tabuleiro!
                    }
                    //peao anda duas casas -----------
                    copitab(&tabaux, &tabu);
                    if(tabu.vez == brancas)  //vez das brancas
                    {
                        if(j == 1)  //esta na linha inicial
                        {
                            if(tabu.tab[SQ(i, 2)] == VAZIA && tabu.tab[SQ(i, 3)] == VAZIA)
                            {
                                tabaux.tab[SQ(i, 1)] = VAZIA;
                                tabaux.tab[SQ(i, 3)] = -PEAO;
                                if(!xeque_rei_das(brancas, tabaux))
                                {
                                    // enche_pmovi (pmovi, i, 1, i, 3, i, 1, 0, 1);
                                    enche_lmovi(lmov, i, 1, i, 3, /*peao*/i, /*roque*/1, /*especial*/0, /*flag50*/1);
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
                                tabaux.tab[SQ(i, 4)] = PEAO;
                                if(!xeque_rei_das(pretas, tabaux))
                                {
                                    //                                 enche_pmovi (pmovi, i, 6, i, 4, i, 1, 0, 1);
                                    enche_lmovi(lmov, i, 6, i, 4, /*peao*/i, /*roque*/1, /*especial*/0, /*flag50*/1);
                                    if(geramodo == GERA_UNICO) //se nao afogou retorna
                                        return 1;
                                } //deixa rei em xeque
                            } //alguma das casas esta ocupada
                        } //este peao ja mexeu antes
                    }
                    // peao comeu normalmente (i.e., nao eh en passant) -------------
                    copitab(&tabaux, &tabu);
                    if(tabu.vez == brancas)  //vez das brancas
                    {
                        k = i - 1;
                        if(k < 0)  //peao da torre so come para direita
                            k = i + 1;
                        do //diagonal esquerda e direita do peao.
                        {
                            //peca preta
                            if(tabu.tab[SQ(k, j + 1)] > 0)
                            {
                                tabaux.tab[SQ(i, j)] = VAZIA;
                                tabaux.tab[SQ(k, j + 1)] = -PEAO;
                                if(!xeque_rei_das(brancas, tabaux))
                                {
                                    // enche_pmovi (pmovi, i, j, k, j + 1, -1, 1, 0, flag50=3);
                                    if(j + 1 == 7)  //se promoveu comendo
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_lmovi(lmov, i, j, k, j + 1, /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3);
                                    }
                                    else //comeu sem promover
                                        enche_lmovi(lmov, i, j, k, j + 1, /*peao*/ -1, /*roque*/1, /*especial*/0, /*flag50*/3);
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
                            if(tabu.tab[SQ(k, j - 1)] < 0) //peca branca
                            {
                                tabaux.tab[SQ(i, j)] = VAZIA;
                                tabaux.tab[SQ(k, j - 1)] = PEAO;
                                if(!xeque_rei_das(pretas, tabaux))
                                {
                                    //4:dama, 5:cavalo, 6:torre, 7:bispo
                                    //peao preto promove comendo
                                    //                                 enche_pmovi (pmovi, i, j, k, j - 1, -1, 1, 0, 3);
                                    //                                 (*nmovi)++;
                                    if(j - 1 == 0)  //se promoveu comendo
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_lmovi(lmov, i, j, k, j - 1, /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3);
                                    }
                                    else //comeu sem promover
                                        enche_lmovi(lmov, i, j, k, j - 1, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/3);
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
                            if(sinal(tabu.tab[SQ(i + col, j + lin)]) == tabu.vez)
                                continue; //casa possui peca da mesma cor.
                            copitab(&tabaux, &tabu);
                            tabaux.tab[SQ(i, j)] = VAZIA;
                            tabaux.tab[SQ(i + col, j + lin)] = CAVALO * tabu.vez;
                            if(!xeque_rei_das(tabu.vez, tabaux))
                            {
                                //                             enche_pmovi (pmovi, i, j, col + i, lin + j, -1, 1, 0, 2);
                                if(tabu.tab[SQ(col + i, lin + j)] == VAZIA)
                                    ff = 0; //Cavalo nao capturou
                                else
                                    ff = 2; // Cavalo capturou peca adversaria.
                                enche_lmovi(lmov, i, j, col + i, lin + j, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/ff);
                                if(geramodo == GERA_UNICO) //se nao afogou retorna
                                    return 1;
                            }
                            //deixa rei em xeque
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
                                if(col >= 0 && col <= 7 && sinal(tabu.tab[SQ(col, j)]) != tabu.vez && l == 0)  //gira col, mantem lin
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[SQ(i, j)] = VAZIA;
                                    tabaux.tab[SQ(col, j)] = peca * tabu.vez;
                                    if(!xeque_rei_das(tabu.vez, tabaux))
                                    {
                                        rr = 1; //ainda pode fazer roque
                                        if(peca == TORRE)  //mas, se for torre ...
                                        {
                                            if(tabu.vez == brancas && j == 0)  //se for vez das brancas e linha 1
                                            {
                                                if(i == 0)  //e coluna a
                                                    rr = 3; //mexeu TD
                                                if(i == 7)  //ou coluna h
                                                    rr = 2; //mexeu TR
                                            }
                                            if(tabu.vez == pretas && j == 7)  //se for vez das pretas e linha 8
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
                                        // enche_pmovi (pmovi, i, j, col, j, -1, 1, 0, 0);
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        enche_lmovi(lmov, i, j, col, j, /*peao*/ -1, /*roque*/rr, /*especial*/ 0, /*flag50*/ff);
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
                                if(lin >= 0 && lin <= 7 && sinal(tabu.tab[SQ(i, lin)]) != tabu.vez && m == 0)  //gira lin, mantem col
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[SQ(i, j)] = VAZIA;
                                    tabaux.tab[SQ(i, lin)] = peca * tabu.vez;
                                    if(!xeque_rei_das(tabu.vez, tabaux))
                                    {
                                        rr = 1; //ainda pode fazer roque
                                        if(peca == TORRE)  //mas, se for torre ...
                                        {
                                            if(tabu.vez == brancas && j == 0)  //se for vez das brancas e linha 1
                                            {
                                                if(i == 0)  //e coluna a
                                                    rr = 3; //mexeu TD
                                                if(i == 7)  //ou coluna h
                                                    rr = 2; //mexeu TR
                                            }
                                            if(tabu.vez == pretas && j == 7)  //se for vez das pretas e linha 8
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
                                        //                                     enche_pmovi (pmovi, i, j, i, lin, -1, 1, 0, 0);
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        enche_lmovi(lmov, i, j, i, lin, /*peao*/ -1, /*roque*/rr, /*especial*/ 0, /*flag50*/ff);
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
                                while(col >= 0 && col <= 7 && lin >= 0 && lin <= 7 && sinal(tabu.tab[SQ(col, lin)]) != tabu.vez && flag == 0)
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[SQ(i, j)] = VAZIA;
                                    tabaux.tab[SQ(col, lin)] = peca * tabu.vez;
                                    if(!xeque_rei_das(tabu.vez, tabaux))
                                    {
                                        //enche_pmovi(pmovi, c0, c1, c2, c3, peao, roque, espec, flag);
                                        //preenche a estrutura movimento
                                        //                                     if ((*nmovi) == -1)
                                        //                                         return (movimento *) TRUE;
                                        ff = 0; //flag50 nao altera
                                        if(tabu.tab[SQ(col, lin)] != VAZIA)
                                        {
                                            ff = 2; //Dama ou Bispo capturou peca adversaria.
                                            flag = 1;
                                        }
                                        // enche_pmovi (pmovi, i, j, col, lin, -1, 1, 0, 0);
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        enche_lmovi(lmov, i, j, col, lin, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/ff);
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
//     if (cabeca == pmovi) //se a lista esta vazia

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
        if(cor == brancas)
            if(tabu.tab[SQ(icol, lin)] == -TORRE || tabu.tab[SQ(icol, lin)] == -DAMA)
                return (1);
            else
                break;
        else
            // pretas atacam a casa
            if(tabu.tab[SQ(icol, lin)] == TORRE || tabu.tab[SQ(icol, lin)] == DAMA)
                return (1);
            else
                break;
    }
    for(icol = col + 1; icol < 8; icol++)
        //sobe coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[SQ(icol, lin)] == -TORRE || tabu.tab[SQ(icol, lin)] == -DAMA)
                return (1);
            else
                break;
        else
            if(tabu.tab[SQ(icol, lin)] == TORRE || tabu.tab[SQ(icol, lin)] == DAMA)
                return (1);
            else
                break;
    }
    for(ilin = lin + 1; ilin < 8; ilin++)
        // direita na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[SQ(col, ilin)] == -TORRE || tabu.tab[SQ(col, ilin)] == -DAMA)
                return (1);
            else
                break;
        else
            if(tabu.tab[SQ(col, ilin)] == TORRE || tabu.tab[SQ(col, ilin)] == DAMA)
                return (1);
            else
                break;
    }
    for(ilin = lin - 1; ilin >= 0; ilin--)
        // esquerda na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[SQ(col, ilin)] == -TORRE || tabu.tab[SQ(col, ilin)] == -DAMA)
                return (1);
            else
                break;
        else
            if(tabu.tab[SQ(col, ilin)] == TORRE || tabu.tab[SQ(col, ilin)] == DAMA)
                return (1);
            else
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
            if(cor == brancas)
                if(tabu.tab[SQ(col + icol, lin + ilin)] == -CAVALO)
                    return (1);
                else
                    continue;
            else
                if(tabu.tab[SQ(col + icol, lin + ilin)] == CAVALO)
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
            {
                if(cor == brancas)
                {
                    if(tabu.tab[SQ(casacol, casalin)] == -BISPO
                            || tabu.tab[SQ(casacol, casalin)] == -DAMA)
                        return 1;
                    else
                        continue;
                }
                else // achou peca, mas esta nao anda em diagonal ou e' peca propria
                    if(tabu.tab[SQ(casacol, casalin)] == BISPO || tabu.tab[SQ(casacol, casalin)] == DAMA)
                        return 1;
            }
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
            if(cor == brancas)
                if(tabu.tab[SQ(icol, ilin)] == -REI)
                    return (1);
                else
                    continue;
            else
                if(tabu.tab[SQ(icol, ilin)] == REI)
                    return (1);
        }
    if(cor == brancas)
        // ataque de peao branco
    {
        if(lin > 1)
        {
            ilin = lin - 1;
            if(col - 1 >= 0)
                if(tabu.tab[SQ(col - 1, ilin)] == -PEAO)
                    return (1);
            if(col + 1 <= 7)
                if(tabu.tab[SQ(col + 1, ilin)] == -PEAO)
                    return (1);
        }
    }
    else
        //ataque de peao preto
    {
        if(lin < 6)
        {
            ilin = lin + 1;
            if(col - 1 >= 0)
                if(tabu.tab[SQ(col - 1, ilin)] == PEAO)
                    return (1);
            if(col + 1 <= 7)
                if(tabu.tab[SQ(col + 1, ilin)] == PEAO)
                    return (1);
        }
    }
    return (0);
}

//fim de int ataca(int cor, int col, int lin, tabuleiro tabu)
int xeque_rei_das(int cor, tabuleiro tabu)
{
    int ilin, icol;
    for(ilin = 0; ilin < 8; ilin++)  //roda linha
        for(icol = 0; icol < 8; icol++)              //roda coluna
            if(tabu.tab[SQ(icol, ilin)] == (REI * cor))                 //achou o rei
            {
                if(ataca(adv(cor), icol, ilin, tabu))                        //alguem ataca o rei
                    return 1;
                else
                    return 0;
            }
    //ninguem ataca o rei
    return 0;
    //nao achou o rei!!
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
    int lanc[4], moves, minutes;
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
            if(tabu->vez == brancas)
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
            if(tabu->vez == brancas)
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
            if(tabu->vez == pretas)
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
            setboard = 1;
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
                if(i < 0 || i > 8)  //BUG 8 ta errado, mas ta atingido. precisa arrumar isso.
                    msgsai("# Posicao FEN incorreta.", 24);  //em vez de msg sai, printf2("Error (Wrong FEN %s): setboard", movinito);
                switch(movinito[k])  //KkQqRrBbNnPp
                {
                    case 'K':
                        tabu->tab[SQ(i, j)] = -REI;
                        i++;
                        break;
                    case 'k':
                        tabu->tab[SQ(i, j)] = REI;
                        i++;
                        break;
                    case 'Q':
                        tabu->tab[SQ(i, j)] = -DAMA;
                        i++;
                        break;
                    case 'q':
                        tabu->tab[SQ(i, j)] = DAMA;
                        i++;
                        break;
                    case 'R':
                        tabu->tab[SQ(i, j)] = -TORRE;
                        i++;
                        break;
                    case 'r':
                        tabu->tab[SQ(i, j)] = TORRE;
                        i++;
                        break;
                    case 'B':
                        tabu->tab[SQ(i, j)] = -BISPO;
                        i++;
                        break;
                    case 'b':
                        tabu->tab[SQ(i, j)] = BISPO;
                        i++;
                        break;
                    case 'N':
                        tabu->tab[SQ(i, j)] = -CAVALO;
                        i++;
                        break;
                    case 'n':
                        tabu->tab[SQ(i, j)] = CAVALO;
                        i++;
                        break;
                    case 'P':
                        tabu->tab[SQ(i, j)] = -PEAO;
                        i++;
                        break;
                    case 'p':
                        tabu->tab[SQ(i, j)] = PEAO;
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
                tabu->vez = brancas;
            else
                tabu->vez = pretas;
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
            tabu->meionum = (atoi(movinito) -1) * 2 + (tabu->vez == pretas ? 1 : 0);
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
        /*		if(!strcmp (movinito, "ping"))
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
        if(!movi2lance(lanc, movinito))   //se nao existe este lance
        {
//            printdbg(debug, "# xadreco : Error (unknown command): %s\n", movinito);
            printf2("Error (unknown command): %s\n", movinito);
            tente = 1;
            continue;
        }
        if(!valido(*tabu, lanc, &mval))  //lanc eh int lanc[4]; preenche mval
        {
//            printdbg(debug, "# xadreco : Illegal move: %s\n", movinito);
            printf2("Illegal move: %s\n", movinito);
            tente = 1;
            continue;
        }
    }
    while(tente);    // BUG checar tempo dentro do laco e dar abort se demorar, ou flag...

    if(mval.especial > 3)  //promocao do peao 4,5,6,7: Dama, Cavalo, Torre, Bispo
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

int valido(tabuleiro tabu, int *lanc, movimento *result)
{
    arena aval;
    lista *llval = NULL;
    no *n;
    movimento *m;

    arena_inicia(&aval, 64 * 1024);
    lst_cria(&aval, &llval);
    geramov(tabu, llval, GERA_TUDO);

    n = llval->cabeca;
    while(n)
    {
        m = (movimento *)n->info;
        if(igual(m->lance, lanc))
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

//dois lances[4] sao iguais
int igual(int *l1, int *l2)
{
    int j;
    for(j = 0; j < 4; j++)
        if(l1[j] != l2[j])
            return (0);
    return (1);
}

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
            switch(tabu.tab[SQ(i, j)])
            {
                case -DAMA:
                case -TORRE:
                case -PEAO:
                    insuf_branca += 3; //3: eh suficiente
                    break;
                case DAMA:
                case TORRE:
                case PEAO:
                    insuf_preta += 3; //3: eh suficiente
                    break;
                case -BISPO:
                    insuf_branca += 2; //2: falta pelo menos mais um
                    break;
                case BISPO:
                    insuf_preta += 2; //2: falta pelo menos mais um
                    break;
                case -CAVALO:
                    insuf_branca++; //1: falta pelo menos mais dois
                    break;
                case CAVALO:
                    insuf_preta++; //1: falta pelo menos mais dois
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
            if(tabu.vez == brancas)
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
    // declaracao de variaveis locais ---------------------------------------
    char res;
    int i;
    int nv = 1;
    int melhorvalor1;
    int moveto;
    // lances calc. em maior nivel tem mais importancia?
    movimento *melhorcaminho1;
    movimento *melhor_caminho = NULL;
    movimento *succ;
    no *n;
    limpa_pensa();  //limpa algumas variaveis para iniciar a ponderacao
    melhorvalor1 = -LIMITE;
    melhorcaminho1 = NULL;

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
        if(result.plance == NULL)
            USALIVRO = 0;
        libera_lances(&melhorcaminho1);
        melhorcaminho1 = copilistmov(result.plance);
        melhorvalor1 = result.valor;
    }

    // se nao livro, random ou minimax -------------------------------------
    if(result.plance == NULL)
    {
        //mudou para busca em amplitude: variavel nivel sem uso. Implementar "sd n"
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
        //funcao COMPJOGA (CONFERIR)
        if(randomchess)
        {
            n = plmov->cabeca;
            result.plance = NULL;
            result.valor = 0;
            moveto = (int)(rand() % plmov->qtd);  //sorteia um lance possivel da lista de lances
            for(i = 0; i < moveto; ++i)
                if(n != NULL)
                    n = n->prox; //escolhe este lance como o que sera jogado

            if(n != NULL)
            {
                succ = (movimento *)n->info;
                melhor_caminho = copimel(*succ, result.plance);
                succ->valor_estatico = 0;
                result.valor = 0;
                libera_lances(&result.plance);
                result.plance = copilistmov(melhor_caminho);
                libera_lances(&melhor_caminho);
                melhorcaminho1 = copilistmov(result.plance);
                melhorvalor1 = result.valor;
            } //if n
        } //if randomchess
        else
            while(result.valor < XEQUEMATE)
            {
                limpa_pensa();
                if(debug == 2)  //nivel extra de debug
                {
                    fprintf(fmini, "#\n#\n# *************************************************************");
                    fprintf(fmini, "#\n# minimax(*tabu, prof=0, alfa=%d, beta=%d, nv=%d)",
                            -LIMITE, LIMITE, nv);
                }
                //tabuleiro atual, profundidade zero, limite minimo de estatico (alfa ou passo), limite maximo de estatico (beta ou uso), nivel da busca
                //profflag=1; //pode analisar.
                minimax(*tabu, 0, -LIMITE, +LIMITE, nv);
                //retorna o melhor caminho a partir de tab...
                if(result.plance == NULL)
                {
//                    printf ("# Error compjoga 2857: Sem lances; result.plance==NULL; break; \n");
                    //Nova definicao: sem lances, pode ser que queira avancar apos mate.
                    //20131202 NULL nao implica nao ter lance. Pode ter no nivel anterior
                    break;
                }
                //18/10/2004, 19:20 +3 GMT. Descobri e criei esse teste apos muito sofrimento em debugs. Fim da ver. cinco!
                //01/12/2013, funcao difclocks trabalhando com segundos (nao mais centisegundos)
                if(difclocks() < tempomovclock)
                {
                    libera_lances(&melhorcaminho1);
                    melhorcaminho1 = copilistmov(result.plance);
                    melhorvalor1 = result.valor;
                }
                else
                    if(melhorcaminho1 != NULL) /* time exceeded and we have a move: stop now */
                        break;
                totalnodo += totalnodonivel;
                lst_ordem(plmov);  //ordena lista de movimentos
                if(debug == 2)
                {
                    fprintf(fmini, "#\n# result.valor: %+.2f totalnodo: %d\n# result.plance: ", result.valor / 100.0, totalnodo);
                    if(!mostrapensando || abs(result.valor) == FIMTEMPO || abs(result.valor) == LIMITE)
                        imprime_linha(result.plance, 1, 2);
                    //1=mnum do lance, 2=vez: pular impressao na tela.
                    //tabu->meionum+1, -tabu->vez);
                }
                //nivel pontuacao tempo totalnodo variacao === usado!
                //nivel tempo valor lances no arena. (nao usado aqui. testar)
                if(mostrapensando && abs(result.valor) != FIMTEMPO && abs(result.valor) != LIMITE)
                {
//                    printf ("# xadreco : ply score time nodes pv\n");
                    printf("%3d %+6d %3d %7d ", nv, result.valor, (int)difclocks(), totalnodo);
                    imprime_linha(result.plance, tabu->meionum + 1, -tabu->vez);
//                    printdbg(debug, "# xadreco : nv=%d value=%+.2f time=%ds totalnodo=%d ", nv, (float) result.valor / 100.0, (int)difclocks(), totalnodo);
                }
                // termino do laco infinito baseado no tempo
                if((difclocks() > tempomovclock && debug != 2) || (debug == 2 && nv == 3))   // termino do laco infinito baseado no tempo
                {
                    if(result.plance == NULL)
                    {
                        printdbg(debug, "# compjoga 2898: Sem lances; difclocks()>tempomovclock; result.plance==NULL; (!break);\n");
                        // nv--; /* testar se pode rodar de novo o mesmo nivel e ter lance */
                        // antigo: if(dif) break! retirar o else.
                    }
                    else
                        break;
                }
                nv++; // busca em amplitude: aumenta um nivel.
            } //while result.plance < XEQUEMATE
    } // fim do se nao usou livro
    libera_lances(&result.plance);
    result.plance = copilistmov(melhorcaminho1);
    libera_lances(&melhorcaminho1);
    result.valor = melhorvalor1;
    //nivel extra de debug
    if(debug == 2)
        fclose(fmini);
    //Utilizado: ABANDONA==-2730, alterado quando contra outra engine
    if(result.valor < ABANDONA && ofereci <= 0)
    {
        printdbg(debug, "# xadreco : resign. value: %+.2f\n", result.valor / 100.0);
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

    //oferecer empate: result.valor esta invertido na vez.
    if(result.valor < QUANTO_EMPATE1 && (tabu->meionum > MOVE_EMPATE1 && tabu->meionum < MOVE_EMPATE2) && ofereci > 0)
    {
        //atencao: oferecer pode significar aceitar, se for feito logo apos uma oferta recebida.
        printdbg(debug, "# xadreco : offer draw (1) value: %+.2f\n", result.valor / 100.0);
        /* printf2("offer draw\n"); */
        OFERECEREMPATE = 1;
        --ofereci;
    }
    //oferecer empate: result.valor esta invertido na vez.
    if(result.valor < QUANTO_EMPATE2 && tabu->meionum >= MOVE_EMPATE2 && ofereci > 0)
    {
        printdbg(debug, "# xadreco : offer draw (2) value: %+.2f\n", result.valor / 100.0);
        /* printf2("offer draw\n"); */
        OFERECEREMPATE = 1;
        --ofereci;
    }
    //Nova definicao: sem lances, pode ser que queira avancar apos mate.
    //algum problema ocorreu que esta sem lances
    if(result.plance == NULL)
    {
        res = randommove(tabu);
        if(res == 'e')
            return res; //vazio mesmo! Nem aleatorio foi!
        printdbg(debug, "# xadreco : Error. I don't know what to play... Playing a random move (compjoga)!\n");
    }
    res = joga_em(tabu, *result.plance, 1);  // computador joga
    return (res);
} //fim da compjoga

char analisa(tabuleiro *tabu)
{
    tabuleiro tanalise;
    int nv = 0;
    // lances calc. em maior nivel tem mais importancia?
    //mudou para logo abaixo do scanf de humajoga
    copitab(&tanalise, tabu);
    limpa_pensa();
    if(debug == 2)
        //nivel extra de debug
    {
        fmini = fopen("minimax.log", "w");
        if(fmini == NULL)
            debug = 1;
    }
    //mudou para busca em amplitude: variavel nivel obsoleta!
    if(USALIVRO && tabu->meionum < 50 && setboard != 1)
        usalivro(*tabu);
    if(result.plance != NULL)
    {
        //tclock2 = time(NULL);
//        clock2 = clock () * 100 / CLOCKS_PER_SEC;	// retorna cloock em centesimos de segundos...
        //diftclock = difftime(tclock2 , tclock1);
//        difclock = clock2 - clock1;
        //nivel pontuacao tempo totalnodo variacao === usado!
//        printf ("# xadreco : ply score time nodes pv\n");
        printf("%3d %+6d %3d %7d ", nv, result.valor, (int)difclocks(), totalnodo);
        imprime_linha(result.plance, tabu->meionum + 1, -tabu->vez);
//        printdbg(debug, "# xadreco : nv=%d value=%+.2f time=%ds totalnodo=%d\n", nv, (float) result.valor / 100.0, (int)difclocks(), totalnodo);
//        printdbg(debug, "# \n");
    }
    else
    {
        nv = 1;
        lst_recria(&plmov);
        geramov(*tabu, plmov, GERA_TUDO);  //gera os sucessores
        totalnodo = 0;
        while(result.valor < XEQUEMATE)
            //for(nv=1; nv<=7; nv++)
            //funcao ANALISA (CONFERIR)
        {
            limpa_pensa();
            // tabuleiro atual, profundidade zero, limite minimo de estatico (alfa ou passo), limite maximo de estatico (beta ou uso), nivel da busca
            minimax(*tabu, 0, -LIMITE, LIMITE, nv);
            //retorna o melhor caminho a partir de tab...
            totalnodo += totalnodonivel;
//            clock2 = clock () * 100 / CLOCKS_PER_SEC;	// retorna cloock em centesimos de segundos...
            //tclock2 = time(NULL);
            //diftclock = difftime(tclock2 , tclock1);
//            difclock = clock2 - clock1;
            lst_ordem(plmov);
            //nivel pontuacao tempo totalnodo variacao === usado!
            if(abs(result.valor) != FIMTEMPO && abs(result.valor) != LIMITE)
            {
//                printf ("# xadreco : ply score time nodes pv\n");
                printf("%3d %+6d %3d %7d ", nv, result.valor, (int)difclocks(), totalnodo);
//                printdbg(debug, "# xadreco : nv=%d value=%+.2f time=%ds nodos=%d ", nv, (float) result.valor / 100.0, (int)difclocks(), totalnodo);
                imprime_linha(result.plance, tabu->meionum + 1, -tabu->vez);
                //1=mnum do lance, 2=vez: pular impressao na tela
//                if(debug)
//                {
//                    if (result.plance == NULL)
//                        printf ("# no moves\n");
//                    else
//                        printf ("# \n");
//                }
            }
            if(debug == 2)
            {
                fprintf(fmini, "#\n# result.valor: %+.2f totalnodo: %d\nresult.plance: ", result.valor / 100.0, totalnodo);
                imprime_linha(result.plance, 1, 2);
                //1=mnum do lance, 2=vez: pular impressao na tela.
                // tabu->meionum+1, -tabu->vez);
            }
            if((difclocks() > tempomovclock && debug != 2) || (debug == 2 && nv == 3))  // termino do laco infinito baseado no tempo
                break;
            if(result.plance == NULL)  //Nova definicao: sem lances, pode ser que queira avancar apos mate.
                break;
            nv++;
        }
    }
    if(debug == 2)          //nivel extra de debug
        fclose(fmini);
    if(result.plance == NULL)
        //...apos o termino da partida, so pode-se usar edicao, undo, etc.
        //              msgsai("# Computador sem lances validos 3", 35);
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
void minimax(tabuleiro atual, int prof, int alfa, int beta, int niv)
{
    movimento *melhor_caminho = NULL;  //PV, stays malloc (PLAN7)
    movimento *succ;
    int novo_valor, contamov = 0;
    tabuleiro tab;
    char m[8];
    no *n;
    lista *llmov = NULL;
    size_t saved;

    assert(prof >= 0 && alfa <= beta && "Invalid minimax parameters");
    if(profsuf(atual, prof, alfa, beta, niv))   // profundidade suficiente ==1 ou ==-1:tempo estourou
    {
        //coloque um nulo no ponteiro plance
        //nao eh necessario libera_lance, pois plance eh temporario apesar de global.
        //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. Prof: qual nivel estao analisando?)
        //if (debug == 2)
        //fprintf(fmini, "\nprofsuf! ");
        return;
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
        lst_cria(plmov->a, &llmov);
        geramov(atual, llmov, GERA_TUDO); // gerar lista de lances para tabuleiro imaginario
        n = llmov->cabeca;
    }

    //succ==NULL se alguem ganhou ou empatou
    //if(debug == 2) {
    //fprintf(fmini, "\nsucc=geramov(): ");
    //imprime_linha(succ, 1, 2);
    //1=mnum do lance, 2=vez: pular impressao na tela
    //}
    if(!n)
    {
        //entao o estatico refletira isso: afogamento
        //libera_lances(&result.plance); //bugbug
        result.plance = NULL;
        //coloque um nulo no ponteiro plance
        //nao eh necessario libera_lance, pois plance eh temporario apesar de global.
        result.valor = estatico(atual, 0, prof, alfa, beta);
        //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. prof: qual nivel estao analisando?)
        if(debug == 2)
            fprintf(fmini, "#NULL "); //result.valor=%+.2f", result.valor);
        if(prof != 0)
            plmov->a->usado = saved;  //rewind arena
        return;
    }
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
            lance2movi(m, succ->lance, succ->especial);
            fprintf(fmini, "#\n# nivel %d, %d-lance %s (%d%d%d%d):", prof, totalnodonivel, m, succ->lance[0], succ->lance[1], succ->lance[2], succ->lance[3]);
        }
        minimax(tab, prof + 1, -beta, -alfa, niv);  //analisa o novo tabuleiro
        lst_remove(pltab);  //retira o ultimo tabuleiro da lista
        novo_valor = -result.valor; //implementar o "random"
        if(novo_valor > alfa)  // || (novo_valor==alfa && rand()%10>7))
        {
            alfa = novo_valor;
            libera_lances(&melhor_caminho);
            melhor_caminho = copimel(*succ, result.plance);  //cria nova cabeca
        }
        if(debug == 2 && prof == 0)
        {
            lance2movi(m, succ->lance, succ->especial);
            fprintf(fmini, "#\n# novo_valor=%+.2f", novo_valor / 100.0);
            if(melhor_caminho != NULL)
            {
                fprintf(fmini, "#\n# melhor_caminho=");
                imprime_linha(melhor_caminho, 1, 2);
                //1=mnum do lance, 2=vez: pular impressao na tela
            }
        }
        //implementar o NULL-MOVE
        if(novo_valor >= beta)
            //corte alfa-beta! Conferido 2026-04-15 (alfa==passo, beta==uso, para brancas)
        {
            alfa = beta; //Aha! Retorna beta como o melhor possivel desta arvore
            if(debug == 2)
            {
                lance2movi(m, succ->lance, succ->especial);
                fprintf(fmini, "#\n# succ: novo_valor>=beta (%+.2f>=%+.2f) %s (%d%d%d%d) Corte Alfa/Beta!", novo_valor / 100.0, beta / 100.0, m, succ->lance[0], succ->lance[1], succ->lance[2], succ->lance[3]);
            }
            break; //faz o corte!
        }
        if(prof == 0)
            succ->valor_estatico = novo_valor;
        n = n->prox;
        contamov++;
        if(prof != 0 && contamov > llmov->qtd * PORCENTO_MOVIMENTOS + 1)	//tentando com 60%
            break;
    } //while(n)
    result.valor = alfa;
    libera_lances(&result.plance);
    result.plance = copilistmov(melhor_caminho);
    //funcao retorna uma cabeca nova
    libera_lances(&melhor_caminho);
    if(prof != 0)
        plmov->a->usado = saved;  //rewind arena
    if(debug == 2)
        fprintf(fmini, "#\n#------------------------------------------END Minimax prof: %d", prof);
}

int profsuf(tabuleiro atual, int prof, int alfa, int beta, int niv)
{
    char input;

    //tclock2 = time(NULL);
    //diftclock = difftime(tclock2 , tclock1);
//    clock2 = clock () * 100 / CLOCKS_PER_SEC;	// retorna cloock em centesimos de segundos...
//    difclock = clock2 - clock1;
    //se tem captura ou xeque... liberou
    //se ja passou do nivel estipulado, pare a busca incondicionalmente
    if(prof > niv)
    {
        result.plance = NULL;
        result.valor = estatico(atual, 0, prof, alfa, beta);
        //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. Prof: qual nivel estao analisando?)
        return 1;
    }
    //retorna sem analisar... Deve desconsiderar o lance
    if(difclocks() >= tempomovclock && debug != 2)
    {
        result.plance = NULL;
        result.valor = estatico(atual, 1, prof, alfa, beta);	//-FIMTEMPO;//
        return 1;
    }
    //retorna sem analisar... Deve desconsiderar o lance. Usuario clicou em "move now" (?)
    //if((int)(difclock) % 100 == 0) // a cada 100 centesimos, examina
    //{
    //  if(moveagora())
    //  {
    //	result.plance = NULL;
    //	result.valor = estatico (atual, 1, prof, alfa, beta); //-FIMTEMPO;//
    //	ungetc (input, stdin);
    //	return 1;
    //  }
    //  }

//    if(clock2 - inputcheckclock > 8) // && teminterroga==0) //CLOCKS_PER_SEC/10000)
    if(difftime(tatual, tultimoinput) >= 1) //faz poll uma vez por segundo apenas
    {
        //		printf("(%d) ",clock2 - inputcheckclock);
        if(pollinput())
        {
            do
            {
                input = getc(stdin);
                //				printf("input: %c", input);
            }
            while((input == '\n' || input == '\r') && pollinput());
            //			printf ("%c\n", input);
            //		ungetc (input, stdin);
            if(input == '?')
            {
                result.plance = NULL;
                result.valor = estatico(atual, 1, prof, alfa, beta);  //-FIMTEMPO;//
                ungetc(input, stdin);
                //				teminterroga=1;
                //				printf("teminterroga: %d",teminterroga);
                return 1;
            }
            else
            {
                ungetc(input, stdin);
//                inputcheckclock = clock2;
                tultimoinput = time(NULL);
                //				return 0;
            }
        }
    }

    //  if(teminterroga)
    //  {
    //	result.plance = NULL;
    //	result.valor = estatico (atual, 1, prof, alfa, beta); //-FIMTEMPO;//
    //	return 1;
    //  }

    if(!profflag)
        //nao liberou profflag==0 retorna
    {
        result.plance = NULL;
        result.valor = estatico(atual, 0, prof, alfa, beta);
        //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. Prof: qual nivel estao analisando?)
        return 1;
        //a profundidade ja eh sufuciente
    }
    //se esta no nivel estipulado e nao liberou
    //if(prof==niv && profflag<2)
    //return 1;
    //a profundidade ja eh sufuciente
    //se OU Nem-Chegou-no-Nivel OU Liberou, pode ir fundo
    return 0;
}

char joga_em(tabuleiro *tabu, movimento movi, int cod)
{
    int i;
    char res;

    if(movi.especial == 7)  //promocao do peao: BISPO
        tabu->tab[SQ(movi.lance[0], movi.lance[1])] = BISPO * tabu->vez;
    if(movi.especial == 6)  //promocao do peao: TORRE
        tabu->tab[SQ(movi.lance[0], movi.lance[1])] = TORRE * tabu->vez;
    if(movi.especial == 5)  //promocao do peao: CAVALO
        tabu->tab[SQ(movi.lance[0], movi.lance[1])] = CAVALO * tabu->vez;
    if(movi.especial == 4)  //promocao do peao: DAMA
        tabu->tab[SQ(movi.lance[0], movi.lance[1])] = DAMA * tabu->vez;
    if(movi.especial == 3)  //comeu en passant
        tabu->tab[SQ(tabu->peao_pulou, movi.lance[1])] = VAZIA;
    if(movi.especial == 2)  //roque grande
    {
        tabu->tab[SQ(0, movi.lance[1])] = VAZIA;
        tabu->tab[SQ(3, movi.lance[1])] = TORRE * tabu->vez;
    }
    if(movi.especial == 1)  //roque pequeno
    {
        tabu->tab[SQ(7, movi.lance[1])] = VAZIA;
        tabu->tab[SQ(5, movi.lance[1])] = TORRE * tabu->vez;
    }
    if(movi.flag_50)  //empate de 50 lances sem mover peao ou capturar
        tabu->empate_50 = 0;
    else
        tabu->empate_50 += .5;
    if(tabu->vez == brancas)
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
    tabu->tab[SQ(movi.lance[2], movi.lance[3])] =
        tabu->tab[SQ(movi.lance[0], movi.lance[1])];
    tabu->tab[SQ(movi.lance[0], movi.lance[1])] = VAZIA;
    for(i = 0; i < 4; i++)
        tabu->lancex[i] = movi.lance[i];
    tabu->meionum++;
    //conta os movimentos de cada peao (e chamado de dentro de funcao recursiva!)
    tabu->vez = adv(tabu->vez);
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

//copia ummovi no comeco da lista *plan, e retorna a cabeca desta NOVA lista (mel)
movimento *copimel(movimento ummovi, movimento *plan)
{
    movimento *mel;
    mel = (movimento *) malloc(sizeof(movimento));
    if(mel == NULL)
        msgsai("# Erro de alocacao de memoria em copimel 1", 25);
    copimov(mel, &ummovi);
    copilistmovmel(mel, plan);
    return (mel);
}

// copia do segundo lance para frente, mantendo o primeiro
void copilistmovmel(movimento *dest, movimento *font)
{
    movimento *loop;
    if(font == dest)
        msgsai("# Funcao copilistmovmel precisa de duas listas DIFERENTES.", 26);
    if(dest == NULL)
        msgsai("# Funcao copilistmovmel precisa dest!=NULL.", 27);
    if(font == NULL)
    {
        dest->prox = NULL;
        return;
    }
    dest->prox = (movimento *) malloc(sizeof(movimento));
    if(dest->prox == NULL)
        msgsai("# Erro de alocacao de memoria em copilistmovmel 1", 28);
    loop = dest->prox;
    while(font != NULL)
    {
        copimov(loop, font);
        font = font->prox;
        if(font != NULL)
        {
            loop->prox = (movimento *) malloc(sizeof(movimento));
            if(loop->prox == NULL)
                msgsai("# Erro de alocacao de memoria em copilistmovmel 2", 29);
            loop = loop->prox;
        }
    }
    loop->prox = NULL;
}

//nao compia o ponteiro prox
void copimov(movimento *dest, movimento *font)
{
    int i;
    for(i = 0; i < 4; i++)
        dest->lance[i] = font->lance[i];
    dest->peao_pulou = font->peao_pulou;
    dest->roque = font->roque;
    dest->flag_50 = font->flag_50;
    dest->especial = font->especial;
    dest->ordena = font->ordena;
    dest->valor_estatico = font->valor_estatico;
}

//cria nova lista de movimentos para destino
movimento *copilistmov(movimento *font)
{
    movimento *cabeca, *pmovi;
    if(font == NULL)
        return NULL;
    cabeca = (movimento *) malloc(sizeof(movimento));   //valor novo que sera do result.plance # BUG valgrind memory leak
    if(cabeca == NULL)
        msgsai("# Erro ao alocar memoria em copilistmov 1", 30);
    pmovi = cabeca;
    while(font != NULL)
    {
        copimov(pmovi, font);
        font = font->prox;
        if(font != NULL)
        {
            pmovi->prox = (movimento *) malloc(sizeof(movimento));
            if(pmovi->prox == NULL)
                msgsai("# Erro ao alocar memoria em copilistmov 2", 31);
            pmovi = pmovi->prox;
        }
    }
    pmovi->prox = NULL;
    return cabeca;
}

//retorna o valor tatico e estrategico de um tabuleiro, sendo valor positivo melhor quem esta na vez
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
    int ordem[64][7];
    //coloca todas pecas do tabuleiro em ordem de valor
    //64 casas, cada uma com 7 info: 0:i, 1:j, 2:peca,
    //3: qtdataquebranco, 4: menorb, 5: qtdataquepreto, 6: menorp
    //levando em conta a situacao do tabuleiro
    switch(tabu.situa)
    {
        case 3: //Brancas tomaram mate
            if(tabu.vez == brancas)
                //vez das brancas, mas...
                return -XEQUEMATE + 20 * (niv + 1);
            //elas estao em xeque-mate. Conclusao: pessimo negocio, ganha -10000. Porem, quanto mais distante o mate, melhor
            else
                return +XEQUEMATE - 20 * (niv + 1);
        //na vez das pretas, o mate nas brancas e positivo, e quanto mais distante, pior...
        case 4: //Pretas tomaram mate
            if(tabu.vez == pretas)
                return -XEQUEMATE + 20 * (niv + 1);
            else
                return +XEQUEMATE - 20 * (niv + 1);
        case 1: //Empatou
            return 0;
        //valor de uma posicao empatada.
        case 2: //A posicao eh de XEQUE!
            //analisada abaixo. nao precisa mais desse.
            //        if (tabu.meionum > 40)
            //xeque no meio-jogo e final vale mais que na abertura
            if(tabu.vez == brancas)
                //                totp += 30;
                //            else
                //                totb += 30;
                //        else if (tabu.vez == brancas)
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
            if(abs(peca) != REI)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(brancas, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(pretas, i, j, tabu, &ordem[k][6]);
            if(peca < 0)
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
            if(abs(peca) != DAMA)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(brancas, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(pretas, i, j, tabu, &ordem[k][6]);
            if(peca < 0)
            {
                //caso peca branca
                //pretas ganham 5 pontos por ataque nela
                totp += ordem[k][5] * 20;
                //dama branca no ataque ganha bonus
                if(j > 4 && tabu.meionum > 30)
                    totb += 90;
                pecab += peca;
            }
            else
            {
                totb += ordem[k][3] * 20;
                if(j < 3 && tabu.meionum > 30)
                    totp += 90;
                pecap += peca;
            }
            k++;
        }
    //acha torres.
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(abs(peca) != TORRE)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(brancas, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(pretas, i, j, tabu, &ordem[k][6]);
            if(peca < 0)
            {
                //caso peca branca
                //pretas ganham 5 pontos por ataque nela
                totp += ordem[k][5] * 20;
                //torre branca no ataque ganha bonus
                if(j > 4)
                    totb += 90;
                pecab += peca;
            }
            else
            {
                totb += ordem[k][3] * 20;
                if(j < 3)
                    totp += 90;
                pecap += peca;
            }
            k++;
        }
    // acha bispos
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(abs(peca) != BISPO)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(brancas, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(pretas, i, j, tabu, &ordem[k][6]);
            if(peca < 0)
            {
                //caso peca branca
                totp += ordem[k][5] * 10;
                pecab += peca;
            }
            //pretas ganham pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 10;
                pecap += peca;
            }
            k++;
        }
    //acha cavalos.
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(abs(peca) != CAVALO)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(brancas, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(pretas, i, j, tabu, &ordem[k][6]);
            if(peca < 0)
            {
                //caso peca branca
                totp += ordem[k][5] * 10;
                pecab += peca;
            }
            //pretas ganham 5 pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 10;
                pecap += peca;
            }
            k++;
        }
    //acha peoes.
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[SQ(i, j)];
            if(abs(peca) != PEAO)
                continue;
            ordem[k][0] = i;
            ordem[k][1] = j;
            ordem[k][2] = peca;
            ordem[k][3] = qataca(brancas, i, j, tabu, &ordem[k][4]);
            ordem[k][5] = qataca(pretas, i, j, tabu, &ordem[k][6]);
            if(peca < 0)
            {
                //caso peca branca
                totp += ordem[k][5] * 10;
                pecab += peca;
            }
            //pretas ganham 5 pontos por ataque nela
            else
            {
                totb += ordem[k][3] * 10;
                pecap += peca;
            }
            k++;
        }
    if(k < 64)
        ordem[k][0] = -1;
    //sinaliza o fim
    //as pecas estao em ordem de valor

    //--------------------------- lazy evaluation
    //ponto de vista branco (branco eh negativo, preto eh positivo)
    //inverte sinal
    pecab = (-pecab);
    material = (int)((1.0 + 75.0 / (float)(pecab + pecap)) * (float)(pecab - pecap));
    if(tabu.vez == pretas)
        material = (-material);

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
        if(peca < 0)
        {
            //se peca branca:
            //totb += abs(peca);
            //qtdp = qataca(pretas, i, j, tabu, &menorp);
            //se uma peca preta ja comeu, ela nao pode contar de novo como atacante! (corrigir)
            if(ordem[k][5] == 0)
                continue;
            //qtdb = qataca(brancas, i, j, tabu, &menorb);
            //defesas
            if(ordem[k][5] > ordem[k][3])
                totp += (ordem[k][5] - ordem[k][3]) * 10;
            else
                totb += (ordem[k][3] - ordem[k][5] + 1) * 10;
            if(ordem[k][6] < -peca)
                totp += 50;
        }
        else
        {
            //se peca preta:
            //totp += peca;
            //qtdb = qataca(brancas, i, j, tabu, &menorb);
            if(ordem[k][3] == 0)
                continue;
            //qtdp = qataca(pretas, i, j, tabu, &menorp);
            if(ordem[k][3] > ordem[k][5])
                totb += (ordem[k][3] - ordem[k][5]) * 10;
            else
                totp += (ordem[k][5] - ordem[k][3] + 1) * 10;
            if(ordem[k][4] < peca)
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
        //        if (tabu.vez == brancas && peca > 0)
        //nao eh meu prejuizio a peca cravada do adversario
        //            continue;
        //        if (tabu.vez == pretas && peca < 0)
        //nao eh meu prejuizio a peca cravada do adversario
        //            continue;
        tabu.tab[SQ(i, j)] = VAZIA;
        //imagina se essa peca nao existisse?
        if(peca < 0)
        {
            qtdp = qataca(pretas, i, j, tabu, &menorp);
            if(qtdp > ordem[k][5] && menorp < abs(peca))
                totb -= (abs(peca) / 7);
        }
        else
        {
            qtdb = qataca(brancas, i, j, tabu, &menorb);
            if(qtdb > ordem[k][3] && menorb < peca)
                totp -= (peca / 7);
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
        if(abs(ordem[k][2]) != PEAO)
            continue;
        i = ordem[k][0];
        j = ordem[k][1];
        if(ordem[k][2] == -PEAO)  //Peao branco (white pawn)
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
            if(tabu.tab[SQ(i, j)] == -PEAO)
                peaob++;
            if(tabu.tab[SQ(i, j)] == PEAO)
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
    for(cor = -1; cor < 2; cor += 2)
        for(i = 2; i < 6; i++)             //c,d,e,f
            for(j = 2; j < 6; j++)                 //3,4,5,6
                if(ataca(cor, i, j, tabu))
                {
                    if(cor == brancas)
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
        if(tabu.tab[SQ(3, 0)] == -DAMA)
            totb += 50;
        if(tabu.tab[SQ(3, 7)] == DAMA)
            totp += 50;
    }
    //bonificacao para quem fez roque na abertura
    //TODO usar flag para saber se fez roque mesmo
    if(tabu.meionum < 32 && setboard != 1)
    {
        if(tabu.tab[SQ(6, 0)] == -REI && tabu.tab[SQ(5, 0)] == -TORRE) //brancas com roque pequeno
            totb += 70;
        if(tabu.tab[SQ(2, 0)] == -REI && tabu.tab[SQ(3, 0)] == -TORRE) //brancas com roque grande
            totb += 50;
        if(tabu.tab[SQ(6, 7)] == REI && tabu.tab[SQ(5, 7)] == TORRE) //pretas com roque pequeno
            totp += 70;
        if(tabu.tab[SQ(2, 7)] == REI && tabu.tab[SQ(3, 7)] == TORRE) //pretas com roque grande
            totp += 50;
    }
    //bonificacao para rei protegido na abertura com os peoes do Escudo Real
    if(tabu.meionum < 60 && setboard != 1)
    {
        if(tabu.tab[SQ(6, 0)] == -REI &&
                //brancas com roque pequeno
                tabu.tab[SQ(6, 1)] == -PEAO &&
                tabu.tab[SQ(7, 1)] == -PEAO && tabu.tab[SQ(7, 0)] == VAZIA)
            //apenas peoes g e h
            totb += 60;
        if(tabu.tab[SQ(2, 0)] == -REI &&
                //brancas com roque grande
                tabu.tab[SQ(0, 1)] == -PEAO &&
                tabu.tab[SQ(1, 1)] == -PEAO &&
                tabu.tab[SQ(2, 1)] == -PEAO &&
                tabu.tab[SQ(0, 0)] == VAZIA && tabu.tab[SQ(1, 0)] == VAZIA)
            //peoes a, b e c
            totb += 50;
        if(tabu.tab[SQ(6, 7)] == REI &&
                //pretas com roque pequeno
                tabu.tab[SQ(6, 6)] == PEAO &&
                tabu.tab[SQ(7, 6)] == PEAO && tabu.tab[SQ(7, 7)] == VAZIA)
            //apenas peoes g e h
            totp += 60;
        if(tabu.tab[SQ(2, 7)] == REI &&
                //pretas com roque grande
                tabu.tab[SQ(0, 6)] == PEAO &&
                tabu.tab[SQ(1, 6)] == PEAO &&
                tabu.tab[SQ(2, 6)] == PEAO &&
                tabu.tab[SQ(0, 7)] == VAZIA && tabu.tab[SQ(1, 7)] == VAZIA)
            //peoes a, b e c
            totp += 50;
    }
    //penalidade se mexer o peao do bispo, cavalo ou da torre no comeco da abertura!
    if(tabu.meionum < 16 && setboard != 1)
    {
        //caso das brancas------------------
        if(tabu.tab[SQ(5, 1)] != -PEAO)
            //PBR
            totb -= 50;
        if(tabu.tab[SQ(6, 1)] != -PEAO)
            //PCR
            totb -= 40;
        if(tabu.tab[SQ(7, 1)] != -PEAO)
            //PTR
            totb -= 30;
        if(tabu.tab[SQ(0, 1)] != -PEAO)
            //PTD
            totb -= 30;
        if(tabu.tab[SQ(1, 1)] != -PEAO)
            //PCD
            totb -= 40;
        //              if(tabu.tab[SQ(2,1)]==VAZIA)
        //PBD   nao eh penalizado!
        //                      totb-=10;
        //caso das pretas-------------------
        if(tabu.tab[SQ(5, 6)] != PEAO)
            //PBR
            totp -= 50;
        if(tabu.tab[SQ(6, 6)] != PEAO)
            //PCR
            totp -= 40;
        if(tabu.tab[SQ(7, 6)] != PEAO)
            //PTR
            totp -= 30;
        if(tabu.tab[SQ(0, 6)] != PEAO)
            //PTD
            totp -= 30;
        if(tabu.tab[SQ(1, 6)] != PEAO)
            //PCD
            totp -= 40;
        //PBD   nao eh penalizado!
    }
    //prepara o retorno:----------------
    if(tabu.vez == brancas)
        // se vez das brancas
        return material + totb - totp;
    // retorna positivo se as brancas estao ganhando (ou negativo c.c.)
    else
        // se vez das pretas
        return material + totp - totb;
    // retorna positivo se as pretas estao ganhando (ou negativo c.c.)
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
    *menor = REI;
    //torre ou dama atacam a casa...
    for(icol = col - 1; icol >= 0; icol--)  //desce coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[SQ(icol, lin)] == -TORRE || tabu.tab[SQ(icol, lin)] == -DAMA)
            {
                total++;
                if(-tabu.tab[SQ(icol, lin)] < *menor)
                    *menor = -tabu.tab[SQ(icol, lin)];
                break;
            }
            else
                break;
        else // pretas atacam a casa
            if(tabu.tab[SQ(icol, lin)] == TORRE || tabu.tab[SQ(icol, lin)] == DAMA)
            {
                total++;
                if(tabu.tab[SQ(icol, lin)] < *menor)
                    *menor = tabu.tab[SQ(icol, lin)];
                break;
            }
            else
                break;
    }
    for(icol = col + 1; icol < 8; icol++)  //sobe coluna
    {
        if(tabu.tab[SQ(icol, lin)] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[SQ(icol, lin)] == -TORRE || tabu.tab[SQ(icol, lin)] == -DAMA)
            {
                total++;
                if(-tabu.tab[SQ(icol, lin)] < *menor)
                    *menor = -tabu.tab[SQ(icol, lin)];
                break;
            }
            else
                break;
        else
            if(tabu.tab[SQ(icol, lin)] == TORRE || tabu.tab[SQ(icol, lin)] == DAMA)
            {
                total++;
                if(tabu.tab[SQ(icol, lin)] < *menor)
                    *menor = tabu.tab[SQ(icol, lin)];
                break;
            }
            else
                break;
    }
    for(ilin = lin + 1; ilin < 8; ilin++)  // direita na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[SQ(col, ilin)] == -TORRE || tabu.tab[SQ(col, ilin)] == -DAMA)
            {
                total++;
                if(-tabu.tab[SQ(col, ilin)] < *menor)
                    *menor = -tabu.tab[SQ(col, ilin)];
                break;
            }
            else
                break;
        else
            if(tabu.tab[SQ(col, ilin)] == TORRE || tabu.tab[SQ(col, ilin)] == DAMA)
            {
                total++;
                if(tabu.tab[SQ(col, ilin)] < *menor)
                    *menor = tabu.tab[SQ(col, ilin)];
                break;
            }
            else
                break;
    }
    for(ilin = lin - 1; ilin >= 0; ilin--)  // esquerda na linha
    {
        if(tabu.tab[SQ(col, ilin)] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[SQ(col, ilin)] == -TORRE || tabu.tab[SQ(col, ilin)] == -DAMA)
            {
                total++;
                if(-tabu.tab[SQ(col, ilin)] < *menor)
                    *menor = -tabu.tab[SQ(col, ilin)];
                break;
            }
            else
                break;
        else //pecas pretas atacam
            if(tabu.tab[SQ(col, ilin)] == TORRE || tabu.tab[SQ(col, ilin)] == DAMA)
            {
                total++;
                if(tabu.tab[SQ(col, ilin)] < *menor)
                    *menor = tabu.tab[SQ(col, ilin)];
                break;
            }
            else
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
            if(cor == brancas)  //cavalo branco ataca?
                if(tabu.tab[SQ(col + icol, lin + ilin)] == -CAVALO)
                {
                    total++; //sim,ataca!
                    if(CAVALO < *menor)
                        *menor = CAVALO;
                }
                else
                    continue;
            else //cavalo preto ataca?
                if(tabu.tab[SQ(col + icol, lin + ilin)] == CAVALO)
                {
                    if(CAVALO < *menor)
                        *menor = CAVALO;
                    total++; //sim,ataca!
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
            {
                if(cor == brancas)
                    if(tabu.tab[SQ(casacol, casalin)] == -BISPO || tabu.tab[SQ(casacol, casalin)] == -DAMA)
                    {
                        total++;
                        if(-tabu.tab[SQ(casacol, casalin)] < *menor)
                            *menor = -tabu.tab[SQ(casacol, casalin)];
                        continue; //proxima diagonal
                    }
                    else
                        continue; // achou peca, mas esta nao anda em diagonal ou e' peca propria
                else //ataque de peca preta
                    if(tabu.tab[SQ(casacol, casalin)] == BISPO || tabu.tab[SQ(casacol, casalin)] == DAMA)
                    {
                        total++;
                        if(tabu.tab[SQ(casacol, casalin)] < *menor)
                            *menor = tabu.tab[SQ(casacol, casalin)];
                        continue; //proxima diagonal
                    }
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
            if(cor == brancas)
                if(tabu.tab[SQ(icol, ilin)] == -REI)
                {
                    total++;
                    break;
                }
                else
                    continue;
            else
                if(tabu.tab[SQ(icol, ilin)] == REI)
                {
                    total++;
                    break;
                }
        }
    // ataque de peao branco (white pawn attack)
    if(cor == brancas)
    {
        if(lin > 1)
        {
            ilin = lin - 1;
            if(col - 1 >= 0)
                if(tabu.tab[SQ(col - 1, ilin)] == -PEAO)
                {
                    if(PEAO < *menor)
                        *menor = PEAO;
                    total++;
                }
            if(col + 1 <= 7)
                if(tabu.tab[SQ(col + 1, ilin)] == -PEAO)
                {
                    if(PEAO < *menor)
                        *menor = PEAO;
                    total++;
                }
        }
    }
    else //ataque de peao preto (black pawn attack)
    {
        if(lin < 6)
        {
            ilin = lin + 1;
            if(col - 1 >= 0)
                if(tabu.tab[SQ(col - 1, ilin)] == PEAO)
                {
                    if(PEAO < *menor)
                        *menor = PEAO;
                    total++;
                }
            if(col + 1 <= 7)
                if(tabu.tab[SQ(col + 1, ilin)] == PEAO)
                {
                    if(PEAO < *menor)
                        *menor = PEAO;
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
        lance2movi(m, t->lancex, t->especial);
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

//retorna uma linha de jogo como se fosse a resposta do minimax
//com inicio no lance da vez.
movimento *string2pmovi(int mnum, char *linha)
{
    char m[8];
    int n = 0, lanc[4], i, conta = 0;
    movimento *pmovi, *pmoviant, *cabeca;
    movimento mval;
    //posicao inicial
    tabuleiro tab =
    {
        {   /* tab[64]: col 0-7, each col has rows 0-7 */
            -TORRE, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, TORRE,   /* a1-a8 */
            -CAVALO, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, CAVALO, /* b1-b8 */
            -BISPO, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, BISPO,   /* c1-c8 */
            -DAMA, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, DAMA,     /* d1-d8 */
            -REI, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, REI,       /* e1-e8 */
            -BISPO, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, BISPO,   /* f1-f8 */
            -CAVALO, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, CAVALO, /* g1-g8 */
            -TORRE, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, TORRE    /* h1-h8 */
        },
        -1, -1, 1, 1, 0, 0,
        {0, 0, 0, 0},
        0, 0
    };
    cabeca = NULL;
    pmoviant = NULL;
    pmovi = NULL;
    while(linha[n] != '\0')
    {
        n++;
        conta++;
    }
    n = 0;
    while(n < conta - 1)
    {
        for(i = 0; i < 4; i++)
            m[i] = linha[n + i];
        m[4] = '\0';
        movi2lance(lanc, m);
        if(!valido(tab, lanc, &mval))
            break;
        //chamar joga_em() apenas para atualizar esse tabuleiro local, para usar a funcao valido()
        disc = (char) joga_em(&tab, mval, 0);
        //a funcao joga_em deve inserir no listab, cod: 1:insere, 0:nao insere
        if(n / 5 >= mnum) //chegou na posicao atual! comeca inserir na lista
        {
            pmovi = (movimento *) malloc(sizeof(movimento));
            if(pmovi == NULL)
                msgsai("# Erro ao alocar memoria em string2pmovi", 38);
            copimov(pmovi, &mval);
            pmovi->prox = NULL;
            if(cabeca == NULL)
            {
                cabeca = pmovi;
                pmoviant = pmovi;
            }
            else
            {
                pmoviant->prox = pmovi;
                pmoviant = pmoviant->prox;
            }
        }
        n += 5;
    }
    return cabeca;
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

//dada uma linha, pegue apenas os dois primeiros movimentos (branca e preta)
void pega2moves(char *linha2, char *linha)
{
    strcpy(linha2, linha);
    /* 'e2e4 e7e5 \0' */
    /* '012345678\0' */
    linha2[9] = '\0'; /* nao inclui espaco apos lance */
}

/* pega total de lances em strlance + 1 */
void pegaNmoves(char *linha2, char *linha, char *strlance)
{
    int ll, lli;

    ll = strlen(strlance);
    lli = strlen(linha);

    strcpy(linha2, strlance);

    if(ll >= lli - 5) /* nao tem mais um lance para add */
        return;

    do
    {
        linha2[ll] = linha[ll];
        ll++;
    }
    while(linha[ll] != '\0' || linha[ll] == ' ');
    linha2[ll] = '\0';
}

//retorna em result.plance uma variante do livro
void usalivro(tabuleiro tabu)
{
    movimento *cabeca;
    char linha[256], strlance[256], sjoga[256], linha2[256];
    FILE *flivro;
    int sorteio, nlinha = 0;
    /* int novovalor; */
    char *p;

    cabeca = NULL;
    flivro = fopen(bookfname, "r");
    if(!flivro)
    {
        /* chamada por compjoga() e analisa() */
        libera_lances(&result.plance);
        result.plance = NULL;
        return;
    }

    if(tabu.meionum == 0)  // No primeiro lance de brancas, sorteia uma abertura
    {
        if(LINHASBOAS < 1)
        {
            fclose(flivro);
            libera_lances(&result.plance);
            result.plance = NULL;
            return;
        }

        sorteio = irand_minmax(1, LINHASBOAS + 1); /* irand_minmax  [min,max[ */
        nlinha = 0;
        while(1)
        {
            fgets(linha, 256, flivro);
            if(feof(flivro))
                break;
            if(linha[0] == '#')
            {
                if((p = strchr(linha, ' ')) != NULL)
                    * p = '\0';
                if(!strcmp(linha, "#LINHASRUINS"))
                    break; /* daqui para baixo nao conta */
                continue; /* essa eh #comentario, ignora */
            }
            nlinha++;
            if(nlinha == sorteio)
                break;
        }
        printdbg(debug, "# move 0 - sorteado= %d, linha: '%s'", sorteio, linha);

        //maximo ate as linhas boas do livro! #LINHASRUINS abaixo nao
        cabeca = string2pmovi(tabu.meionum, linha);
        result.valor = estatico_pmovi(tabu, cabeca);
        libera_lances(&result.plance);
        result.plance = copilistmov(cabeca);
    }
    else
        if(tabu.meionum == 1)  // No primeiro lance de pretas, sorteia uma possivel resposta
        {
            sjoga[0] = '\0';
            listab2string(strlance); /* um lance de brancas 'e2e4 ' */
            while(1)
            {
                fgets(linha, 256, flivro);
                if(feof(flivro))
                    break;
                if(linha[0] == '#')
                {
                    if((p = strchr(linha, ' ')) != NULL)
                        * p = '\0';
                    if(!strcmp(linha, "#LINHASRUINS"))
                        break; /* daqui para baixo nao conta */
                    continue; /* essa eh #comentario, ignora */
                }
                if(!igual_strlances_strlinha(strlance, linha))
                    continue;
                pega2moves(linha2, linha);
                if(!strcmp(linha2, sjoga))
                    continue;
                strcpy(sjoga, linha2);
                nlinha++;
            }
            printf("#Num de linhas diferentes para %s eh %d\n", strlance, nlinha);
            if(nlinha < 1)
            {
                fclose(flivro);
                libera_lances(&result.plance);
                result.plance = NULL;
                return;
            }

            fseek(flivro, 0, SEEK_SET);
            sorteio = irand_minmax(1, nlinha + 1); /* irand_minmax  [min,max[ */
            nlinha = 0;
            listab2string(strlance); /* um lance de brancas 'e2e4 ' */
            sjoga[0] = '\0';
            while(1)
            {
                fgets(linha, 256, flivro);
                if(feof(flivro))
                    break;
                if(linha[0] == '#')
                {
                    if((p = strchr(linha, ' ')) != NULL)
                        * p = '\0';
                    if(!strcmp(linha, "#LINHASRUINS"))
                        break; /* daqui para baixo nao conta */
                    continue; /* essa eh #comentario, ignora */
                }
                if(!igual_strlances_strlinha(strlance, linha))
                    continue;
                pega2moves(linha2, linha);
                if(!strcmp(linha2, sjoga)) /* igual ao anterior, nao conta */
                    continue;
                strcpy(sjoga, linha2);
                nlinha++;
                if(nlinha == sorteio)
                    break;
            }
            printdbg(debug, "# xboard move 1 - sorteado= %d, linha: '%s'", sorteio, linha);
            /* contou ate a linha sorteada, jogue */
            cabeca = string2pmovi(tabu.meionum, linha);
            result.valor = estatico_pmovi(tabu, cabeca);
            libera_lances(&result.plance);
            result.plance = copilistmov(cabeca);
        }
        else /* tab.meionum>1 ... move 0 (brancas), move 1 (pretas) ja escolhidos. Agora move 2 em diante, pega o melhor */
        {
            sjoga[0] = '\0';
            listab2string(strlance); /* N lances jogados ate entao 'e2e4 e7e5 b1c3 ... ' */
            while(1)
            {
                fgets(linha, 256, flivro);
                if(feof(flivro))
                    break;
                if(linha[0] == '#')
                {
                    if((p = strchr(linha, ' ')) != NULL)
                        * p = '\0';
                    if(!strcmp(linha, "#LINHASRUINS"))
                        break; /* daqui para baixo nao conta */
                    continue; /* essa eh #comentario, ignora */
                }
                if(!igual_strlances_strlinha(strlance, linha))
                    continue;
                pegaNmoves(linha2, linha, strlance); /* pega total de lances em strlance + 1 */
                if(!strcmp(linha2, sjoga)) /* mesma linha anterior, nao conta */
                    continue;
                strcpy(sjoga, linha2);
                nlinha++;
            }
            printf("# tab.meionum>1, Num de linhas diferentes para %s eh %d\n", strlance, nlinha);
            if(nlinha < 1)
            {
                fclose(flivro);
                libera_lances(&result.plance);
                result.plance = NULL;
                return;
            }

            fseek(flivro, 0, SEEK_SET);
            sorteio = irand_minmax(1, nlinha + 1); /* irand_minmax  [min,max[ */
            nlinha = 0;
            listab2string(strlance); /* um lance de brancas 'e2e4 ' */
            sjoga[0] = '\0';
            while(1)
            {
                fgets(linha, 256, flivro);
                if(feof(flivro))
                    break;
                if(linha[0] == '#')
                {
                    if((p = strchr(linha, ' ')) != NULL)
                        * p = '\0';
                    if(!strcmp(linha, "#LINHASRUINS"))
                        break; /* daqui para baixo nao conta */
                    continue; /* essa eh #comentario, ignora */
                }
                if(!igual_strlances_strlinha(strlance, linha))
                    continue;
                pegaNmoves(linha2, linha, strlance); /* pega total de lances em strlance + 1 */
                if(!strcmp(linha2, sjoga)) /* igual ao anterior, nao conta */
                    continue;
                strcpy(sjoga, linha2);
                nlinha++;
                if(nlinha == sorteio)
                    break;
            }
            printdbg(debug, "# xboard move > 1 - sorteado= %d, linha: '%s'", sorteio, linha);
            /* contou ate a linha sorteada, jogue */
            cabeca = string2pmovi(tabu.meionum, linha);
            result.valor = estatico_pmovi(tabu, cabeca);
            libera_lances(&result.plance);
            result.plance = copilistmov(cabeca);

            /* if(cabeca == NULL) */
            /* { */
            /*     cabeca = string2pmovi(tabu.meionum, linha); */
            /*     result.valor = estatico_pmovi(tabu, cabeca); */
            /*     libera_lances(&result.plance); */
            /*     result.plance = copilistmov(cabeca); */
            /* } */
            /* else */
            /* { */
            /*     libera_lances(&cabeca); */
            /*     cabeca = string2pmovi(tabu.meionum, linha); */
            /*     //eh a primeira linha que casou */
            /*     novovalor = estatico_pmovi(tabu, cabeca); */
            /*     if(novovalor > result.valor) */
            /*     { */
            /*         result.valor = novovalor; */
            /*         libera_lances(&result.plance); */
            /*         result.plance = copilistmov(cabeca); */
            /*     } */
            /* } */
        } //else nao e o primeiro lance nem de brancas, nem de pretas

    if(flivro)
        fclose(flivro);
    return;
}

/* conta quantas linhas boas tem o livro; para em #LINHASRUINS */
void conta_linhas_livro(void)
{
    char linha[256];
    FILE *flivro;
    char *p;

    LINHASBOAS = 0;
    flivro = fopen(bookfname, "r");
    if(!flivro)
    {
        USALIVRO = 0;
        LINHASBOAS = 0;
        return;
    }

    while(1)
    {
        fgets(linha, 256, flivro);
        if(feof(flivro))
            break;
        if((p = strchr(linha, '\n')) == NULL) /* nao achou fim de linha! */
        {
            printdbg(debug, "# xboard : conta_linhas_livro ERROR - linha maior que buffer 256 bytes\n");
            break;
        }
        if(linha[0] == '#')
        {
            if((p = strchr(linha, ' ')) == NULL)
                continue; /* comentario # sem espaco, ignora */
            *p = '\0'; /* tem espaco, cerca a primeira palavra para teste */
            if(!strcmp(linha, "#LINHASRUINS"))
                break; /* tag para fim de contagem */
            continue; /* comentario qualquer, ignore */
        }
        LINHASBOAS++;
    }
    fclose(flivro);
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
    libera_lances(&result.plance);
    result.plance = NULL;
    if(plmov)
        arena_destroi(plmov->a);  //libera arena de movimentos
    if(pltab)
        arena_destroi(pltab->a);  //libera arena de tabuleiros
    exit(error);
}

//inicializa variaveis do programa. (new game)
void inicia(tabuleiro *tabu)
{
    int i, j;
    pausa = 'n';
    result.valor = 0;
    libera_lances(&result.plance);
    result.plance = NULL;
    lst_recria(&plmov);
    lst_recria(&pltab);
    ofereci = 1; //computador pode oferecer 1 empate
    USALIVRO = 0;
    if(LINHASBOAS > 0)
        USALIVRO = 1;
    //use o livro nas posicoes iniciais
    setboard = 0; //setboard command
//    if (setboard == 1) //se for -1, deixa aparecer o primeiro.
//        setboard = 0;
    tabu->roqueb = 1; // inicializar variaveis do roque e peao_pulou
    tabu->roquep = 1; //0:mexeu R e/ou 2T. 1:pode dos 2 lados. 2:mexeu TR. 3:mexeu TD.
    tabu->peao_pulou = -1; //-1:o adversario nao andou duas com peao. //0-7:coluna do peao que andou duas.
    tabu->vez = brancas;
    tabu->empate_50 = 0.0;
    tabu->meionum = 0;
    ultimo_resultado[0] = '\0';
    tabu->situa = 0;
    tabu->especial = 0;
    for(i = 0; i < 4; i++)
        tabu->lancex[i] = 0;
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
            tabu->tab[SQ(i, j)] = VAZIA;
    primeiro = 'h'; //humano inicia, com comandos para acertar detalhes do jogo
    segundo = 'c'; //computador espera para saber se jogara de brancas ou pretas
    nivel = 3; // sem uso depois de colocar busca em amplitude (uso no debug apenas)
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

//coloca as peoes na posicao inicial
void  coloca_pecas(tabuleiro *tabu)
{
    int i;
    for(i = 0; i < 8; i++)  //i = column
    {
        tabu->tab[SQ(i, 1)] = -PEAO;
        tabu->tab[SQ(i, 6)] = PEAO;
    }
    tabu->tab[SQ(0, 0)] = tabu->tab[SQ(7, 0)] = -TORRE;
    tabu->tab[SQ(0, 7)] = tabu->tab[SQ(7, 7)] = TORRE;
    tabu->tab[SQ(1, 0)] = tabu->tab[SQ(6, 0)] = -CAVALO;
    tabu->tab[SQ(1, 7)] = tabu->tab[SQ(6, 7)] = CAVALO;
    tabu->tab[SQ(2, 0)] = tabu->tab[SQ(5, 0)] = -BISPO;
    tabu->tab[SQ(2, 7)] = tabu->tab[SQ(5, 7)] = BISPO;
    tabu->tab[SQ(3, 0)] = -DAMA;
    tabu->tab[SQ(4, 0)] = -REI;
    tabu->tab[SQ(3, 7)] = DAMA;
    tabu->tab[SQ(4, 7)] = REI;
}

//limpa algumas variaveis para iniciar ponderacao
void limpa_pensa(void)
{
    libera_lances(&result.plance);
    result.plance = NULL;
    //eh necessario
    result.valor = -LIMITE;
    //conferir
    profflag = 1;
    //    totalnodo=0;
    totalnodonivel = 0;
    //  teminterroga = 0;
}

//preenche a estrutura movimento
void enche_pmovi(movimento **cabeca, movimento **pmovi, int c0, int c1, int c2, int c3, int p, int r, int e, int f, int *nmovi)
{
    movimento *paux = (movimento *) malloc(sizeof(movimento));
    if(paux == NULL)
        msgsai("# Erro de alocacao de memoria em enche_pmovi 1", 1);

    if((*cabeca) == NULL)
        (*cabeca) = (*pmovi) = paux;
    else
    {
        (*pmovi)->prox = paux;
        (*pmovi) = (*pmovi)->prox;
    }
    (*pmovi)->prox = NULL;
    (*pmovi)->lance[0] = c0; //lance em notacao inteira
    (*pmovi)->lance[1] = c1;
    (*pmovi)->lance[2] = c2;
    (*pmovi)->lance[3] = c3;
    (*pmovi)->peao_pulou = p; //contem coluna do peao que andou duas neste lance
    (*pmovi)->roque = r;   //0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
    (*pmovi)->especial = e; //0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
    (*pmovi)->flag_50 = f; //:0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Entao zera empate_50;
    (*pmovi)->ordena = 0; //flag para ordenar a lista de movimentos segundo o valor_estatico
    (*pmovi)->valor_estatico = 0; //valor deste lance, dado um tabuleiro para ele.
    (*nmovi)++;
}

//preenche a estrutura movimento usando arena e lst_insere (substitui enche_pmovi)
void enche_lmovi(lista *lmov, int c0, int c1, int c2, int c3, int p, int r, int e, int f)
{
    if(!lmov)
        return;
    movimento *m = (movimento *)arena_aloca(lmov->a, sizeof(movimento));
    if(!m)
        msgsai("# Erro arena cheia em enche_lmovi", 37);
    m->lance[0] = c0;
    m->lance[1] = c1;
    m->lance[2] = c2;
    m->lance[3] = c3;
    m->peao_pulou = p;
    m->roque = r;
    m->especial = e;
    m->flag_50 = f;
    m->ordena = 0;
    m->valor_estatico = 0;
    lst_insere(lmov, m, sizeof(movimento));
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
    char move[5];
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

//imprime uma sequencia de lances armazenada na lista movimento
//tabuvez==2 para pular numeracao em debug==2
void imprime_linha(movimento *loop, int mnum, int tabuvez)
{
    int num, vez;
    char m[80];
//    FILE *fimprime;
    num = (int)((mnum + 1.0) / 2.0);
    vez = tabuvez;
//    fimprime = NULL;
//    if(debug == 2)
//        fimprime = fmini;
//    if(debug == 1)
//        fimprime = fsaida;
    printdbg(debug, "# ");
    while(loop != NULL)
    {
        lance2movi(m, loop->lance, loop->especial);
        if(vez == brancas)  //jogou anterior as pretas
        {
            if(tabuvez == brancas && num == (int)((mnum + 1.0) / 2.0))
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
        loop = loop->prox;
        vez *= (-1);
    }
    printf("\n");
}

//ordena succ_geral
void ordena_succ(int nmov)
{
    movimento *loop, *pior, *insere, *aux;
    int peguei, pior_valor;
    if(nmov <= 1)
        return;
    insere = NULL;
    pior = NULL;
    loop = succ_geral;
    if(loop == NULL)  //se a lista esta vazia
        return;
    while(loop != NULL)
    {
        if(loop->flag_50 > 1 || loop->especial)  //para manter os especiais em primeiro
            loop->ordena = 1;
        else
            loop->ordena = 0;
        loop = loop->prox;
    }
    peguei = 1;
    while(peguei)
    {
        pior_valor = +XEQUEMATE + 1;
        loop = succ_geral;
        peguei = 0;
        while(loop != NULL)
        {
            if(loop->valor_estatico <= pior_valor && loop->ordena != 1)
            {
                pior = loop;
                pior_valor = loop->valor_estatico;
                peguei = 1;
            }
            loop = loop->prox;
        } //fim do while loop, e consequentemente da lista
        if(peguei)
        {
            aux = copimel(*pior, insere);
            //insere pior na cabeca (a lista  fica em ordem decrescente: o melhor na cabeca)
            libera_lances(&insere);
            insere = aux;
            pior->ordena = 1;
        }
    } //fim do while trocou
    //os primeiros ficam intactos
    //conta com a funcao geramov, que coloca os especiais na cabeca
    loop = succ_geral;
    while(loop != NULL)
        if(loop->flag_50 > 1 || loop->especial)
        {
            aux = copimel(*loop, insere);  //movimento *copimel(movimento ummovi, movimento *plan)
            libera_lances(&insere);
            insere = aux;
            loop = loop->prox;
        }
        else
            break;
    libera_lances(&succ_geral);
    succ_geral = insere;
}

//retorna o valor estatico de um tabuleiro apos jogada a lista de movimentos cabeca
int estatico_pmovi(tabuleiro tabu, movimento *cabeca)
{
    //cod: 1: acabou o tempo, 0: ou eh avaliacao normal?
    //niv: qual a distancia do tabuleiro real para a copia tabu avaliada?
    int niv = 0;
    while(cabeca != NULL)
    {
        disc = (char) joga_em(&tabu, *cabeca, 0);
        //a funcao joga_em deve inserir no listab, cod: 1:insere, 0:nao insere
        //com isso surge o problema de que o programa nao detecta empate por 3 repeticoes enquanto le o livro de aberturas.
        //a solucao simples e nao incluir no livro de abertura linhas que empatam com tres repeticoes na fase de abertura (se e que isso existe!)
        cabeca = cabeca->prox;
        niv++;
    }
    return estatico(tabu, 0, niv, -LIMITE, LIMITE);
    //      return rand_minmax(-8, +8);
    /* sorteio = irand_minmax(-8, 9); /-* irand_minmax  [min,max[ */
}

// pollinput() Emprestado do jogo de xadrez pepito: dica de Fabio Maia
// pollinput() Borrowed from pepito: tip from Fabio Maia
// retorna verdadeiro se existe algum caracter no buffer para ser lido
// return true if there are some character in the buffer to be read
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

//retorna verdadeiro se leu o comando "?" para mover agora
//return true if there is a "?" command to move now
//int moveagora(void)


//    clock2 = clock () * 100 / CLOCKS_PER_SEC;	// retorna cloock em centesimos de segundos...
//    difclock = clock2 - clock1;
// difclocks = valores em segundos
double difclocks(void)
{
    tatual = time(NULL);
    tdifs = difftime(tatual, tinimov);
    return tdifs;
}

char randommove(tabuleiro *tabu)
{
    int moveto;
    movimento *succ, *melhor_caminho = NULL;
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
        melhor_caminho = copimel(*succ, NULL);
        result.valor = 0;
        result.plance = copilistmov(melhor_caminho);
        libera_lances(&melhor_caminho);
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
    printf("\nCopyright (C) 1994-%d %s <%s>, GNU GPL version 2 <http://gnu.org/licenses/gpl.html>. This  is  free  software: you are free to change and redistribute it. There is NO WARRANTY, to the extent permitted by law. USE IT AS IT IS. The author takes no responsability to any damage this software may inflige in your data.\n\n", 2018, "Ruben Carlo Benante", "rcb@beco.cc");
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
void lst_cria(arena *a, lista **pl)
{
    if(!pl)
        return;
    lista *l = (lista *)arena_aloca(a, sizeof(lista));
    if(!l)
    {
        *pl = NULL; // endereco da lista global armazenada
        return;
    }
    l->pl = pl; // a lista sabe o endereco de onde ela mesma esta
    *pl = l; // endereco da lista global armazenada
    l->cabeca = NULL;
    l->cauda = NULL;
    l->qtd = 0;
    l->a = a;
    return;
}

// libera reserva de uma lista alocada em uma arena
void lst_zera(lista *l)
{
    l->cabeca = NULL; // sem free, a vantagem da arena
    l->cauda = NULL; // assim, duma vez
    l->qtd = 0; // zerou numero de items
}

// insere um item ao final de uma lista (append)
void lst_insere(lista *l, void *i, size_t tam)
{
    no *n = (no *)arena_aloca(l->a, sizeof(no));

    if(!n)
        return;

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
    arena *a;
    if(!*pl)
        return;
    a = (*pl)->a;
    arena_zera(a);
    lst_cria(a, pl);
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
    lst_insere(pltab, t, sizeof(tabuleiro));

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

