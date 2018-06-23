//-----------------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%% Direitos Autorais segundo normas do codigo-livre: (GNU - GPL)
//%% Universidade Estadual Paulista - UNESP, Universidade Federal de Pernambuco - UFPE
//%% Jogo de xadrez: xadreco
//%% Arquivo xadreco.c
//%% Tecnica: MiniMax
//%% Autor: Ruben Carlo Benante
//%% criacao: 10/06/1999
//%% versao para XBoard/WinBoard: 26/09/04
//%% versao 5.8, de 27/10/10
//%% e-mail do autor: rcb@beco.cc
//%% edicao: 2013-19-11
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//-----------------------------------------------------------------------------
/***************************************************************************
 *   xadreco version 5.8. Chess engine compatible                          *
 *   with XBoard/WinBoard Protocol (C)                                     *
 *   Copyright (C) 1994-2018 by Ruben Carlo Benante <rcb@beco.cc>          *
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

#ifdef __linux
    #warning Linux detected
    #include <sys/select.h>
    #include <sys/time.h>
    #include <unistd.h>
#else
    #warning Not Linux detected (assuming Win32)
    #include <conio.h>
    #include <windows.h>
    #include <winbase.h>
    #include <wincon.h>
    #include <io.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> /* get options from system argc/argv */
#include <stdarg.h> /* function with multiple arguments */

/* ---------------------------------------------------------------------- */
/* definitions */

#ifdef __linux
    #define waits(s) sleep(s)
#else
    #define waits(s) Sleep(s*1000)
#endif

#ifdef __arm__
    #warning raspberry pi detected
#else
    #warning not a raspberry pi, are you?
#endif

//Versao do programa
//program version
#ifndef VERSION /* gcc -DVERSION="0.1" */
#define VERSION "5.84" /**< Version Number (string) */
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

#define TOTAL_MOVIMENTOS 60
//estimativa da duracao do jogo
//estimate of game lenght
#define MARGEM_PREGUICA 400
//margem para usar a estatica preguicosa
//margin to use the lazy evaluation
#define PORCENTO_MOVIMENTOS 0.85
//a porcentagem de movimentos considerados, do total de opcoes
//the percent of the total moves considered
#define FIMTEMPO 106550
//usada para indicar que o tempo acabou, na busca minimax
//used to indicate that the time is over, under minimax search
#define LIMITE 105000
//define o limite para a funcao de avaliacao estatica
//define the min/max limite of the static valuation function
#define XEQUEMATE 100000
//define o valor do xequemate
//defines the value of the checkmate
#define LINHASDOLIVRO 408
//define a qtd de linhas no livro de abertura
//define how much lines there are in the openning book
#define MOVE_EMPATE1 52
//numero do movimento que a maquina pode pedir empate pela primeira vez
//ply that engine can ask first draw
#define MOVE_EMPATE2 88
//numero do movimento que a maquina pode pedir empate pela segunda vez
//ply that engine can ask second draw
#define QUANTO_EMPATE1 -200
// pede empate se tiver menos que 2 PEOES
//ask for draw if he has less then 2 PAWNS
#define QUANTO_EMPATE2 20
//pede empate se lances > MOVE_EMPATE2 e tem menos que 0.2 PEAO
//ask for draw if ply > MOVE_EMPATE2 and he has less then 0.2 PAWN
#define AFOGOU -2
//flagvf de geramov, so para retornar rapido com um lance valido ou NULL
// dados ----------------------

typedef struct stabuleiro
{
    int tab[8][8];
    //contem as pecas, sendo [coluna][linha], ou seja: e4
    //the pieces. [column][line], example: e4
    int vez;
    //contem -1 ou 1 para 'branca' ou 'preta'
    //the turn to play: -1 white, 1 black
    int peao_pulou;
    //contem coluna do peao adversario que andou duas, ou -1 para 'nao pode comer enpassant'
    //column pawn jump two squares. -1 used to say " cannot capture 'en passant' "
    int roqueb, roquep;
    //roque: 0:nao pode mais. 1:pode ambos. 2:mexeu Torre do Rei. Pode O-O-O. 3:mexeu Torre da Dama. Pode O-O.
    //castle. 0: none. 1: can do both. 2:moved king's rook. Can do O-O-O. 3:moved queen's rook. Can do O-O.
    float empate_50;
    //contador: quando chega a 50, empate.
    //counter: when equal 50, draw by 50 moves rule.
    int situa;
    //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.) 7: * sem resultado
    //board situation: 0:nothing, 1:draw!, 2:check!, 3:white mated, 4:black mated, 5 and 6: time fell (white and black respec.) 7: * {Game was unfinished}
    int lancex[4];
    //lance executado originador deste tabuleiro.
    //the move that originate this board. (integer notation)
    int especial;
    //0:nada. 1:roque pqn. 2:roque grd. 3:comeu en passant.
    //promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
    //0:nothing. 1:king castle. 2:queen castle. 3:en passant capture.
    //promotion: 4=queen, 5=knight, 6=rook, 7=bishop.
    int numero;
    //numero do movimento (nao lance!): 1-e2e4 2-e7e5 3-g1f3 4-b8c6
    //ply number or half-moves
}
tabuleiro;

typedef struct smovimento
{
    int lance[4];
    //lance em notacao inteira
    //move in integer notation
    int peao_pulou;
    //contem coluna do peao que andou duas neste lance
    //column pawn jump two squares. -1 used to say " cannot capture 'en passant' "
    int roque;
    //roque: 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
    //castle: 0:king moved. 1: can yet. 2:moved king's rook. 3:moved queen's rook.
    int flag_50;
    //Quando igual a:0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Entao zera empate_50;
    //TODO 4=roque eh irreversivel e deveria recontar o empate_50, 5=promocao (incluida em 1?)
    //when equal to: 0:nothing, 1:pawn moved, 2:capture, 3:pawn capture. Put zero to empate_50;
    int especial;
    //0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant.
    //promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
    //0:nothing. 1:king castle. 2:queen castle. 3:en passant capture.
    //promotion: 4=queen, 5=knight, 6=rook, 7=bishop.
    int valor_estatico;
    //dado um tabuleiro, este e o valor estatico deste tabuleiro apos a execucao deste movimento, no contexto da analise minimax
    //given a board, this is the static value of the board position after this move, in the context of minimax analyzes
    int ordena;
    //flag para dizer se ja foi inserido esse elemento na lista ordenada;
    //flag that says if this move was already inserted in the new ordered list
    struct smovimento *prox;
    //ponteiro para o proximo movimento da lista
    //pointer to the next move on the list
}
movimento;

typedef struct sresultado
//resultado de uma analise de posicao
//result of an analyzed position
{
    int valor;
    //valor da variante
    //variation value
    movimento *plance;
    //os movimentos da variante
    //variation moves
}
resultado;

typedef struct slistab
//usado para armazenar o historico do jogo e comparar posicoes repetidas
//used to store the history of the game and analyze the repeated positions
{
    int tab[8][8];
    //[coluna][linha], exemplo, lance: [e][2] para [e][4]
    //the pieces. [column][line], example: e4
    int vez;
    //de quem e a vez
    //the turn to play: -1 white, 1 black
    int peao_pulou;
    //contem coluna do peao adversario que andou duas, ou -1 para nenhuma
    //column pawn jump two squares. -1 used to say "cannot capture 'en passant'"
    int roqueb, roquep;
    //1:pode para os 2 lados. 0:nao pode mais. 3:mexeu TD. 2:mexeu TR.
    //castle. 1: can do both. 0: none. 3:moved queen rook. 2:moved king rook.
    float empate_50;
    //contador:quando chega a 50, empate.
    //counter: when equal 50, draw by 50 moves rule.
    int situa;
    //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.) 7: sem resultado
    //board situation: 0:nothing, 1:draw!, 2:check!, 3:white mated, 4:black mated, 5 and 6: time fell (white and black respec.) 7: * {Game was unfinished}
    int lancex[4];
    //lance executado originador deste tabuleiro.
    //the move that originate this board. (integer notation)
    int especial;
    //0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant.
    //promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
    //0:nothing. 1:king castle. 2:queen castle. 3:en passant capture.
    //promotion: 4=queen, 5=knight, 6=rook, 7=bishop.
    int numero;
    //numero do movimento: 1-e2e4 2-e7e5 3-g1f3 4-b8c6
    //move number (not ply)
    int rep;
    //quantidade de vezes que essa posicao ja repetiu comparada
    //how many times this position repeated
    struct slistab *prox;
    //ponteiro para o proximo da lista
    //pointer to next on list
    struct slistab *ant;
    //ponteiro para o anterior da lista
    //pointer to before on list
}
listab;

int myrating, opprating;
//my rating and opponent rating
//meu rating e rating do oponente
resultado result;
//a melhor variante achada
//the best variation found
listab *plcabeca;
//cabeca da lista encadeada de posicoes repetidas
//pointer to the first position on list
listab *plfinal;
//fim da lista encadeada
//pointer to the last position on list
movimento *succ_geral;
//ponteiro para a primeira lista de movimentos de uma sequencia de niveis a ser analisadas;
//pointer to the first list move that should be analyzed
FILE *fmini; //log file for debug==2
int USALIVRO = 1;
//1:consulta o livro de aberturas livro.txt. 0:nao consulta
//1:use opening book. 0:do not use.
int ofereci;
//o computador pode oferecer empate (duas|uma) vezes.
//the engine can offer draw twice: one as he got a bad position, other in the end of game
char disc;
//sem uso. variavel de descarte, para nao sujar o stdin, dic=pega()
//not used. discard value from pega(), just to do not waring and trash stdin
int mostrapensando = 0;
//mostra as linhas que o computador esta escolhendo
//show thinking option
enum piece_values
{
    REI = 10000, DAMA = 900, TORRE = 500, BISPO = 325, CAVALO = 300, PEAO =
        100, VAZIA = 0, NULA = -1
};
//valor das pecas (positivo==pretas)
//pieces value (positive==black)
char primeiro = 'h', segundo = 'c';
//primeiro e segundo jogadores: h:humano, c:computador
//first and second players: h:human, c:computer
int analise = 0;
//0 nao analise. 1 para analise. ("exit" coloca ela como 0 novamente)
//0 do not analyze. 1 analyze game. ("exit" put it 0 again)
int randomchess = 0;
// 0: pensa para jogar. 1: joga ao acaso.
// 0: think to play. 1: play at random.
int setboard = 0;
//-1 para primeira vez. 0 nao edita. 1 edita. Posicao FEN.
//-1 first time. 0 do not edit. 1 edit game. FEN Position.
char ultimo_resultado[80] = "";
//armazena o ultimo resultado, ex: 1-0 {White mates}
//stores the last result
char pausa;
//pause entre lances (computador x computador)
//pause between moves (computer x computer)
int nivel = 3;
//nivel de profundidade (agora com aprofundamento iterativo, esta sem uso)
//deep level (now with iterative search deep it is useless)
int profflag = 1;
//Flag de captura ou xeque para liberar mais um nivel em profsuf
//when capturing or checking, let the search go one more level
int totalnodo = 0;
//total de nodos analisados para fazer um lance
//total of nodes analyzed before one move
int totalnodonivel = 0;
//total de nodos analisados em um nivel da arvore
//total of nodes analyzed per level of tree

time_t tinijogo, tinimov, tatual, tultimoinput;
double tbrancasac, tpretasac; //tempo brancas/pretas acumulado
double tdifs; // diferenca em segundos tdifs=difftime(t2,t1)
//calcular o tempo gasto no minimax e outras
//calculating time in minimax function and others
double tempomovclock;	//em segundos
double tempomovclockmax; // max allowed
//3000 miliseg = 3 seg por lance = 300 seg por 100 lances = 5 min por jogo (de 100 lances)
//time per move. Examplo: 3000milisec = 3 sec per move = 300 sec per 100 moves = 5 min per game (of 100 moves)
const int brancas = -1;
//pecas brancas sao negativas
//white pieces are negatives
const int pretas = 1;
//pecas pretas sao positivas
//black pieces are positives
unsigned char gira = (unsigned char) 0;
//para mostrar um rostinho sorrindo (sem uso)
//to show a smile face (useless)
char flog[80] = "";
//Nome do arquivo de log
//log file name
int ABANDONA = -2430;
//abandona a partida quando perder algo como DAMA, 2 TORRES, 1 BISPO e 1 PEAO; ou nunca, quando jogando contra outra engine
//the engine will resign when loosing a QUEEN, 2 ROOKS, 1 BISHOP and 1 PAWN or something like that; it never resigns when playing against another engine
int COMPUTER = 0;
//flag que diz se esta jogando contra outra engine.
//Flag that says it is playing against other engine
//int teminterroga = 0;
//flag que diz que o comando "?" foi executado
//flag to mark that command "?" run
int WHISPER = 0; /*0:nada, 1:v>200 :)), 2: v>100 :), 3: -100<v<100 :|, 4: v<-100 :(, 5: v<-200 :(( */
enum e_server {none, fics, lichess} server; /* Am I connected to someone, or stand alone? */
char bookfname[80]="livro.txt"; /* book file name */
/* int verb = 0; /1* verbose variable *1/ */
int debug = 0; /* BUG: trocar por DEBUG - usando chave -v */
//coloque zero para evitar gravar arquivo. 0:sem debug, 1:-v debug, 2:-vv debug minimax
//0:do not save file xadreco.log, 1:save file, 2:minimax debug

/* ---------------------------------------------------------------------- */
/* general prototypes --------------------------------------------------------- */
// prototipos gerais ---------------------------------------------------------
void imptab(tabuleiro tabu);
//imprime o tabuleiro
//print board
void mostra_lances(tabuleiro tabu);
//mostra na tela informacoes do jogo e analises
//show on screen game information and analyzes
void lance2movi(char *m, int *l, int especial);
//transforma lances int 0077 em char tipo a1h8
//change integer notation to algebric notation
int movi2lance(int *l, char *m);
//faz o contrario: char b1c3 em int 1022. Retorna falso se nao existe.
//change algebric notation to integer notation. Return FALSE if it is not possible.
inline int adv(int vez);
//retorna o adversario de quem esta na vez
//return the other player to play
inline int sinal(int x);
//retorna 1, -1 ou 0. Sinal de x
//return 1, -1 or 0: signal of x.
int igual(int *lance1, int *lance2);
//compara dois vetores de lance[4]. Se igual, retorna 1
//return 1 if lance1==lance2
char pega(char *no, char *msg);
//pegar caracter (linux e windows)
//to solve getch, getche, getchar and whatever problems of portability
float rand_minmax(float min, float max);
//retorna um valor entre [min,max], inclusive
//return a random value between [min,max], inclusive
void sai(int error);
//termina o programa
//exit the program
void inicia(tabuleiro *tabu);
//para inicializar alguns valores no inicio do programa;
//to start some values in the beggining of the program
void coloca_pecas(tabuleiro *tabu);
//coloca as pecas na posicao inicial
//put the pieces in the start position
void testapos(char *pieces, char *color, char *castle, char *enpassant, char *halfmove, char *fullmove);
//testa posicao dada. devera ser melhorado.
//used to test a determined position; should be improved.
void testajogo(char *movinito, int numero);
//retorna um lance do jogo de teste
//returns a move in the test game
void limpa_pensa(void);
//limpa algumas variaveis para iniciar a ponderacao
//to clean some variables to start pondering
void enche_pmovi(movimento **cabeca, movimento **pmovi, int c0, int c1, int c2, int c3, int pp, int rr, int ee, int ff, int *nmovi);
// enche_pmovi (movimento **cabeca, movimento **pmovi, int c0, int c1, int c2, int c3, int peao_pulou, int roque, int especial, int flag50, nummovi)
//pp peao_pulou: contem -1 ou coluna do peao que andou duas neste lance
//rr roque: 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
//ee especial: 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
//ff flag_50: 0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
//preenche a estrutura movimento
//fullfil movimento structure
void msgsai(char *msg, int error);
//emite mensagem antes de sair do programa (por falta de memoria ou outros itens), ou tudo certo
//message before exit program because lack of memory or other problems, or just exiting ok.
void imprime_linha(movimento *loop, int numero, int vez);
//imprime uma sequencia de lances armazenada na lista movimento, numerados.
//print a sequence of moves in movimento list, numbered.
int pollinput(void);
// retorna verdadeiro se existe algum caracter no buffer para ser lido
// return true if there are some character in the buffer to be read
/* char *build(void);  NO NEED, use BUILD */
// compiled time - build version
// hora da compilacao - versao de construcao
// difclocks = valores em segundos
double difclocks(void);
//calcula diferenca de tempo em segundo do lance atual
//calculates time difference in seconds for the current move
//return tdifs = difftime(tatual, tinimov);
void inicia_fics(void);
//get tourney, resume, seek games...
char randommove(tabuleiro *tabu);
// joga aleatorio!
void help(void); 
/* imprime o help e termina */
/* print some help */
void copyr(void); 
/* imprime mensagem de copyright */
/* print version and copyright information */
void printfics(char *fmt, ...);
/* print fics commands */
void printdbg(int dbg, ...);
/* print debug information  */

// apoio xadrez -----------------------------------------------------
int ataca(int cor, int col, int lin, tabuleiro tabu);
//retorna 1 se "cor" ataca casa(col,lin) no tabuleiro tabu
//return 1 if "color" atack square(col, lin) in board tabu
int qataca(int cor, int col, int lin, tabuleiro tabu, int *menor);
//retorna o numero de ataques da "cor" na casa(col,lin) de tabu, *menor e a menor peca que ataca.
//return the number of atacks of "color" at square(col,lin) in board tabu, *menor is the minor piece that atack
int xeque_rei_das(int cor, tabuleiro tabu);
//retorna 1 se o rei de cor "cor" esta em xeque
//return 1 if "color" king is in check
void volta_lance(tabuleiro *tabu);
//para voltar um movimento. Use duas vezes para voltar um lance inteiro.
//take back one ply. Use twice to take back one move
char analisa(tabuleiro *tabu);
//analisa uma posicao mas nao joga
//analyze a position, but do not play it
movimento *valido(tabuleiro tabu, int *lanc);
//procura nos movimentos de geramov se o lance em questao eh valido. Retorna o *movimento preenchido. Se nao, retorna NULL.
//search on the list generated by geramov if the move lanc is valid. Return *movimento fullfiled. If not, return NULL.
char situacao(tabuleiro tabu);
//retorna char que indica a situacao do tabuleiro, como mate, empate, etc...
//return a char that indicate the situation of the board, as mate, draw, etc...
void ordena_succ(int nmov);
//ordena succ_geral
//sort succ_geral

// livro --------------------------------------------------------------
void usalivro(tabuleiro tabu);
//retorna em result.plance uma variante do livro, baseado na posicao do tabuleiro tabu
//put in result.plance a line found in the book, from the position on the board tabu
void listab2string(char *strlance);
//pega a lista de tabuleiros e cria uma string de movimentos, como "e1e2 e7e5"
//generate a string with all moves, like "e1e2 e7e5"
movimento *string2pmovi(int numero, char *linha);
//retorna uma (lista) linha de jogo como se fosse a resposta do minimax
//return a variation on a list, like the minimax's return.
int igual_strlances_strlinha(char *strlances, char *strlinha);
//retorna verdadeiro se o jogo atual strlances casa com a linha do livro atual strlinha
//return TRUE if the game strlances match with one line strlinha of the opening book
int estatico_pmovi(tabuleiro tabu, movimento *cabeca);
//retorna o valor estatico de um tabuleiro apos jogada a lista de movimentos cabeca

// computador joga ----------------------------------------------------------
movimento *geramov(tabuleiro tabu, int *nmov);
//retorna lista de lances possiveis, ordenados por xeque e captura. Deveria ser uma ordem melhor aqui.
//return a list of possibles moves, ordered by check and capture. Should put a best order here.
void minimax(tabuleiro atual, int prof, int alfa, int beta, int niv);
//coloca em result a melhor variante e seu valor.
//put in result the better variation and its value
int profsuf(tabuleiro atual, int prof, int alfa, int beta, int niv);
//retorna verdadeiro se (prof>nivel) ou (prof==nivel e nao houve captura ou xeque) ou (houve Empate!)
//return TRUE if (deep>level) or (deep==level and not capture or check) or (it is a draw)
int estatico(tabuleiro tabu, int cod, int niv, int alfa, int beta);
//retorna um valor estatico que avalia uma posicao do tabuleiro, fixa. Cod==1: tempo estourou no meio da busca. Niv: o nivel de distancia do tabuleiro real para a copia examinada
//return the static value of a position. If cod==1, means that this position was here only because the time is over in the middle of the search. Niv: the distance between the real board and the virtual copy examined.
char joga_em(tabuleiro *tabu, movimento movi, int cod);
//joga o movimento movi em tabuleiro tabu. retorna situacao. Insere no listab *plfinal se cod==1
//do the move! Move movi on board tabu. Return the situation. Insert the new board in the listab *plfina list if cod==1

// listas dinamicas ----------------------------------------------------------------
movimento *copimel(movimento pmovi, movimento *plance);
//retorna nova lista contendo o movimento pmovi mais a sequencia de movimentos plance. (para melhor_caminho)
//return a new list adding (new single move pmovi)+(old list moves plance)
void copimov(movimento *dest, movimento *font);
//copia os itens da estrutura movimento, mas nao copia ponteiro prox. dest=font
//copy only the contents of the structure, but not the pointer. dest=font
void copilistmovmel(movimento *dest, movimento *font);
//mantem dest, e copia para dest->prox a lista encadeada font. Assim, a nova lista e (dest+font)
//keep dest, and copy the list font to dest->prox. So, the new list is (dest+font)
movimento *copilistmov(movimento *font);
//copia uma lista encadeada para outra nova. Retorna cabeca da lista destino
//copy one list to another new one. Return the new head.
void insere_listab(tabuleiro tabu);
//Insere o tabuleiro tabu na lista listab. posiciona plcabeca, plfinal.   Para casos de empate de repeticao de lances, e para pegar o historico de lances
//insert the board tabu in the list listab *plcabeca, positioning plcabeca and plfinal. Use to see draw, and to get history
void retira_listab(void);
//posiciona plfinal no *ant. ou seja: apaga o ultimo da lista. (usada para voltar de variantes ruins ou para voltar um lance a pedido do jogador)
//delete the last board in the listab. (used to retract from worst variations, or to take back a move asked by player)
void copitab(tabuleiro *dest, tabuleiro *font);
//copia font para dest. dest=font.
//copy font to dest. dest=font
void libera_lances(movimento *cabeca);
//libera da memoria uma lista encadeada de movimentos
//free memory of a list of moves
void retira_tudo_listab(void);
//zera a lista de tabuleiros
//free all listab list.
char humajoga(tabuleiro *tabu);
//humano joga. Aceita comandos XBoard/WinBoard.
//human play. Accept XBoard/WinBoard commands.
char compjoga(tabuleiro *tabu);
//computador joga. Chama o livro de aberturas ou o minimax.
//computer play. Call the opening book or the minimax functions.

// funcoes -------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int opt; /* return from getopt() */
    tabuleiro tabu;
    char feature[256];
    char res;
    char movinito[80] = "wait_xboard"; /* scanf : entrada de comandos ou lances */
    int d2 = 0; /* wait for 2 dones */
//    int joga; /* flag enquanto joga */
    struct tm *tmatual;
    char hora[] = "2013-12-03 00:28:21";
    int seed=0;
    server = none;

    IFDEBUG("Starting optarg loop...");

    /* getopt() configured options:
     *        -h  help
     *        -V  version
     *        -v  verbose
     */
    /* Usage: xadreco [-h|-v] [-c{none,fics,lichess}] [-r] [-x] [-b path/bookfile.txt] */
    /* -r seed,  --random seed        Xadreco plays random moves. Initializes seed. If seed=0, 'true' random */
    /* -c,  --connection              none: no connection; fics: freeches.org; lichess: lichess.org*/
    /* -x,  --xboard                  Gives 'xboard' keyword immediattely (protocol is xboard with or withour -x) */
    /* -b path/bookfile.txt, --book   Sets the path for book file (not implemented) */
    opterr = 0;
    while((opt = getopt(argc, argv, "vhVr:c:xb:")) != EOF)
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
                else
                    srand(time(NULL));
                randomchess = 1;
                break;
            case 'c': /* kind of connection: none, fics, lichess */
                if(!strcmp("fics", optarg))
                    server = fics;
                else if(!strcmp("lichess", optarg))
                    server = lichess;
                else
                    server = none;
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

    printdbg(debug, "# DEBUG MACRO compiled value: %d\n", DEBUG);
    printdbg(debug, "# Debug verbose level set at: %d\n", debug);
    printdbg(debug, "# seed: -r %d\n", seed);
    printdbg(debug, "# connection: -c %s\n", !server?"none":server==1?"fics":"lichess");
    printdbg(debug, "# wait: -x %s\n", movinito);
    printdbg(debug, "# book: -b %s\n", bookfname);
    
    //turn off buffers. Immediate input/output.
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    /* Xadreco 5.8 accepts Xboard Protocol V2 */
    sprintf(feature, "%s%s%s", "feature ping=0 setboard=1 playother=1 san=0 usermove=0 time=0 draw=1 sigint=0 sigterm=1 reuse=0 analyze=1 myname=\"Xadreco ", VERSION, "\" variants=\"normal\" colors=0 ics=0 name=0 pause=0 nps=0 debug=1 memory=0 smp=0 exclude=0 setscore=0");
    printdbg(debug, "# Xadreco version %s build %s (C) 1994-2018, by Dr. Beco\n"
           "# Xadreco comes with ABSOLUTELY NO WARRANTY;\n"
           "# This is free software, and you are welcome to redistribute it\n"
           "# under certain conditions; Please, visit http://www.fsf.org/licenses/gpl.html\n"
           "# for details.\n\n", VERSION, BUILD);

    /* 'xboard': scanf if not there yet */
    if(strcmp("xboard", movinito))
        scanf("%s", movinito);

    if(strcmp(movinito, "xboard"))   // primeiro comando: xboard
    {
        printdbg(debug, "# xboard: %s\n", movinito);
        msgsai("# xadreco : xboard command missing.\n", 36);
    }
    printf("\n"); /* output a newline when xboard comes in */

    printf("feature done=0\n");
    printf("%s\n", feature);
    printf("feature done=1\n");

    /* comandos que aparecem no inicio, terminando com done
     * ignorar todos exceto: quit e post, ate contar dois 'done'
     */
    while(d2 < 2)
    {
        scanf("%s", movinito);

        if(!strcmp(movinito, "quit"))
            msgsai("# Thanks for playing Xadreco.", 0);
        //protover N = versao do protocolo
        //new = novo jogo, brancas iniciam
        //hard, easy = liga/desliga pondering (pensar no tempo do oponente). Ainda nao implementado.

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
                printdbg(debug, "# xboard: ignoring %s\n", movinito);
    }

    /* joga==0, fim. joga==1, novo lance. (joga==2, nova partida) */
    //------------------------------------------------------------------------------
    // novo jogo
    inicia(&tabu);  // zera variaveis
    coloca_pecas(&tabu);  //coloca pecas na posicao inicial
    insere_listab(tabu);
    if(server==fics)
        inicia_fics();
    //------------------------------------------------------------------------------
    //joga_novamente: (play another move)

    while(TRUE)
    {
        tatual = time(NULL);
        // if(debug) printf ("# xadreco : Tempo atual %s", ctime(&tatual)); //ctime returns "\n"
        tmatual = localtime(&tatual); // convert time_t to struct tm
        strftime(hora, sizeof(hora), "%F %T", tmatual);
        if(tabu.numero == 2) //pretas jogou o primeiro. Relogios iniciam
        {
            tinijogo = tinimov = tatual;
            printdbg(debug, "# xadreco : N.2. Relogio ligado em %s\n", hora);  //ctime(&tinijogo));
        }
        if(tabu.numero > 2)
        {
            tdifs = difftime(tatual, tinimov);
            if(tabu.vez == brancas) //iniciando vez das brancas
            {
                tpretasac += tdifs; //acumulou lance anterior, das pretas
                printdbg(debug, "# xadreco : N.%d. Pretas. Tempo %fs. Acumulado: %fs. Hora: %s\n", tabu.numero, tdifs, tpretasac, hora);
            }
            else
            {
                tbrancasac += tdifs;
                printdbg(debug, "# xadreco : N.%d. Brancas. Tempo %fs. Acumulado: %fs. Hora: %s\n", tabu.numero, tdifs, tbrancasac, hora);
            }
            tinimov = tatual; //ancora para proximo tempo acumulado
        }
        if(tabu.vez == brancas)  // jogam as brancas
            if(primeiro == 'c')
                res = compjoga(&tabu);
            else
                res = humajoga(&tabu);
        else // jogam as pretas
            if(segundo == 'c')
                res = compjoga(&tabu);
            else
                res = humajoga(&tabu);
        if(res != 'w' && res != 'c')
            imptab(tabu);
        switch(res)
        {
            case 'w': //Novo jogo
                // novo jogo
                inicia(&tabu);  // zera variaveis
                coloca_pecas(&tabu);  //coloca pecas na posicao inicial
                insere_listab(tabu);
                if(server==fics)
                    inicia_fics();
                break;
//                    continue;
            case '*':
                strcpy(ultimo_resultado, "* {Game was unfinished}");
                printf("* {Game was unfinished}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'M':
                strcpy(ultimo_resultado, "0-1 {Black mates}");
                printf("0-1 {Black mates}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'm':
                strcpy(ultimo_resultado, "1-0 {White mates}");
                printf("1-0 {White mates}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'a':
                strcpy(ultimo_resultado, "1/2-1/2 {Stalemate}");
                printf("1/2-1/2 {Stalemate}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'p':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by endless checks}");
                printf("1/2-1/2 {Draw by endless checks}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'c':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by mutual agreement}");  //aceitar empate
                printdbg(debug, "# xadreco : offer draw, draw accepted\n");
                printf("offer draw\n");
                printf("1/2-1/2 {Draw by mutual agreement}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'i':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by insufficient mating material}");
                printf("1/2-1/2 {Draw by insufficient mating material}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case '5':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by fifty moves rule}");
                printf("1/2-1/2 {Draw by fifty moves rule}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'r':
                strcpy(ultimo_resultado, "1/2-1/2 {Draw by triple repetition}");
                printf("1/2-1/2 {Draw by triple repetition}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'T':
                strcpy(ultimo_resultado, "0-1 {White flag fell}");
                printf("0-1 {White flag fell}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 't':
                strcpy(ultimo_resultado, "1-0 {Black flag fell}");
                printf("1-0 {Black flag fell}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'B':
                strcpy(ultimo_resultado, "0-1 {White resigns}");
                printf("0-1 {White resigns}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'b':
                strcpy(ultimo_resultado, "1-0 {Black resigns}");
                printf("1-0 {Black resigns}\n");
                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'e': //se existe um resultado, envia ele para finalizar a partida
                if(ultimo_resultado[0] != '\0')
                {
                    printdbg(debug, "# xadreco : case 'e' (empty) %s\n", ultimo_resultado);
                    printf("%s\n", ultimo_resultado);
                    primeiro = segundo = 'h';
                }
                else
                {
//                    printdbg(debug, "# case 'e' 732: Computador sem lances validos 1. Erro: 35\n");
                    printf("# xadreco : Error. I don't know what to play... (main)\n");
                    res = randommove(&tabu);
                    if(res == 'e') //vazio mesmo! Nem aleatorio foi!
                    {
                        printf("# xadreco : I really don't know what to play... resigning!\n");
                        primeiro = segundo = 'h';
                        printf("resign");
                        if(primeiro == 'c')
                        {
                            res = 'B';
                            strcpy(ultimo_resultado, "0-1 {White resigns}");
                            printf("0-1 {White resigns}\n");

                        }
                        else
                        {
                            res = 'b';
                            strcpy(ultimo_resultado, "1-0 {Black resigns}");
                            printf("1-0 {Black resigns}\n");
                        }
                    }
                    else
                    {
                        printf("# xadreco : Playing a random move (main)!\n");
                        res = joga_em(&tabu, *result.plance, 1);
                    }
                }
//                primeiro = segundo = 'h';
                break;
            //            continue;
            case 'x': //xeque: joga novamente
            case 'y': //retorna y: simplesmente gira o laco para jogar de novo. Troca de adv.
            default: //'-' para tudo certo...
//                primeiro = segundo = 'h';
                break;
        } //fim do switch(res)
    } //while (TRUE)

    msgsai("# out of main loop...\n", 0);  //vai apenas para o log, mas nao para a saida
    return EXIT_SUCCESS;
}


void inicia_fics(void)
{
    waits(10); /* bug: trocar por espera que nao prende a engine */
    printfics("tellicsnoalias tell mamer gettourney blitz\n");
    printfics("tellicsnoalias resume\n");
    printfics("tellicsnoalias seek 2 1 f m\n");
    printfics("tellicsnoalias seek 5 5 f m\n");
    printfics("tellicsnoalias seek 10 10 f m\n");
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
    printf("%3d %+6d %3d %7d ", nivel, result.valor, (int)difclocks(), totalnodo);
    //result.valor*10 o xboard divide por 100. centi-peao
//    if (debug) printf ("# xadreco : %d %+.2f %d %d ", nivel, result.valor / 100.0, (int)difclocks(), totalnodo);
    //result.valor/100 para a Dama ficar com valor 9
    imprime_linha(result.plance, tabu.numero, tabu.vez);
//    fflush(stdout);
}

//fim do mostra_lances
void libera_lances(movimento *cabeca)
{
    movimento *loop, *aux;
    if(cabeca == NULL)
        return;
    loop = cabeca;
    while(loop != NULL)
    {
        aux = loop->prox;
        free(loop);
        loop = aux;
    }
//     cabeca = NULL;
}

// imprime o movimento
// funcao intermediaria chamada no intervalo humajoga / 'ga
void imptab(tabuleiro tabu)
{
//     int col, lin, casacor;
    //nao precisa se tem textbackground
//     int colmax, colmin, linmax, linmin, inc;
    char movinito[80];
    double razao;
    movinito[79] = '\0';
    lance2movi(movinito, tabu.lancex, tabu.especial);
    //imprime o movimento
    if((tabu.vez == brancas && segundo == 'c') || (tabu.vez == pretas && primeiro == 'c'))
    {
        if(debug)
        {
//            difclock = clock2 - clock1;
            tatual = time(NULL); //atualiza, pois a funcao difclocks() a atualiza tambem e pode ficar errada a conta
//            printf ("# xadreco : move %s (%ds)\n", movinito, (int)difclocks());
        }
        printf("move %s\n", movinito);
    }
    // estimar tempo de movimento em segundos:
    if(tpretasac > 5.0 && tbrancasac > 5.0)
    {
        if(primeiro == 'c') //computador brancas
            razao = (tpretasac / (tabu.numero / 2.0)) / (tbrancasac / (tabu.numero / 2.0));
        else //computador pretas
            razao = (tbrancasac / (tabu.numero / 2.0)) / (tpretasac / (tabu.numero / 2.0));
        tempomovclock *= razao;
        if(tempomovclock > tempomovclockmax)
            tempomovclock = tempomovclockmax; //maximo tempomovclockmax
        if(tempomovclock < 2) //minimo 1 segundo
            tempomovclock = 2; /* evita ficar sem lances */

//        printf ("# xadreco : tempomovclock=%f\n", tempomovclock);
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
            dest->tab[i][j] = font->tab[i][j];
    dest->vez = font->vez;
    dest->peao_pulou = font->peao_pulou;
    dest->roqueb = font->roqueb;
    dest->roquep = font->roquep;
    dest->empate_50 = font->empate_50;
    for(i = 0; i < 4; i++)
        dest->lancex[i] = font->lancex[i];
    dest->numero = font->numero;
    dest->especial = font->especial;
    //0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant.
    //promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
}

// se nmovi==-1 retorna verdadeiro (ponteiro para o primeiro lance) / falso NULL
movimento *geramov(tabuleiro tabu, int *nmovi)
{
    tabuleiro tabaux;
    movimento *pmovi = NULL, *pmoviant = NULL, *cabeca = NULL;
    int i, j, k, l, m, flag;
    int col, lin;
    int peca;
    int flagvf = *nmovi; /* flagvf==AFOGOU, retorna unico lance (saber se esta afogado), 0, retorna todos */
//     int pp; //peao pulou
    int rr, ee, ff; //peao pulou, roque, especial e flag50;

    for(i = 0; i < 8; i++)  //#coluna
        for(j = 0; j < 8; j++)  //#linha
        {
            if((tabu.tab[i][j]) * (tabu.vez) <= 0)  //casa vazia, ou cor errada:
                continue; // (+)*(-)==(-) , (-)*(+)==(-)
            peca = abs(tabu.tab[i][j]);
            switch(peca)
            {
                case REI:
                    for(col = i - 1; col <= i + 1; col++)  //Rei anda normalmente
                        for(lin = j - 1; lin <= j + 1; lin++)
                        {
                            if(lin < 0 || lin > 7 || col < 0 || col > 7)
                                continue; //casa de indice invalido
                            if(sinal(tabu.tab[col][lin]) == tabu.vez)
                                continue; //casa possui peca da mesma cor ou o proprio rei
                            if(ataca(adv(tabu.vez), col, lin, tabu))
                                continue; //adversario esta protegendo a casa
                            copitab(&tabaux, &tabu);
                            tabaux.tab[i][j] = VAZIA;
                            tabaux.tab[col][lin] = REI * tabu.vez;
                            if(!xeque_rei_das(tabu.vez, tabaux))
                            {
                                //nao pode mais fazer roque. Mexeu Rei
                                if(tabu.tab[col][lin] == VAZIA)
                                    ff = 0; //nao capturou
                                else
                                    ff = 2; //Rei capturou peca adversaria
                                // enche_pmovi (movimento **cabeca, movimento **pmovi, int c0, int c1, int c2, int c3, int peao_pulou, int roque, int especial, int flag50, nummovi)
                                //pp contem -1 ou coluna do peao que andou duas neste lance
                                //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                //ee 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                enche_pmovi(&cabeca, &pmovi, i, j, col, lin, /*peao*/ -1, /*roque*/0, /*especial*/0, /*flag50*/ff, nmovi);
                                if(flagvf == AFOGOU) //pergunta a geramov se afogou e retorna rapido
                                    return cabeca;
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
                            if(tabu.roqueb != 2 && tabu.tab[7][0] == -TORRE)
                            {
                                //Nao mexeu TR (e ela esta la Adv poderia ter comido antes de mexer)
                                // roque pequeno
                                if(tabu.tab[5][0] == VAZIA && tabu.tab[6][0] == VAZIA)  //f1,g1
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
                                        enche_pmovi(&cabeca, &pmovi, 4, 0, 6, 0, /*pp*/ -1, /*rr*/0, /*ee*/1, /*ff*/0, nmovi); //BUG flag50 zera no roque? consultar FIDE
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;

                                    }
                                } // roque grande
                            } //mexeu TR
                            if(tabu.roqueb != 3 && tabu.tab[0][0] == -TORRE)  //Nao mexeu TD (e a torre existe!)
                            {
                                if(tabu.tab[1][0] == VAZIA && tabu.tab[2][0] == VAZIA && tabu.tab[3][0] == VAZIA)  //b1,c1,d1
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
                                        enche_pmovi(&cabeca, &pmovi, 4, 0, 2, 0, /*peao*/ -1, /*roque*/0, /*especial*/2, /*flag50*/0, nmovi);
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;
                                    }
                                }
                            } //mexeu TD
                        }
                    else // vez das pretas jogarem
                        if(tabu.roquep == 0)  //Se nao pode rocar: break!
                            break;
                        else //roque de pretas
                        {
                            if(tabu.roquep != 2 && tabu.tab[7][7] == TORRE)  //Nao mexeu TR (e a torre nao foi capturada)
                            {
                                // roque pequeno
                                if(tabu.tab[5][7] == VAZIA && tabu.tab[6][7] == VAZIA)  //f8,g8
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
                                        enche_pmovi(&cabeca, &pmovi, 4, 7, 6, 7, /*peao*/ -1, /*roque*/0, /*especial*/1, /*flag50*/0, nmovi);
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;
                                    }
                                } // roque grande.
                            } //mexeu TR
                            if(tabu.roquep != 3 && tabu.tab[0][7] == TORRE)  //Nao mexeu TD (e a torre esta la)
                            {
                                if(tabu.tab[1][7] == VAZIA && tabu.tab[2][7] == VAZIA && tabu.tab[3][7] == VAZIA)  //b8,c8,d8
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
                                        enche_pmovi(&cabeca, &pmovi, 4, 7, 2, 7, /*peao*/ -1, /*roque*/0, /*especial*/2, /*flag50*/0, nmovi);
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;
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
                                    tabaux.tab[tabu.peao_pulou][5] = -PEAO;
                                    tabaux.tab[tabu.peao_pulou][4] = VAZIA;
                                    tabaux.tab[i][4] = VAZIA;
                                    if(!xeque_rei_das(brancas, tabaux))   //nao deixa rei branco em xeque
                                    {
                                        //                                  enche_pmovi (pmovi, i, j, tabu.peao_pulou, 5, -1, 1, 3, 3);
                                        enche_pmovi(&cabeca, &pmovi, i, j, tabu.peao_pulou, 5, /*peao*/ -1, /*roque*/1, /*especial*/3, /*flag50*/3, nmovi);
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;
                                    }
                                }
                            }
                            else // vez das pretas.
                            {
                                if(j == 3)  // Esta na linha certa. Peao preto vai comer enpassant!
                                {
                                    tabaux.tab[tabu.peao_pulou][2] = PEAO;
                                    tabaux.tab[tabu.peao_pulou][3] = VAZIA;
                                    tabaux.tab[i][3] = VAZIA;
                                    if(!xeque_rei_das(pretas, tabaux))
                                    {
                                        //  enche_pmovi (pmovi, i, j, tabu.peao_pulou, 2, -1, 1, 3, 3);
                                        enche_pmovi(&cabeca, &pmovi, i, j, tabu.peao_pulou, 2, /*peao*/ -1, /*roque*/1, /*especial*/3, /*flag50*/3, nmovi);
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;
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
                            if(tabu.tab[i][j + 1] == VAZIA)
                            {
                                tabaux.tab[i][j] = VAZIA;
                                tabaux.tab[i][j + 1] = -PEAO;
                                if(!xeque_rei_das(brancas, tabaux))
                                {
                                    // enche_pmovi (pmovi, i, j, i, j + 1, -1, 1, 0, 1);
                                    if(j + 1 == 7)  //se promoveu
                                    {
                                        //TODO inverter laco e=4, e<8, ee++
                                        for(ee = 4; ee < 8; ee++) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_pmovi(&cabeca, &pmovi, i, j, i, j + 1, /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1, nmovi);
                                    }
                                    else //nao promoveu
                                        enche_pmovi(&cabeca, &pmovi, i, j, i, j + 1, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/1, nmovi);
                                    if(flagvf == AFOGOU) //se nao afogou retorna
                                        return cabeca;
                                } //deixa rei em xeque
                            } //casa ocupada
                        } //passou da casa de promocao e caiu do tabuleiro!
                    }
                    else //vez das pretas andar com peao uma casa
                    {
                        if(j - 1 > -1)
                        {
                            if(tabu.tab[i][j - 1] == VAZIA)
                            {
                                tabaux.tab[i][j] = VAZIA;
                                tabaux.tab[i][j - 1] = PEAO;
                                if(!xeque_rei_das(pretas, tabaux))
                                {
                                    // enche_pmovi (pmovi, i, j, i, j - 1, -1, 1, 0, 1);
                                    if(j - 1 == 0)  //se promoveu
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_pmovi(&cabeca, &pmovi, i, j, i, j - 1, /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/1, nmovi);
                                    }
                                    else //nao promoveu
                                        enche_pmovi(&cabeca, &pmovi, i, j, i, j - 1, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/1, nmovi);
                                    if(flagvf == AFOGOU) //se nao afogou retorna
                                        return cabeca;
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
                            if(tabu.tab[i][2] == VAZIA && tabu.tab[i][3] == VAZIA)
                            {
                                tabaux.tab[i][1] = VAZIA;
                                tabaux.tab[i][3] = -PEAO;
                                if(!xeque_rei_das(brancas, tabaux))
                                {
                                    // enche_pmovi (pmovi, i, 1, i, 3, i, 1, 0, 1);
                                    enche_pmovi(&cabeca, &pmovi, i, 1, i, 3, /*peao*/i, /*roque*/1, /*especial*/0, /*flag50*/1, nmovi);
                                    if(flagvf == AFOGOU) //se nao afogou retorna
                                        return cabeca;
                                } //deixa rei em xeque
                            } //alguma das casas esta ocupada
                        } //este peao ja mexeu antes
                    }
                    else //vez das pretas andar com peao 2 casas
                    {
                        if(j == 6)  //esta na linha inicial
                        {
                            if(tabu.tab[i][5] == VAZIA && tabu.tab[i][4] == VAZIA)
                            {
                                tabaux.tab[i][6] = VAZIA;
                                tabaux.tab[i][4] = PEAO;
                                if(!xeque_rei_das(pretas, tabaux))
                                {
                                    //                                 enche_pmovi (pmovi, i, 6, i, 4, i, 1, 0, 1);
                                    enche_pmovi(&cabeca, &pmovi, i, 6, i, 4, /*peao*/i, /*roque*/1, /*especial*/0, /*flag50*/1, nmovi);
                                    if(flagvf == AFOGOU) //se nao afogou retorna
                                        return cabeca;
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
                            if(tabu.tab[k][j + 1] > 0)
                            {
                                tabaux.tab[i][j] = VAZIA;
                                tabaux.tab[k][j + 1] = -PEAO;
                                if(!xeque_rei_das(brancas, tabaux))
                                {
                                    // enche_pmovi (pmovi, i, j, k, j + 1, -1, 1, 0, flag50=3);
                                    if(j + 1 == 7)  //se promoveu comendo
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_pmovi(&cabeca, &pmovi, i, j, k, j + 1, /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3, nmovi);
                                    }
                                    else //comeu sem promover
                                        enche_pmovi(&cabeca, &pmovi, i, j, k, j + 1, /*peao*/ -1, /*roque*/1, /*especial*/0, /*flag50*/3, nmovi);
                                    if(flagvf == AFOGOU) //se nao afogou retorna
                                        return cabeca;
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
                            if(tabu.tab[k][j - 1] < 0)  //peca branca
                            {
                                tabaux.tab[i][j] = VAZIA;
                                tabaux.tab[k][j - 1] = PEAO;
                                if(!xeque_rei_das(pretas, tabaux))
                                {
                                    //4:dama, 5:cavalo, 6:torre, 7:bispo
                                    //peao preto promove comendo
                                    //                                 enche_pmovi (pmovi, i, j, k, j - 1, -1, 1, 0, 3);
                                    //                                 (*nmovi)++;
                                    if(j - 1 == 0)  //se promoveu comendo
                                    {
                                        for(ee = 7; ee >= 4; ee--) //4:dama, 5:cavalo, 6:torre, 7:bispo
                                            enche_pmovi(&cabeca, &pmovi, i, j, k, j - 1, /*peao*/ -1, /*roque*/1, /*especial*/ ee, /*flag50*/3, nmovi);
                                    }
                                    else //comeu sem promover
                                        enche_pmovi(&cabeca, &pmovi, i, j, k, j - 1, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/3, nmovi);
                                    if(flagvf == AFOGOU) //se nao afogou retorna
                                        return cabeca;
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
                            if(sinal(tabu.tab[i + col][j + lin]) == tabu.vez)
                                continue; //casa possui peca da mesma cor.
                            copitab(&tabaux, &tabu);
                            tabaux.tab[i][j] = VAZIA;
                            tabaux.tab[i + col][j + lin] = CAVALO * tabu.vez;
                            if(!xeque_rei_das(tabu.vez, tabaux))
                            {
                                //                             enche_pmovi (pmovi, i, j, col + i, lin + j, -1, 1, 0, 2);
                                if(tabu.tab[col + i][lin + j] == VAZIA)
                                    ff = 0; //Cavalo nao capturou
                                else
                                    ff = 2; // Cavalo capturou peca adversaria.
                                enche_pmovi(&cabeca, &pmovi, i, j, col + i, lin + j, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/ff, nmovi);
                                if(flagvf == AFOGOU) //se nao afogou retorna
                                    return cabeca;
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
                                if(col >= 0 && col <= 7 && sinal(tabu.tab[col][j]) != tabu.vez && l == 0)   //gira col, mantem lin
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[i][j] = VAZIA;
                                    tabaux.tab[col][j] = peca * tabu.vez;
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
                                        if(tabu.tab[col][j] != VAZIA)
                                        {
                                            ff = 2; //Dama ou Torre capturou peca adversaria.
                                            l = 1;
                                        }
                                        // enche_pmovi (pmovi, i, j, col, j, -1, 1, 0, 0);
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        enche_pmovi(&cabeca, &pmovi, i, j, col, j, /*peao*/ -1, /*roque*/rr, /*especial*/ 0, /*flag50*/ff, nmovi);
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;
                                    }
                                    else //deixa rei em xeque
                                        if(tabu.tab[col][j] != VAZIA)
                                            //alem de nao acabar o xeque, nao pode mais seguir nessa direcao pois tem peca
                                            l = 1;
                                }
                                else
                                    //nao inclui mais nenhuma casa desta linha; A casa esta fora do tabuleiro, ou tem peca de mesma cor ou capturou
                                    l = 1;
                                if(lin >= 0 && lin <= 7 && sinal(tabu.tab[i][lin]) != tabu.vez && m == 0)   //gira lin, mantem col
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[i][j] = VAZIA;
                                    tabaux.tab[i][lin] = peca * tabu.vez;
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
                                        if(tabu.tab[i][lin] != VAZIA)
                                        {
                                            ff = 2; //Dama ou Torre capturou peca adversaria.
                                            m = 1;
                                        }
                                        //                                     enche_pmovi (pmovi, i, j, i, lin, -1, 1, 0, 0);
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        enche_pmovi(&cabeca, &pmovi, i, j, i, lin, /*peao*/ -1, /*roque*/rr, /*especial*/ 0, /*flag50*/ff, nmovi);
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;
                                    }
                                    else //deixa rei em xeque
                                        if(tabu.tab[i][lin] != VAZIA)
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
                                while(col >= 0 && col <= 7 && lin >= 0 && lin <= 7 && sinal(tabu.tab[col][lin]) != tabu.vez && flag == 0)
                                {
                                    //inclui esta casa na lista
                                    copitab(&tabaux, &tabu);
                                    tabaux.tab[i][j] = VAZIA;
                                    tabaux.tab[col][lin] = peca * tabu.vez;
                                    if(!xeque_rei_das(tabu.vez, tabaux))
                                    {
                                        //enche_pmovi(pmovi, c0, c1, c2, c3, peao, roque, espec, flag);
                                        //preenche a estrutura movimento
                                        //                                     if ((*nmovi) == -1)
                                        //                                         return (movimento *) TRUE;
                                        ff = 0; //flag50 nao altera
                                        if(tabu.tab[col][lin] != VAZIA)
                                        {
                                            ff = 2; //Dama ou Bispo capturou peca adversaria.
                                            flag = 1;
                                        }
                                        // enche_pmovi (pmovi, i, j, col, lin, -1, 1, 0, 0);
                                        //pp contem -1 ou coluna do peao que andou duas neste lance
                                        //rr 0:mexeu rei. 1:ainda pode. 2:mexeu TR. 3:mexeu TD.
                                        //ee 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant. promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
                                        //ff :0=nada,1=Moveu peao,2=Comeu,3=Peao Comeu. Zera empate_50;
                                        enche_pmovi(&cabeca, &pmovi, i, j, col, lin, /*peao*/ -1, /*roque*/1, /*especial*/ 0, /*flag50*/ff, nmovi);
                                        if(flagvf == AFOGOU) //se nao afogou retorna
                                            return cabeca;
                                    }
                                    else //deixa rei em xeque
                                        if(tabu.tab[col][lin] != VAZIA)  //alem de nao acabar o xeque, nao pode mais seguir nessa direcao pois tem peca
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

    if(flagvf == AFOGOU) //pergunta a geramov se afogou e retorna rapido
        return cabeca; //nao retornou ate aqui, cabeca==NULL
    if(cabeca == NULL)  //se a lista esta vazia
        //free (cabeca); /* core dump BUG */
        return NULL;
    //ordena e retorna! flag_50: 0-nada, 1-peao andou, 2-captura, 3-peao capturou
    // especial: 0-nada, roque 1-pqn, 2-grd, 3-enpassant
    // 4,5,6,7 promocao de Dama, Cavalo, Torre, Bispo
//         free (pmovi);
//         pmoviant->prox = NULL; //aloca e desaloca o ultimo. desperdicio de tempo.
    pmoviant = cabeca;
    pmovi = cabeca->prox;
    while(pmovi != NULL)  //percorre, passa para cabeca tudo de interessante.
    {
        //especial 0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant.
        //especial promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
        //flag_50 :0=nada, 1=Moveu peao, 2=Comeu, 3=Peao Comeu
        if(pmovi->flag_50 > 1 || pmovi->especial)  //achou captura ou especial
        {
            //move para a cabeca da lista!
            pmoviant->prox = pmovi->prox; //o anterior aponta para o prox.
            pmovi->prox = cabeca; //o prox do especial aponta para o primeiro
            cabeca = pmovi; //o cabeca agora eh pmovi!
            pmovi = pmoviant->prox;
        }
        else
        {
            // faz nada... anda ponteiros para os proximos
            pmoviant = pmovi;
            pmovi = pmovi->prox;
        }
    } //fim do while, e consequentemente da lista
    return cabeca; //lista criada: quem chamou que a libere
}
//fim de   ----------- movimento *geramov(tabuleiro tabu)

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
        if(tabu.tab[icol][lin] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[icol][lin] == -TORRE || tabu.tab[icol][lin] == -DAMA)
                return (1);
            else
                break;
        else
            // pretas atacam a casa
            if(tabu.tab[icol][lin] == TORRE || tabu.tab[icol][lin] == DAMA)
                return (1);
            else
                break;
    }
    for(icol = col + 1; icol < 8; icol++)
        //sobe coluna
    {
        if(tabu.tab[icol][lin] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[icol][lin] == -TORRE || tabu.tab[icol][lin] == -DAMA)
                return (1);
            else
                break;
        else
            if(tabu.tab[icol][lin] == TORRE || tabu.tab[icol][lin] == DAMA)
                return (1);
            else
                break;
    }
    for(ilin = lin + 1; ilin < 8; ilin++)
        // direita na linha
    {
        if(tabu.tab[col][ilin] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[col][ilin] == -TORRE || tabu.tab[col][ilin] == -DAMA)
                return (1);
            else
                break;
        else
            if(tabu.tab[col][ilin] == TORRE || tabu.tab[col][ilin] == DAMA)
                return (1);
            else
                break;
    }
    for(ilin = lin - 1; ilin >= 0; ilin--)
        // esquerda na linha
    {
        if(tabu.tab[col][ilin] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[col][ilin] == -TORRE || tabu.tab[col][ilin] == -DAMA)
                return (1);
            else
                break;
        else
            if(tabu.tab[col][ilin] == TORRE || tabu.tab[col][ilin] == DAMA)
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
                if(tabu.tab[col + icol][lin + ilin] == -CAVALO)
                    return (1);
                else
                    continue;
            else
                if(tabu.tab[col + icol][lin + ilin] == CAVALO)
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
            while(tabu.tab[casacol][casalin] == VAZIA);

            if(casacol >= 0 && casacol <= 7 && casalin >= 0 && casalin <= 7)
            {
                if(cor == brancas)
                {
                    if(tabu.tab[casacol][casalin] == -BISPO
                            || tabu.tab[casacol][casalin] == -DAMA)
                        return 1;
                    else
                        continue;
                }
                else // achou peca, mas esta nao anda em diagonal ou e' peca propria
                    if(tabu.tab[casacol][casalin] == BISPO || tabu.tab[casacol][casalin] == DAMA)
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
                if(tabu.tab[icol][ilin] == -REI)
                    return (1);
                else
                    continue;
            else
                if(tabu.tab[icol][ilin] == REI)
                    return (1);
        }
    if(cor == brancas)
        // ataque de peao branco
    {
        if(lin > 1)
        {
            ilin = lin - 1;
            if(col - 1 >= 0)
                if(tabu.tab[col - 1][ilin] == -PEAO)
                    return (1);
            if(col + 1 <= 7)
                if(tabu.tab[col + 1][ilin] == -PEAO)
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
                if(tabu.tab[col - 1][ilin] == PEAO)
                    return (1);
            if(col + 1 <= 7)
                if(tabu.tab[col + 1][ilin] == PEAO)
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
            if(tabu.tab[icol][ilin] == (REI * cor))                  //achou o rei
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
    movimento *pval = 0;
    char res;
    //    char aux;
    //     char peca;
    int tente;
    int lanc[4], moves, minutes;
    double secs, incre;
    int i, j, k;
    //     int casacor;
    //nao precisa se tem textbackground
    //     listab *plaux;
    char pieces[80], color[80], castle[80], enpassant[80], halfmove[80], fullmove[80]; //para testar uma posicao
    int testasim = 0, estat;
    movinito[79] = movinito[0] = '\0';

    do
    {
        tente = 0;
        scanf("%s", movinito);	//---------------- (*quase) Toda entrada do xboard
//        if (debug) printf ("# xboard: %s\n", movinito);

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
            printf("# xboard: force. Xadreco is in force mode.\n");
            continue;
        }
        if(!strcmp(movinito, "undo"))   // volta um movimento (ply)
        {
            primeiro = 'h';
            segundo = 'h';
            volta_lance(tabu);
            tente = 1;
            printf("# xboard: undo. Back one ply. Xadreco is in force mode.\n");
            continue;
        }
        if(!strcmp(movinito, "remove"))   // volta dois movimentos == 1 lance
        {
            volta_lance(tabu);
            volta_lance(tabu);
            tente = 1;
            printf("# xboard: remove. Back one move.\n");
            continue;
        }
        if(!strcmp(movinito, "post"))   //showthinking ou mostrapensando
        {
            analise = 0;
            mostrapensando = 1;
            tente = 1;
            printf("# xboard: post. Xadreco will show what its thinking. (2)\n");
            continue;
        }
        if(!strcmp(movinito, "nopost"))   //no show thinking ou desliga o mostrapensando
        {
            mostrapensando = 0;
            tente = 1;
            printf("# xboard: nopost. Xadreco will not show what its thinking.\n");
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
            printf("# xboard: analyze. Xadreco starts analyzing in force mode.\n");
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
            printf("# xboard: exit. Xadreco stops analyzing.\n");
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
                    if(debug)  //bug: eu era branca, e ele tinha dama aceitou empate.
                        printf("# xadreco : draw accepted (HumaJ-1) %d  turn: %d\n", estat, tabu->vez);
                    return ('c');
                }
                else
                {
                    if(debug)
                        printf("# xadreco : draw rejected (HumaJ-2) %d  turn: %d\n", estat, tabu->vez);
                    tente = 1;
                    continue;
                }
            }
        }
        if(!strcmp(movinito, "version"))
        {
//            if (debug) printf ("# tellopponent Xadreco v.%.2f Compilacao %f para XBoard/WinBoard, baseado no Algoritmo Minimax, por Ruben Carlo Benante, 22/10/04.\n", version, build());
            printfics("tellopponent Xadreco v%s build %s for XBoard/WinBoard, based on Minimax Algorithm, by Ruben Carlo Benante, 1994-2018.\n", VERSION, BUILD);
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "sd"))   //set deep: coloca o nivel de profundidade. Nao esta sendo usado. Implementar.
        {
            nivel = (int)(movinito[3] - '0');
            nivel = (nivel > 6 || nivel < 0) ? 2 : nivel;
            tente = 1;
            printf("# xboard: sd. Xadreco is set deep %d\n", nivel);
            continue;
        }
        if(!strcmp(movinito, "go"))   //troca de lado e joga. (o mesmo que "adv")
        {
            if(tabu->vez == brancas)
            {
                primeiro = 'c';
                segundo = 'h';
                printf("# xboard: go. Xadreco is now white.\n");
            }
            else
            {
                primeiro = 'h';
                segundo = 'c';
                printf("# xboard: go. Xadreco is now black.\n");
            }
            return ('y'); //retorna para jogar ou humano ou computador.
        }
        if(!strcmp(movinito, "playother"))   //coloca o computador com a cor que nao ta da vez. (switch)
        {
            if(tabu->vez == brancas)
            {
                primeiro = 'h';
                segundo = 'c';
                printf("# xboard: playother. Xadreco is now black.\n");
            }
            else
            {
                primeiro = 'c';
                segundo = 'h';
                printf("# xboard: playother. Xadreco is now white.\n");
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
            printf("# xboard: computer. Xadreco now knows its playing against another engine.\n");
            continue;
        }
        if(!strcmp(movinito, "new"))   //novo jogo
        {
            printf("# xboard: new. Xadreco sets the board in initial position.\n");
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
            scanf("%s", movinito);
            myrating = atoi(movinito);
            scanf("%s", movinito);
            opprating = atoi(movinito);
            printf("# xboard: myrating: %d opprating: %d\n", myrating, opprating);
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
        if(!strcmp(movinito, "level"))
        {
            scanf("%s", movinito);
            moves = atoi(movinito);
            scanf("%s", movinito);
            minutes = atoi(movinito);
            scanf("%s", movinito);
            incre = atof(movinito);
            if(moves <= 0)
                moves = TOTAL_MOVIMENTOS;
            if(minutes <= 0)
                minutes = 0;
            if(incre <= 0)
                incre = 0;
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
            if(minutes == 0)
            {
                if(incre != 0.0)
                    secs = 10.0; /* no start time, use 10s */
                else //minutes == 0 && incre == 0; untimed
                    secs = 30.0 * 60.0; /* no time, use 30 min */
            }
            else //minutes !=0
                secs = minutes * 60.0;
            tempomovclockmax = secs / (float)moves + incre; //em segundos
            tempomovclock = tempomovclockmax; //+90)/2;

            printdbg(debug, "# xadreco : %.1fs+%.1fs por %d lances: ajustado para st %f s por lance\n", secs, incre, moves, tempomovclock);
            tente = 1;
            continue;
        }
        if(!strcmp(movinito, "setboard"))   //funciona no prompt tambem
        {
            printf("# xboard: setboard. Xadreco will set a board position.\n");
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
            scanf("%s", movinito);  //trava se colocar uma posicao FEN invalida!
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
                    msgsai("# Posicao FEN incorreta.", 24);  //em vez de msg sai, printf("Error (Wrong FEN %s): setboard", movinito);
                switch(movinito[k])  //KkQqRrBbNnPp
                {
                    case 'K':
                        tabu->tab[i][j] = -REI;
                        i++;
                        break;
                    case 'k':
                        tabu->tab[i][j] = REI;
                        i++;
                        break;
                    case 'Q':
                        tabu->tab[i][j] = -DAMA;
                        i++;
                        break;
                    case 'q':
                        tabu->tab[i][j] = DAMA;
                        i++;
                        break;
                    case 'R':
                        tabu->tab[i][j] = -TORRE;
                        i++;
                        break;
                    case 'r':
                        tabu->tab[i][j] = TORRE;
                        i++;
                        break;
                    case 'B':
                        tabu->tab[i][j] = -BISPO;
                        i++;
                        break;
                    case 'b':
                        tabu->tab[i][j] = BISPO;
                        i++;
                        break;
                    case 'N':
                        tabu->tab[i][j] = -CAVALO;
                        i++;
                        break;
                    case 'n':
                        tabu->tab[i][j] = CAVALO;
                        i++;
                        break;
                    case 'P':
                        tabu->tab[i][j] = -PEAO;
                        i++;
                        break;
                    case 'p':
                        tabu->tab[i][j] = PEAO;
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
                scanf("%s", movinito);
            if(debug)
                printf("# xboard: %s\n", movinito);
            if(movinito[0] == 'w')
                tabu->vez = brancas;
            else
                tabu->vez = pretas;
            //int roqueb, roquep: 1:pode para os dois lados. 0:nao pode mais. 3:mexeu TD. So pode K. 2:mexeu TR. So pode Q
            if(testasim)
                strcpy(movinito, castle);
            else
                scanf("%s", movinito);  //Roque
            if(debug)
                printf("# xboard: %s\n", movinito);
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
                scanf("%s", movinito);  //En passant
            if(debug)
                printf("# xboard: %s\n", movinito);
            tabu->peao_pulou = -1; //nao pode
            if(!strchr(movinito, '-'))   //alguem pode
                tabu->peao_pulou = movinito[0] - 'a'; //pulou 2 na coluna dada
            if(testasim)
                strcpy(movinito, halfmove);
            else
                scanf("%s", movinito);  //Num. de movimentos (ply)
            printdbg(debug, "# xboard: %s\n", movinito);
            tabu->empate_50 = atoi(movinito);  //contador:quando chega a 50, empate.
            if(testasim)
                strcpy(movinito, fullmove);
            else
                scanf("%s", movinito);  //Num. de lances
            printdbg(debug, "# xboard: %s\n", movinito);
            tabu->numero = 0; //mudou para ply.
            //(atoi(movinito)-1)+0.3;
            //inicia no 0.3 para indicar 0
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
            USALIVRO = 0; // Baseado somente nos movimentos, nao na posicao.
            imptab(*tabu);
            insere_listab(*tabu);
            tente = 1;
            continue;
        }
        /*		if(!strcmp (movinito, "ping"))
        		{
                    scanf ("%s", movinito);
                    printf("pong %s",movinito);
                    if (debug)
        	            printf ("# pong %s\n", movinito);
                    tente = 1;
                    continue;
        		}*/
        if(movinito[0] == '{')
        {
            char completo[256];
            strcpy(completo, movinito);
            while(!strchr(movinito, '}'))
            {
                scanf("%s", movinito);
                strcat(completo, " ");
                strcat(completo, movinito);
            }
            if(debug)
                printf("# xboard: %s\n", completo);
            tente = 1;
            continue;
        }

        if(!strcmp(movinito, "ping") || !strcmp(movinito, "san")
                || !strcmp(movinito, "usermove") || !strcmp(movinito, "time")
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
            printf("# xboard: ignoring a valid command %s\n", movinito);
            continue;
        }
        if(!strcmp(movinito, "t"))
            testajogo(movinito, tabu->numero);
        //retorna um lance do jogo de teste (em movinito)
        //e vai la embaixo jogar ele
        if(!movi2lance(lanc, movinito))   //se nao existe este lance
        {
//            if (debug) printf ("# xadreco : Error (unknown command): %s\n", movinito);
            printf("Error (unknown command): %s\n", movinito);
            tente = 1;
            continue;
        }
        pval = valido(*tabu, lanc);  //lanc eh int lanc[4]; cria pval com tudo preenchido
        if(pval == NULL)
        {
//            if (debug) printf ("# xadreco : Illegal move: %s\n", movinito);
            printf("Illegal move: %s\n", movinito);
            tente = 1;
            continue;
        }
    }
    while(tente);    // BUG checar tempo dentro do laco e dar abort se demorar, ou flag...

    if(pval->especial > 3)  //promocao do peao 4,5,6,7: Dama, Cavalo, Torre, Bispo
    {
        switch(movinito[4])  //exemplo: e7e8q
        {
            case 'q': //dama
                pval->especial = 4;
                break;
            case 'n': //cavalo
                pval->especial = 5;
                break;
            case 'r': //torre
                pval->especial = 6;
                break;
            case 'b': //bispo
                pval->especial = 7;
                break;
            default: //se erro, vira dama
                pval->especial = 4;
                break;
        }
    }
    //joga_em e calcula tempo //humano joga
    res = joga_em(tabu, *pval, 1);

    //a funcao joga_em deve inserir no listab cod==1
    free(pval);
    if(analise == 1)  // analise outro movimento
        disc = analisa(tabu);
    return (res);
    //vez da outra cor jogar. retorna a situacao.
} //fim do huma_joga---------------

movimento *valido(tabuleiro tabu, int *lanc)
{
    int nmov;
    movimento *pmovi, *loop, *auxloop;
    nmov = 0; //gerar todos
    pmovi = geramov(tabu, &nmov);  // gerou e sobrou um
    auxloop = NULL;

    loop = pmovi;
    while(pmovi != NULL)  //vai comparando e...
    {
        if(igual(pmovi->lance, lanc))   //o lance valido eh o cabeca da lista que restou
            break;
        pmovi = pmovi->prox;
        free(loop);  //...liberando os diferentes
        loop = pmovi;
    }
    //apagar a lista que restou da memoria
    if(pmovi != NULL)  //o lance esta na lista, logo eh valido,
    {
        loop = pmovi->prox; //apaga o restante da lista.
        pmovi->prox = NULL; // pmovi vira lista de um so lance
    }

    //ou loop==pmovi==NULL e o lance nao vale
    //ou loop==pmovi->prox e apaga o restante
    while(loop != NULL)
    {
        auxloop = loop->prox;
        free(loop);
        loop = auxloop;
    }
    return (pmovi);
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
    movimento *cabeca;
    int insuf_branca = 0, insuf_preta = 0, nmov;
    int i, j;
    listab *plaux;
    if(tabu.empate_50 >= 50.0)  //empate apos 50 lances sem captura ou movimento de peao
        return ('5');
    for(i = 0; i < 8; i++)  //insuficiencia de material
        for(j = 0; j < 8; j++)
        {
            switch(tabu.tab[i][j])
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
    plaux = plfinal; //posicao repetiu 3 vezes
    while(plaux != NULL)
    {
        if(plaux->rep == 3)
            return ('r');
        plaux = plaux->ant;
    }
    nmov = AFOGOU; //somente achar UM LANCE e retornar rapido
    cabeca = geramov(tabu, &nmov);  //TODO criar funcao gera1mov() retorna verdadeiro ou falso: quando nmov == -1, geramov retorna no primeiro valido
    if(cabeca == NULL)  //Sem lances: Mate ou Afogamento.
    {
        if(!xeque_rei_das(tabu.vez, tabu))
            return ('a'); //empate por afogamento.
        else
            if(tabu.vez == brancas)
                return ('M'); //as brancas estao em xeque-mate
            else
                return ('m'); //as pretas estao em xeque-mate
    }
    else //Tem lances... mudou a geramov: tem um lance so: nmovi==AFOGOU
    {
        libera_lances(cabeca); //libera o lance
        if(xeque_rei_das(tabu.vez, tabu))
            return ('x');
    }
    return ('-'); //nada
}

// ------------------------------- jogo do computador -----------------------
char compjoga(tabuleiro *tabu)
{
    char res;
    int i;
    int nv = 1, nmov;
    int melhorvalor1;
    // lances calc. em maior nivel tem mais importancia?
    movimento *melhorcaminho1;
    int moveto;
    movimento *succ = NULL, *melhor_caminho = NULL;
    limpa_pensa();  //limpa algumas variaveis para iniciar a ponderacao
    melhorvalor1 = -LIMITE;
    melhorcaminho1 = NULL;
    if(debug == 2)  //nivel extra de debug
    {
        fmini = fopen("minimax.log", "w");
        if(fmini == NULL)
            debug = 1;
    }
    if(USALIVRO && tabu->numero < 52 && setboard != 1 && !randomchess)
    {
        usalivro(*tabu);
        if(result.plance == NULL)
            USALIVRO = 0;
        libera_lances(melhorcaminho1);
        melhorcaminho1 = copilistmov(result.plance);
        melhorvalor1 = result.valor;
    }
    if(result.plance == NULL)
    {
        //mudou para busca em amplitude: variavel nivel sem uso. Implementar "sd n"
        nv = 1;
        nmov = 0; // zero= retorna todos validos
        libera_lances(succ_geral);
        succ_geral = geramov(*tabu, &nmov);  //gera os sucessores
        totalnodo = 0;
        //funcao COMPJOGA (CONFERIR)
        if(randomchess)
        {
            succ = succ_geral;
            result.plance = NULL;
            result.valor = 0;
            moveto = (int)(rand() % nmov);  //sorteia um lance possivel da lista de lances
            for(i = 0; i < moveto; ++i)
                if(succ != NULL)
                    succ = succ->prox; //escolhe este lance como o que sera jogado

            if(succ != NULL)
            {
                melhor_caminho = copimel(*succ, result.plance);
                succ->valor_estatico = 0;
                result.valor = 0;
                result.plance = copilistmov(melhor_caminho);
                libera_lances(melhor_caminho);
                melhorcaminho1 = copilistmov(result.plance);
                melhorvalor1 = result.valor;
            } //else succ!=NULL
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
                    libera_lances(melhorcaminho1);
                    melhorcaminho1 = copilistmov(result.plance);
                    melhorvalor1 = result.valor;
                }
                totalnodo += totalnodonivel;
                ordena_succ(nmov);  //ordena succ_geral
                if(debug == 2)
                {
                    fprintf(fmini, "#\n# result.valor: %+.2f totalnodo: %d\n# result.plance: ", result.valor / 100.0, totalnodo);
                    if(!mostrapensando || abs(result.valor) == FIMTEMPO || abs(result.valor) == LIMITE)
                        imprime_linha(result.plance, 1, 2);
                    //1=numero do lance, 2=vez: pular impressao na tela.
                    //tabu->numero+1, -tabu->vez);
                }
                //nivel pontuacao tempo totalnodo variacao === usado!
                //nivel tempo valor lances no arena. (nao usado aqui. testar)
                if(mostrapensando && abs(result.valor) != FIMTEMPO && abs(result.valor) != LIMITE)
                {
//                    printf ("# xadreco : ply score time nodes pv\n");
                    printf("%3d %+6d %3d %7d ", nv, result.valor, (int)difclocks(), totalnodo);
                    imprime_linha(result.plance, tabu->numero + 1, -tabu->vez);
//                    if (debug) printf ("# xadreco : nv=%d value=%+.2f time=%ds totalnodo=%d ", nv, (float) result.valor / 100.0, (int)difclocks(), totalnodo);
                }
                // termino do laco infinito baseado no tempo
                if((difclocks() > tempomovclock && debug != 2) || (debug == 2 && nv == 3))   // termino do laco infinito baseado no tempo
                {
                    if(result.plance == NULL)
                    {
                        printf("# compjoga 2898: Sem lances; difclocks()>tempomovclock; result.plance==NULL; (!break);\n");
                        // nv--; /* testar se pode rodar de novo o mesmo nivel e ter lance */
                        // antigo: if(dif) break! retirar o else.
                    }
                    else
                        break;
                }
                nv++; // busca em amplitude: aumenta um nivel.
            } //while result.plance < XEQUEMATE
    } // fim do se nao usou livro
    libera_lances(result.plance);
    result.plance = copilistmov(melhorcaminho1);
    result.valor = melhorvalor1;
    //nivel extra de debug
    if(debug == 2)(void) fclose(fmini);
    //Utilizado: ABANDONA==-2730, alterado quando contra outra engine
    if(result.valor < ABANDONA && ofereci <= 0)
    {
        printdbg(debug, "# xadreco : resign. value: %+.2f\n", result.valor / 100.0);
        --ofereci;
        printf("resign\n");
    }
    //pode oferecer empate duas vezes (caso !randomchess). Uma assim que perder 2 peoes. Outra apos 60 lances e com menos de 2 peoes
//    if(!randomchess)
//    {
//        if (tabu->numero >= MOVE_EMPATE2 && ofereci == 0)
//            ofereci = 1;
//    }
//    else // randomchess
    if(tabu->numero > MOVE_EMPATE1 && ofereci > 0 && randomchess) //posso oferecer empates
    {
        if((int)(rand() % 2)) //sorteio 50% de chances
        {
            printf("offer draw\n");
            --ofereci;
        }
    }

    //oferecer empate: result.valor esta invertido na vez.
    if(result.valor < QUANTO_EMPATE1 && (tabu->numero > MOVE_EMPATE1 && tabu->numero < MOVE_EMPATE2) && ofereci > 0)
    {
        printdbg(debug, "# xadreco : offer draw (1) value: %+.2f\n", result.valor / 100.0);
        --ofereci;
        //atencao: oferecer pode significar aceitar, se for feito logo apos uma oferta recebida.
        printf("offer draw\n");
    }
    //oferecer empate: result.valor esta invertido na vez.
    if(result.valor < QUANTO_EMPATE2 && tabu->numero >= MOVE_EMPATE2 && ofereci > 0)
    {
        printdbg(debug, "# xadreco : offer draw (2) value: %+.2f\n", result.valor / 100.0);
        --ofereci;
        printf("offer draw\n");
    }
    //Nova definicao: sem lances, pode ser que queira avancar apos mate.
    //algum problema ocorreu que esta sem lances
    if(result.plance == NULL)
    {
        res = randommove(tabu);
        if(res == 'e')
            return res; //vazio mesmo! Nem aleatorio foi!
        printf("# xadreco : Error. I don't know what to play... Playing a random move (compjoga)!\n");
    }
    res = joga_em(tabu, *result.plance, 1);  // computador joga
    //vez da outra cor jogar. retorna a situacao(*tabu)

//     if(ics) //conectado ou stand alone
    {
        if(tabu->numero > 4) /* nao mostra nos primeiros lances */
        {
            /*WHISPER = 0:nada, 1:v>200 :)), 2: v>100 :), 3: -100<v<100 :|, 4: v<-100 :(, 5: v<-200 :(( */
            if(result.valor > 200)
            {
                if(WHISPER != 1)
                {
                    printfics("tellicsnoalias whisper :))\n");
                    WHISPER = 1;
                }
            }
            else
                if(result.valor < -200)
                {
                    if(WHISPER != 5)
                    {
                        printfics("tellicsnoalias whisper :((\n");
                        WHISPER = 5;
                    }
                }
                else
                    if(result.valor > 100)
                    {
                        if(WHISPER != 2)
                        {
                            printfics("tellicsnoalias whisper :)\n");
                            WHISPER = 2;
                        }
                    }
                    else
                        if(result.valor < -100)
                        {
                            if(WHISPER != 4)
                            {
                                printfics("tellicsnoalias whisper :(\n");
                                WHISPER = 4;
                            }
                        }
                        else
                            if(WHISPER != 3)
                            {
                                printfics("tellicsnoalias whisper :|\n");
                                WHISPER = 3;
                            }
        }
    }

    return (res);
} //fim da compjoga

char analisa(tabuleiro *tabu)
{
    tabuleiro tanalise;
    int nv = 0, nmov;
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
    if(USALIVRO && tabu->numero < 50 && setboard != 1)
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
        imprime_linha(result.plance, tabu->numero + 1, -tabu->vez);
//        if (debug) printf ("# xadreco : nv=%d value=%+.2f time=%ds totalnodo=%d\n", nv, (float) result.valor / 100.0, (int)difclocks(), totalnodo);
//        if (debug) printf ("# \n");
    }
    else
    {
        nv = 1;
        nmov = 0; //nmove==0, retorna todos validos.
        libera_lances(succ_geral);
        succ_geral = geramov(*tabu, &nmov);  //gera os sucessores
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
            ordena_succ(nmov);
            //nivel pontuacao tempo totalnodo variacao === usado!
            if(abs(result.valor) != FIMTEMPO && abs(result.valor) != LIMITE)
            {
//                printf ("# xadreco : ply score time nodes pv\n");
                printf("%3d %+6d %3d %7d ", nv, result.valor, (int)difclocks(), totalnodo);
//                if (debug) printf ("# xadreco : nv=%d value=%+.2f time=%ds nodos=%d ", nv, (float) result.valor / 100.0, (int)difclocks(), totalnodo);
                imprime_linha(result.plance, tabu->numero + 1, -tabu->vez);
                //1=numero do lance, 2=vez: pular impressao na tela
//                if (debug)
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
                //1=numero do lance, 2=vez: pular impressao na tela.
                // tabu->numero+1, -tabu->vez);
            }
            if((difclocks() > tempomovclock && debug != 2) || (debug == 2 && nv == 3))  // termino do laco infinito baseado no tempo
                break;
            if(result.plance == NULL)  //Nova definicao: sem lances, pode ser que queira avancar apos mate.
                break;
            nv++;
        }
    }
    if(debug == 2)          //nivel extra de debug
        (void) fclose(fmini);
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
    movimento *succ, *melhor_caminho, *cabeca_succ;
    int novo_valor, nmov, contamov = 0;
    tabuleiro tab;
    char m[8];
    succ = NULL;
    melhor_caminho = NULL;
    cabeca_succ = NULL;
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
        cabeca_succ = succ_geral; //copilistmov(succ_geral);
    else
    {
        nmov = 0; //gerar todos
        cabeca_succ = geramov(atual, &nmov);  //nmov==0, retorna todos validos
    }
    //succ==NULL se alguem ganhou ou empatou
    //if (debug == 2) {
    //fprintf(fmini, "\nsucc=geramov(): ");
    //imprime_linha(succ, 1, 2);
    //1=numero do lance, 2=vez: pular impressao na tela
    //}
    if(cabeca_succ == NULL)
    {
        //entao o estatico refletira isso: afogamento
        //libera_lances(result.plance); //bugbug
        result.plance = NULL;
        //coloque um nulo no ponteiro plance
        //nao eh necessario libera_lance, pois plance eh temporario apesar de global.
        result.valor = estatico(atual, 0, prof, alfa, beta);
        //estatico(tabuleiro, 1: acabou o tempo, 0: nao acabou. prof: qual nivel estao analisando?)
        if(debug == 2)
            fprintf(fmini, "#NULL ");	//result.valor=%+.2f", result.valor);
        return;
    }
    succ = cabeca_succ; //laco para analisar todos sucessores (ou corta no porcento)
    while(succ != NULL)
    {
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
        retira_listab();  //retira o ultimo tabuleiro da lista
        novo_valor = -result.valor; //implementar o "random"
        if(novo_valor > alfa)  // || (novo_valor==alfa && rand()%10>7))
        {
            alfa = novo_valor;
            libera_lances(melhor_caminho);
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
                //1=numero do lance, 2=vez: pular impressao na tela
            }
        }
        //implementar o NULL-MOVE
        if(novo_valor >= beta)
            //corte alfa-beta! Isso esta certo? Nao e alfa<=beta? Conferir. (alfa==passo, beta==uso, para brancas)
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
        succ = succ->prox;
        contamov++;
        if(contamov > nmov * PORCENTO_MOVIMENTOS + 1 && prof != 0)	//tentando com 60%
            break;
    } //while(succ!=NULL)
    result.valor = alfa;
    result.plance = copilistmov(melhor_caminho);
    //funcao retorna uma cabeca nova
    libera_lances(melhor_caminho);
    if(prof != 0)
        libera_lances(cabeca_succ);
    //caso contrario, cabeca_succ esta apontando para succ_geral
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
        tabu->tab[movi.lance[0]][movi.lance[1]] = BISPO * tabu->vez;
    if(movi.especial == 6)  //promocao do peao: TORRE
        tabu->tab[movi.lance[0]][movi.lance[1]] = TORRE * tabu->vez;
    if(movi.especial == 5)  //promocao do peao: CAVALO
        tabu->tab[movi.lance[0]][movi.lance[1]] = CAVALO * tabu->vez;
    if(movi.especial == 4)  //promocao do peao: DAMA
        tabu->tab[movi.lance[0]][movi.lance[1]] = DAMA * tabu->vez;
    if(movi.especial == 3)  //comeu en passant
        tabu->tab[tabu->peao_pulou][movi.lance[1]] = VAZIA;
    if(movi.especial == 2)  //roque grande
    {
        tabu->tab[0][movi.lance[1]] = VAZIA;
        tabu->tab[3][movi.lance[1]] = TORRE * tabu->vez;
    }
    if(movi.especial == 1)  //roque pequeno
    {
        tabu->tab[7][movi.lance[1]] = VAZIA;
        tabu->tab[5][movi.lance[1]] = TORRE * tabu->vez;
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
    tabu->tab[movi.lance[2]][movi.lance[3]] =
        tabu->tab[movi.lance[0]][movi.lance[1]];
    tabu->tab[movi.lance[0]][movi.lance[1]] = VAZIA;
    for(i = 0; i < 4; i++)
        tabu->lancex[i] = movi.lance[i];
    tabu->numero++;
    //conta os movimentos de cada peao (e chamado de dentro de funcao recursiva!)
    tabu->vez = adv(tabu->vez);
    tabu->especial = movi.especial;
    if(cod)
        insere_listab(*tabu);  //insere o lance no listab cod==1
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
    cabeca = (movimento *) malloc(sizeof(movimento));   //valor novo que sera do result.plance
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
    int totb = 0, totp = 0;
    int i, j, k, cor, peca;
    int peaob, peaop, isob, isop, pecab = 0, pecap = 0, material;
    int menorb = 0, menorp = 0;
    int qtdb, qtdp;
    int ordem[64][7];
    //coloca todas pecas do tabuleiro em ordem de valor
    //64 casas, cada uma com 7 info: 0:i, 1:j, 2:peca,
    //3: qtdataquebranco, 4: menorb, 5: qtdataquepreto, 6: menorp
    // Estranho de implementar isso. Precisa de mais testes
    // Quando esta avaliando uma posicao e o tempo estourou, por
    // via das duvidas e melhor nao confiar muito nessa posicao...
    //------------------------------------------------------------
    //ignorando a avaliacao estatica, pois precisa retornar
    /*    if (cod)
        {
            k = -1;
            for (i = 0; i < niv; i++)
                k *= (-1);
            return k * FIMTEMPO;	//bugbug
        }*/
    //estando em profsuf, resultado ruim retorna invertido
    //        else
    //              return LIMITE;
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
            //        if (tabu.numero > 40)
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
            peca = tabu.tab[i][j];
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
            peca = tabu.tab[i][j];
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
                if(j > 4 && tabu.numero > 30)
                    totb += 90;
                pecab += peca;
            }
            else
            {
                totb += ordem[k][3] * 20;
                if(j < 3 && tabu.numero > 30)
                    totp += 90;
                pecap += peca;
            }
            k++;
        }
    //acha torres.
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
        {
            peca = tabu.tab[i][j];
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
            peca = tabu.tab[i][j];
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
            peca = tabu.tab[i][j];
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
            peca = tabu.tab[i][j];
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
    material =
        (int)((1.0 + 75.0 / (float)(pecab + pecap)) * (float)(pecab - pecap));
    if(tabu.vez == pretas)
        material = (-material);

    //se estou melhor que a opcao do oponente, retorna a opcao do oponente
    /*    if (material - MARGEM_PREGUICA >= -beta)
        {
            //      if (debug == 2)
            //      fprintf (fmini, "material>=beta (%+.2f >= %+.2f)\n", material / 100.0,
            //               beta / 100.0);
            return (material - MARGEM_PREGUICA);
        }
        //se o que ganhei e menor que minha outra opcao, retorna minha outra opcao
        if (material + MARGEM_PREGUICA <= -alfa)
        {
            //      if (debug == 2)
            //      fprintf (fmini, "material<=alfa (%+.2f >= %+.2f)\n", material / 100.0,
            //               alfa / 100.0);
            return (material + MARGEM_PREGUICA);
        }
    */
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
        tabu.tab[i][j] = VAZIA;
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
        tabu.tab[i][j] = peca;
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
            if(tabu.tab[i][j] == VAZIA)
                continue;
            if(tabu.tab[i][j] == -PEAO)
                peaob++;
            if(tabu.tab[i][j] == PEAO)
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
    if(tabu.numero < 32 && setboard != 1)
    {
        if(tabu.tab[3][0] == -DAMA)
            totb += 50;
        if(tabu.tab[3][7] == DAMA)
            totp += 50;
    }
    //bonificacao para quem fez roque na abertura
    //TODO usar flag para saber se fez roque mesmo
    if(tabu.numero < 32 && setboard != 1)
    {
        if(tabu.tab[6][0] == -REI && tabu.tab[5][0] == -TORRE)  //brancas com roque pequeno
            totb += 70;
        if(tabu.tab[2][0] == -REI && tabu.tab[3][0] == -TORRE)  //brancas com roque grande
            totb += 50;
        if(tabu.tab[6][7] == REI && tabu.tab[5][7] == TORRE)  //pretas com roque pequeno
            totp += 70;
        if(tabu.tab[2][7] == REI && tabu.tab[3][7] == TORRE)  //pretas com roque grande
            totp += 50;
    }
    //bonificacao para rei protegido na abertura com os peoes do Escudo Real
    if(tabu.numero < 60 && setboard != 1)
    {
        if(tabu.tab[6][0] == -REI &&
                //brancas com roque pequeno
                tabu.tab[6][1] == -PEAO &&
                tabu.tab[7][1] == -PEAO && tabu.tab[7][0] == VAZIA)
            //apenas peoes g e h
            totb += 60;
        if(tabu.tab[2][0] == -REI &&
                //brancas com roque grande
                tabu.tab[0][1] == -PEAO &&
                tabu.tab[1][1] == -PEAO &&
                tabu.tab[2][1] == -PEAO &&
                tabu.tab[0][0] == VAZIA && tabu.tab[1][0] == VAZIA)
            //peoes a, b e c
            totb += 50;
        if(tabu.tab[6][7] == REI &&
                //pretas com roque pequeno
                tabu.tab[6][6] == PEAO &&
                tabu.tab[7][6] == PEAO && tabu.tab[7][7] == VAZIA)
            //apenas peoes g e h
            totp += 60;
        if(tabu.tab[2][7] == REI &&
                //pretas com roque grande
                tabu.tab[0][6] == PEAO &&
                tabu.tab[1][6] == PEAO &&
                tabu.tab[2][6] == PEAO &&
                tabu.tab[0][7] == VAZIA && tabu.tab[1][7] == VAZIA)
            //peoes a, b e c
            totp += 50;
    }
    //penalidade se mexer o peao do bispo, cavalo ou da torre no comeco da abertura!
    if(tabu.numero < 16 && setboard != 1)
    {
        //caso das brancas------------------
        if(tabu.tab[5][1] != -PEAO)
            //PBR
            totb -= 50;
        if(tabu.tab[6][1] != -PEAO)
            //PCR
            totb -= 40;
        if(tabu.tab[7][1] != -PEAO)
            //PTR
            totb -= 30;
        if(tabu.tab[0][1] != -PEAO)
            //PTD
            totb -= 30;
        if(tabu.tab[1][1] != -PEAO)
            //PCD
            totb -= 40;
        //              if(tabu.tab[2][1]==VAZIA)
        //PBD   nao eh penalizado!
        //                      totb-=10;
        //caso das pretas-------------------
        if(tabu.tab[5][6] != PEAO)
            //PBR
            totp -= 50;
        if(tabu.tab[6][6] != PEAO)
            //PCR
            totp -= 40;
        if(tabu.tab[7][6] != PEAO)
            //PTR
            totp -= 30;
        if(tabu.tab[0][6] != PEAO)
            //PTD
            totp -= 30;
        if(tabu.tab[1][6] != PEAO)
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

//insere o tabuleiro tabu na lista de tabuleiros
void insere_listab(tabuleiro tabu)
{
    int i, j, flag = 0;
    listab *plaux;
    if(plcabeca == NULL)  //lista vazia?
    {
        plcabeca = (listab *) malloc(sizeof(listab));
        if(plcabeca == NULL)
            msgsai("# Erro ao alocar memoria em insere_listab 1", 32);
        plfinal = plcabeca;
        plcabeca->ant = NULL;
        plfinal->prox = NULL;
        for(i = 0; i < 8; i++)
            for(j = 0; j < 8; j++)
                plfinal->tab[i][j] = tabu.tab[i][j];
        //posicao das pecas
        plfinal->vez = tabu.vez; //de quem e a vez
        plfinal->rep = 1; //primeira vez que aparece
        plfinal->peao_pulou = tabu.peao_pulou; //contem coluna do peao adversario que andou duas, ou -1 para nenhuma
        //1:pode para os 2 lados. 0:nao pode mais. 3:mexeu TD. 2:mexeu TR.
        plfinal->roqueb = tabu.roqueb;
        plfinal->roquep = tabu.roquep;
        plfinal->empate_50 = tabu.empate_50; //contador:quando chega a 50, empate.
        plfinal->situa = tabu.situa; //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.)
        //lance executado originador deste tabuleiro.
        for(i = 0; i < 4; i++)
            plfinal->lancex[i] = tabu.lancex[i];
        plfinal->numero = tabu.numero; //numero do lance (ply)
        plfinal->especial = tabu.especial;
    }
    else
    {
        plfinal->prox = (listab *) malloc(sizeof(listab));
        if(plfinal->prox == NULL)
            msgsai("# Erro ao alocar memoria em insere_listab 2", 33);
        plaux = plfinal;
        plfinal = plfinal->prox;
        plfinal->ant = plaux;
        plfinal->prox = NULL;
        for(i = 0; i < 8; i++)
            for(j = 0; j < 8; j++)
                plfinal->tab[i][j] = tabu.tab[i][j];
        plfinal->vez = tabu.vez;
        plfinal->peao_pulou = tabu.peao_pulou; //contem coluna do peao adversario que andou duas, ou -1 para nenhuma
        //1:pode para os 2 lados. 0:nao pode mais. 3:mexeu TD. 2:mexeu TR.
        plfinal->roqueb = tabu.roqueb;
        plfinal->roquep = tabu.roquep;
        plfinal->empate_50 = tabu.empate_50; //contador:quando chega a 50, empate.
        plfinal->situa = tabu.situa; //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.)
        //lance executado originador deste tabuleiro.
        for(i = 0; i < 4; i++)
            plfinal->lancex[i] = tabu.lancex[i];
        plfinal->numero = tabu.numero; //numero do lance
        plfinal->especial = tabu.especial;
        //compara o inserido agora com todos os anteriores...
        plaux = plfinal->ant;
        while(plaux != NULL)
        {
            flag = 0;
            for(i = 0; i < 8; i++)
            {
                for(j = 0; j < 8; j++)
                {
                    if(plfinal->tab[i][j] != plaux->tab[i][j])
                    {
                        //para ser diferente tem que mudar as pecas e a vez
                        flag = 1;
                        break;
                    }
                    //flag==1:tabuleiro diferente nao eh posicao repetida.
                }
                if(flag)
                    break;
            }
            if(plfinal->vez != plaux->vez)
                flag = 1;
            //flag==1: vez diferente nao eh posicao repetida
            if(!flag)  //flag == 0?
            {
                plfinal->rep = plaux->rep + 1;
                break;
            }
            plaux = plaux->ant;
        }
        if(flag)  //flag == 1?
            plfinal->rep = 1;
    }
}

//retira o plfinal: o ultimo tabuleiro da lista
void retira_listab(void)
{
    listab *plaux;
    if(plfinal == NULL)
        return;
    plaux = plfinal->ant;
    free(plfinal);
    if(plaux == NULL)
    {
        plcabeca = NULL;
        plfinal = NULL;
        return;
    }
    plfinal = plaux;
    plfinal->prox = NULL;
}

//para voltar um lance
void volta_lance(tabuleiro *tabu)
{
    int i, j;
    if(plcabeca == NULL || plfinal == NULL)  //nao tem ninguem... erro.
        return;
    if(plcabeca == plfinal)  //ja esta na posicao inicial
        return;
    retira_listab();
    //posicao das peoes
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
            tabu->tab[i][j] = plfinal->tab[i][j];
    tabu->vez = plfinal->vez; //de quem e a vez
    tabu->peao_pulou = plfinal->peao_pulou; //contem coluna do peao adversario que andou duas, ou -1 para nenhuma
    //1:pode para os 2 lados. 0:nao pode mais. 3:mexeu TD. 2:mexeu TR.
    tabu->roqueb = plfinal->roqueb;
    tabu->roquep = plfinal->roquep;
    tabu->empate_50 = plfinal->empate_50; //contador:quando chega a 50, empate.
    tabu->situa = plfinal->situa; //0:nada,1:Empate!,2:Xeque!,3:Brancas em mate,4:Pretas em mate,5 e 6: Tempo (Brancas e Pretas respec.)
    //lance executado originador deste tabuleiro.
    for(i = 0; i < 4; i++)
        tabu->lancex[i] = plfinal->lancex[i];
    tabu->numero = plfinal->numero; //numero do lance
    //tabu->especial
    //0:nada. 1:roque pqn. 2:roque grd. 3:comeu enpassant.
    //promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
    tabu->especial = plfinal->especial;
    if(tabu->numero < 50 && setboard < 1)
        USALIVRO = 1; //nao usar abertura em posicoes de setboard
}

void retira_tudo_listab(void)  //zera a lista de tabuleiros
{
    while(plcabeca != NULL)
        retira_listab();
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
        if(tabu.tab[icol][lin] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[icol][lin] == -TORRE || tabu.tab[icol][lin] == -DAMA)
            {
                total++;
                if(-tabu.tab[icol][lin] < *menor)
                    *menor = -tabu.tab[icol][lin];
                break;
            }
            else
                break;
        else // pretas atacam a casa
            if(tabu.tab[icol][lin] == TORRE || tabu.tab[icol][lin] == DAMA)
            {
                total++;
                if(tabu.tab[icol][lin] < *menor)
                    *menor = tabu.tab[icol][lin];
                break;
            }
            else
                break;
    }
    for(icol = col + 1; icol < 8; icol++)  //sobe coluna
    {
        if(tabu.tab[icol][lin] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[icol][lin] == -TORRE || tabu.tab[icol][lin] == -DAMA)
            {
                total++;
                if(-tabu.tab[icol][lin] < *menor)
                    *menor = -tabu.tab[icol][lin];
                break;
            }
            else
                break;
        else
            if(tabu.tab[icol][lin] == TORRE || tabu.tab[icol][lin] == DAMA)
            {
                total++;
                if(tabu.tab[icol][lin] < *menor)
                    *menor = tabu.tab[icol][lin];
                break;
            }
            else
                break;
    }
    for(ilin = lin + 1; ilin < 8; ilin++)  // direita na linha
    {
        if(tabu.tab[col][ilin] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[col][ilin] == -TORRE || tabu.tab[col][ilin] == -DAMA)
            {
                total++;
                if(-tabu.tab[col][ilin] < *menor)
                    *menor = -tabu.tab[col][ilin];
                break;
            }
            else
                break;
        else
            if(tabu.tab[col][ilin] == TORRE || tabu.tab[col][ilin] == DAMA)
            {
                total++;
                if(tabu.tab[col][ilin] < *menor)
                    *menor = tabu.tab[col][ilin];
                break;
            }
            else
                break;
    }
    for(ilin = lin - 1; ilin >= 0; ilin--)  // esquerda na linha
    {
        if(tabu.tab[col][ilin] == VAZIA)
            continue;
        if(cor == brancas)
            if(tabu.tab[col][ilin] == -TORRE || tabu.tab[col][ilin] == -DAMA)
            {
                total++;
                if(-tabu.tab[col][ilin] < *menor)
                    *menor = -tabu.tab[col][ilin];
                break;
            }
            else
                break;
        else //pecas pretas atacam
            if(tabu.tab[col][ilin] == TORRE || tabu.tab[col][ilin] == DAMA)
            {
                total++;
                if(tabu.tab[col][ilin] < *menor)
                    *menor = tabu.tab[col][ilin];
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
                if(tabu.tab[col + icol][lin + ilin] == -CAVALO)
                {
                    total++; //sim,ataca!
                    if(CAVALO < *menor)
                        *menor = CAVALO;
                }
                else
                    continue;
            else //cavalo preto ataca?
                if(tabu.tab[col + icol][lin + ilin] == CAVALO)
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
            while(tabu.tab[casacol][casalin] == 0);

            if(casacol >= 0 && casacol <= 7 && casalin >= 0 && casalin <= 7)
            {
                if(cor == brancas)
                    if(tabu.tab[casacol][casalin] == -BISPO || tabu.tab[casacol][casalin] == -DAMA)
                    {
                        total++;
                        if(-tabu.tab[casacol][casalin] < *menor)
                            *menor = -tabu.tab[casacol][casalin];
                        continue; //proxima diagonal
                    }
                    else
                        continue; // achou peca, mas esta nao anda em diagonal ou e' peca propria
                else //ataque de peca preta
                    if(tabu.tab[casacol][casalin] == BISPO || tabu.tab[casacol][casalin] == DAMA)
                    {
                        total++;
                        if(tabu.tab[casacol][casalin] < *menor)
                            *menor = tabu.tab[casacol][casalin];
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
                if(tabu.tab[icol][ilin] == -REI)
                {
                    total++;
                    break;
                }
                else
                    continue;
            else
                if(tabu.tab[icol][ilin] == REI)
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
                if(tabu.tab[col - 1][ilin] == -PEAO)
                {
                    if(PEAO < *menor)
                        *menor = PEAO;
                    total++;
                }
            if(col + 1 <= 7)
                if(tabu.tab[col + 1][ilin] == -PEAO)
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
                if(tabu.tab[col - 1][ilin] == PEAO)
                {
                    if(PEAO < *menor)
                        *menor = PEAO;
                    total++;
                }
            if(col + 1 <= 7)
                if(tabu.tab[col + 1][ilin] == PEAO)
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
    listab *plaux;
    char m[8];
    int n = 0, i;
    if(plcabeca == NULL)
    {
        strlance = NULL;
        return;
    }
    plaux = plcabeca->prox;
    //o primeiro tabuleiro nao tem lancex
    while(plaux != NULL)
    {
        lance2movi(m, plaux->lancex, plaux->especial);
        //flag_50 nao esta sendo usada!
        m[4] = '\0';
        for(i = 0; i < 4; i++)
            strlance[n + i] = m[i];
        n += 4;
        strlance[n] = ' ';
        n++;
        plaux = plaux->prox;
        //gira o laco
    }
    strlance[n] = '\0';
    return;
}

//retorna uma linha de jogo como se fosse a resposta do minimax
//com inicio no lance da vez.
movimento *string2pmovi(int numero, char *linha)
{
    char m[8];
    int n = 0, lanc[4], i, conta = 0;
    movimento *pmovi, *pmoviant, *cabeca;
    //posicao inicial
    tabuleiro tab =
    {
        {
            {-TORRE, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, TORRE},
            {-CAVALO, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, CAVALO},
            {-BISPO, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, BISPO},
            {-DAMA, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, DAMA},
            {-REI, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, REI},
            {-BISPO, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, BISPO},
            {-CAVALO, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, CAVALO},
            {-TORRE, -PEAO, VAZIA, VAZIA, VAZIA, VAZIA, PEAO, TORRE}
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
        pmovi = valido(tab, lanc);
        //lanc eh int lanc[4]; cria pval com tudo preenchido
        if(pmovi == NULL)
            break;
        //chamar joga_em() apenas para atualizar esse tabuleiro local, para usar a funcao valido()
        disc = (char) joga_em(&tab, *pmovi, 0);
        //a funcao joga_em deve inserir no listab, cod: 1:insere, 0:nao insere
        if(n / 5 >= numero)
            //chegou na posicao atual! comeca inserir na lista
        {
            if(cabeca == NULL)
            {
                cabeca = pmovi;
                pmoviant = pmovi;
            }
            else
            {
                pmoviant->prox = pmovi;
                pmoviant = pmoviant->prox;
                pmoviant->prox = NULL;
            }
        }
        pmovi = NULL;
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

//retorna em result.plance uma variante do livro
void usalivro(tabuleiro tabu)
{
    movimento *cabeca;
    char linha[256], strlance[256];
    FILE *flivro;
    int i = 0, sorteio;
    int novovalor;
    cabeca = NULL;
    flivro = fopen(bookfname, "r");
    if(!flivro)
        USALIVRO = 0;
    else
    {
        if(tabu.numero == 0)  // No primeiro lance de brancas, sorteia uma abertura
        {
            sorteio = (int) rand_minmax(0, LINHASDOLIVRO);
            //maximo de linhas no livro!
            while(!feof(flivro) && i < sorteio)
            {
                (void) fgets(linha, 256, flivro);
                i++;
            }
            while(linha[0] == '#')
                //a ultima linha nao deve ser comentario
                (void) fgets(linha, 256, flivro);
            cabeca = string2pmovi(tabu.numero, linha);
            result.valor = estatico_pmovi(tabu, cabeca);
            libera_lances(result.plance);
            result.plance = copilistmov(cabeca);
        }
        else //Responde de pretas com a primeira abertura que achar (primeira tentativa)
        {
            //Pega a primeira, e troca por outras via sorteio. (segunda tentativa)
            //mudar isso para avaliar as linhas e escolher a melhor (terceira)
            listab2string(strlance);
            //pega a lista de tabuleiros e faz uma string
            while(!feof(flivro))
            {
                (void) fgets(linha, 256, flivro);
                if(igual_strlances_strlinha(strlance, linha))
                {
                    if(cabeca == NULL)
                    {
                        cabeca = string2pmovi(tabu.numero, linha);
                        //eh a primeira linha que casou
                        result.valor = estatico_pmovi(tabu, cabeca);
                        libera_lances(result.plance);
                        result.plance = copilistmov(cabeca);
                    }
                    else
                    {
                        //                        sorteio=(int)rand_minmax(0,100);
                        //sorteia num. entre zero e 100 inclusive
                        //                        if(sorteio>85)
                        //as melhores linhas devem estar no topo do livro (so troca para baixo com P()=15%
                        //                        {
                        //                            libera_lances(cabeca);
                        //                              cabeca=string2pmovi(tabu.numero, linha);
                        //                        }
                        // Ordenar por valor estatico da variante
                        libera_lances(cabeca);
                        cabeca = string2pmovi(tabu.numero, linha);
                        //eh a primeira linha que casou
                        novovalor = estatico_pmovi(tabu, cabeca);
                        if(novovalor > result.valor)
                        {
                            result.valor = novovalor;
                            libera_lances(result.plance);
                            result.plance = copilistmov(cabeca);
                        }
                    } //                    libera_lances(cabeca);
                } //se achou no livro a abertura atual
            } //while tem linhas no arquivo
        } //else nao e o primeiro lance
    } //erro ao abrir arquivo Livro.txt
    if(flivro)
        (void) fclose(flivro);
    return;
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

//retorna um valor entre [min,max], inclusive
float rand_minmax(float min, float max)
{
    float sorteio;
    //    cloock_t s;
    //      time_t t;
    //      t=time(NULL);
    //      s = cloock()*10000/CLOCKS_PER_SEC; // retorna cloock em milesimos de segundos...
    static int ini = 1;
    if(ini)
    {
        srand((unsigned)(clock() * 100 / CLOCKS_PER_SEC + time(NULL)));
        //+(unsigned) time(&t));
        ini--;
    }
    sorteio = (float)(rand() % 2719);
    //rand retorna entre 0 e RAND_MAX;
    sorteio /= 2718.281828459;
    sorteio *= (max - min);
    sorteio += min;
    return sorteio;
}

//termina o programa
void sai(int error)
{
    printdbg(debug, "# xadreco : sai ( %d )\n", error);
    libera_lances(result.plance);
    result.plance = NULL;
    retira_tudo_listab();     //zera a lista de tabuleiros
    if(error != 36) // faltou comando xboard
        printfics("tellicsnoalias exit\n");
    exit(error);
}

//inicializa variaveis do programa. (new game)
void inicia(tabuleiro *tabu)
{
    int i, j;
    pausa = 'n';
    result.valor = 0;
    libera_lances(result.plance);
    result.plance = NULL;
    retira_tudo_listab();
    plcabeca = NULL; //cabeca da lista de repeteco
    ofereci = 1; //computador pode oferecer 1 empate
    USALIVRO = 1;
    //use o livro nas posicoes iniciais
//    if (setboard == 1) //se for -1, deixa aparecer o primeiro.
//        setboard = 0;
    tabu->roqueb = 1; // inicializar variaveis do roque e peao_pulou
    tabu->roquep = 1; //0:mexeu R e/ou 2T. 1:pode dos 2 lados. 2:mexeu TR. 3:mexeu TD.
    tabu->peao_pulou = -1; //-1:o adversario nao andou duas com peao. //0-7:coluna do peao que andou duas.
    tabu->vez = brancas;
    tabu->empate_50 = 0.0;
    tabu->numero = 0;
    ultimo_resultado[0] = '\0';
    tabu->situa = 0;
    tabu->especial = 0;
    for(i = 0; i < 4; i++)
        tabu->lancex[i] = 0;
    for(i = 0; i < 8; i++)
        for(j = 0; j < 8; j++)
            tabu->tab[i][j] = VAZIA;
    primeiro = 'h'; //humano inicia, com comandos para acertar detalhes do jogo
    segundo = 'c'; //computador espera para saber se jogara de brancas ou pretas
    nivel = 3; // sem uso depois de colocar busca em amplitude (uso no debug apenas)
    ABANDONA = -2430; // volta o abandona para jogar contra humanos...
    COMPUTER = 0; // jogando contra outra engine?
    //mostrapensando = 0; //post and nopost commands
    setboard = 0; //setboard command
    analise = 0; //analyze and exit commands
    tempomovclock = 3.0;	//em segundos
    tempomovclockmax = 3.0; // max allowed
    tbrancasac = 0.0; //tempo acumulado
    tpretasac = 0.0; //acumulated time
    tultimoinput = time(NULL); //pausa para nao fazer muito poll seguido
    WHISPER = 0; /* carinha feliz */
}

//coloca as peoes na posicao inicial
void  coloca_pecas(tabuleiro *tabu)
{
    int i;
    for(i = 0; i < 8; i++)  //i = column
    {
        tabu->tab[i][1] = -PEAO;
        tabu->tab[i][6] = PEAO;
    }
    tabu->tab[0][0] = tabu->tab[7][0] = -TORRE;
    tabu->tab[0][7] = tabu->tab[7][7] = TORRE;
    tabu->tab[1][0] = tabu->tab[6][0] = -CAVALO;
    tabu->tab[1][7] = tabu->tab[6][7] = CAVALO;
    tabu->tab[2][0] = tabu->tab[5][0] = -BISPO;
    tabu->tab[2][7] = tabu->tab[5][7] = BISPO;
    tabu->tab[3][0] = -DAMA;
    tabu->tab[4][0] = -REI;
    tabu->tab[3][7] = DAMA;
    tabu->tab[4][7] = REI;
}

//limpa algumas variaveis para iniciar ponderacao
void limpa_pensa(void)
{
    libera_lances(result.plance);
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
    (*pmovi)->lance[1] = c1; //move in integer notation
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

void msgsai(char *msg, int error)  //aborta programa por falta de memoria
{
    printdbg(debug, "# xadreco : %s\n", msg);
    if(debug)
        printfics("tellicsnoalias tell beco %s\n", msg);
    sai(error);
}

//cuidado para nao estourar o limite da cadeia, teclando muito <t>
//be carefull to do not overflow the string limit, pressing to much <t>
//retorna um lance do jogo de teste
void testajogo(char *movinito, int numero)
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
        //int(numero)*5; i<int(numero)*5+4; i++)
        // 0.3 0.8 1.3 1.8
        move[i] = jogo1[numero * 5 + i];
    strcpy(movinito, move);
    printf("%d:%s\n", (numero + 2) / 2, move);
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
void imprime_linha(movimento *loop, int numero, int tabuvez)
{
    int num, vez;
    char m[80];
//    FILE *fimprime;
    num = (int)((numero + 1.0) / 2.0);
    vez = tabuvez;
//    fimprime = NULL;
//    if (debug == 2)
//        fimprime = fmini;
//    if (debug == 1)
//        fimprime = fsaida;
    printf("# ");
    while(loop != NULL)
    {
        lance2movi(m, loop->lance, loop->especial);
        if(vez == brancas)  //jogou anterior as pretas
        {
            if(tabuvez == brancas && num == (int)((numero + 1.0) / 2.0))
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

//void ordena_succ(void){}
//para testar sem a funcao ordena_succ. (notei que ha ganho no corte de nodos)
void ordena_succ(int nmov)  //ordena succ_geral
{
    movimento *loop, *pior, *insere, *aux;
    int peguei, pior_valor;
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
            libera_lances(insere);
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
            libera_lances(insere);
            insere = aux;
            loop = loop->prox;
        }
        else
            break;
    libera_lances(succ_geral);
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
}

// pollinput() Emprestado do jogo de xadrez pepito: dica de Fabio Maia
// pollinput() Borrowed from pepito: tip from Fabio Maia
// retorna verdadeiro se existe algum caracter no buffer para ser lido
// return true if there are some character in the buffer to be read
int pollinput(void)
{
#ifndef _WIN32
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
#else

    static int init = 0;
    static int pipe;
    static HANDLE inh;
    static INPUT_RECORD Buffer[256];

    DWORD dw = 0;
    if(!init)
    {
        init = 1;
        inh = GetStdHandle(STD_INPUT_HANDLE);
        pipe = !GetConsoleMode(inh, &dw);
        if(!pipe)
        {
            SetConsoleMode(inh,
                           dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(inh);
        }
    }
    if(pipe)
    {
        if(!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL))
            return 1;
        return dw;
    }
    else
    {
        GetNumberOfConsoleInputEvents(inh, &dw);
        return (dw <= 1 ? 0 : dw);
    }
#endif
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
    int nmov;
    int moveto, i;
    movimento *cabeca_succ = NULL, *succ = NULL, *melhor_caminho = NULL;

    limpa_pensa();  //limpa plance para iniciar a ponderacao
    nmov = 0; //gerar todos
    cabeca_succ = geramov(*tabu, &nmov);  //gera os sucessores
    succ = cabeca_succ;
    moveto = (int)(rand() % nmov);  //sorteia um lance possivel da lista de lances
    for(i = 0; i < moveto; ++i)
        if(succ != NULL)
            succ = succ->prox; //escolhe este lance como o que sera jogado

    if(succ != NULL)
    {
        succ->valor_estatico = 0;
        melhor_caminho = copimel(*succ, NULL);
        result.valor = 0;
        result.plance = copilistmov(melhor_caminho);
        libera_lances(melhor_caminho);
        libera_lances(cabeca_succ);  //BUG era succ_geral, virou succ, agora eh cabeca_succ
        return '-'; //ok
    } //else succ!=NULL
    printf("# empty from randommove\n");
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
    printf("%s - %s\n", "Xadreco", "5.84, by Dr. Beco");
    printf("\nUsage: xadreco [-h|-v] [-c{none,fics,lichess}] [-r] [-x] [-b path/bookfile.txt]\n");
    printf("\nOptions:\n");
    printf("\t-h,  --help\n\t\tShow this help.\n");
    printf("\t-V,  --version\n\t\tShow version and copyright information.\n");
    printf("\t-v,  --verbose\n\t\tSet verbose level (cumulative).\n");
    /* add more options here */
    printf("\t-r seed,  --random seed\n\t\tXadreco plays random moves. Initializes seed. If seed=0, 'true' random.\n");
    printf("\t-c,  --connection\n\t\tnone: no connection; fics: freeches.org; lichess: lichess.org\n");
    printf("\t-x,  --xboard\n\t\tGives 'xboard' keyword immediattely (protocol is xboard with or withour -x)\n");
    printf("\t-b path/bookfile.txt,  --book\n\t\tSets the path for book file (not implemented)\n");
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

/* print fics commands */
void printfics(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    if(server==fics)
        vprintf(fmt, args);
    va_end(args);
}

/* print debug information  */
void printdbg(int dbg, ...)
{
    va_list args;
    char *fmt;

    va_start(args, dbg);
    fmt=va_arg(args, char *);
    /* if(dbg>DEBUG) */
    if(dbg)
        vprintf(fmt, args);
    va_end(args);
}

/* ---------------------------------------------------------------------------- */
/* vi: set ai cin et ts=4 sw=4 tw=0 wm=0 fo=croqltn : C config for Vim modeline */
/* Template by Dr. Beco <rcb at beco dot cc>  Version 20160714.153029           */

