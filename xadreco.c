// xadreco version 1.0. Chess engine compatible with XBoard/WinBoard
// Protocol (C)
// Copyright (C) 1994-2013, Ruben Carlo Benante.
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.
// 
// If you redistribute it and/or modify it, you need to
// cite the original source-code and author name:
// Ruben Carlo Benante.
// email: benante@ig.com.br
// web page: http://www.geocities.com/pag_sax/xadreco/
// address:
// Federal University of Pernambuco
// Center of Computer Science
// PostGrad Room 7-1
// PO Box 7851
// Zip Code 50670-970
// Recife, PE, Brazil
// FAX: +55 (81) 2126-8438
// PHONE: +55 (81) 2126-8430 x.4067
// 
// -----------------------------------------------------------------------------
// 
// xadreco version 1.0. Motor de xadrez compatível com o XBoard/WinBoard
// (C)
// Copyright (C) 2004, Ruben Carlo Benante.
// 
// Este programa é software livre; você pode redistribuí-lo e/ou
// modificá-lo sob os termos da Licença Pública Geral GNU, conforme
// publicada pela Free Software Foundation; tanto a versão 2 da
// Licença como (a seu critério) qualquer versão mais nova.
// 
// Este programa é distribuído na expectativa de ser útil, mas SEM
// QUALQUER GARANTIA; sem mesmo a garantia implícita de
// COMERCIALIZAÇÃO ou de ADEQUAÇÃO A QUALQUER PROPÓSITO EM
// PARTICULAR. Consulte a Licença Pública Geral GNU para obter mais
// detalhes.
// 
// Você deve ter recebido uma cópia da Licença Pública Geral GNU
// junto com este programa; se não, escreva para a Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
// 02111-1307, USA.
// 
// Se você redistribuir ou modificar este programa, você deve citar
// o código-fonte original e o nome do autor:
// Ruben Carlo Benante.
// email: benante@ig.com.br
// página web: http://www.geocities.com/pag_sax/xadreco/
// endereço:
// Universidade Federal de Pernambuco
// Centro de Informática
// Sala de Pós-graduação 7-1
// Caixa Postal 7851
// CEP 50670-970
// Recife, PE, Brasil
// FAX: +55 (81) 2126-8438
// FONE: +55 (81) 2126-8430 ramal.4067
// 
// -----------------------------------------------------------------------------
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %% Direitos Autorais segundo normas do código-livre: (GNU - GPL)
// %% Universidade Estadual Paulista - UNESP, Universidade Federal de
// Pernambuco - UFPE
// %% Jogo de xadrez: xadreco
// %% Arquivo xadreco.cpp
// %% Técnica: MiniMax
// %% Autor do original: Ruben Carlo Benante
// %% Criacao: 10/06/1999
// %% Versão para Winboard: 26/09/04
// %% e-mail do autor: benante@ig.com.br
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %% Modificado por (Modified by):
// %% email:
// %% Data (date):
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// -----------------------------------------------------------------------------

// #include <alloc.h>
// #include <dos.h>
// #include <time.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// dados ----------------------
struct tabuleiro
{
  int tab[8][8];		// contem as pecas
  int vez;			// contem -1 ou 1 para 'branca' ou 'preta'
  int peao_pulou;		// contem coluna do peao adversario que
  // andou duas, ou -1 para 'nao pode comer enpassant'
  int roqueb, roquep;		// 1:pode para os 2 lados. 0:nao pode
  // mais. 3:mexeu TD. 2:mexeu TR.
  float empate_50;		// contador:quando chega a 50, empate.
  int situa;			// 0:nada,1:Empate!,2:Xeque!,3:Brancas em
  // mate,4:Pretas em mate,5 e 6: Tempo
  // (Brancas e Pretas respec.)
  int lancex[4];		// lance executado originador deste
  // tabuleiro.
  int numero;			// numero do lance
};
struct movimento
{
  int lance[4];
  int peao_pulou;		// contem coluna do peao que andou duas
  // neste lance
  int roque;			// 0:mexeu rei. 1:ainda pode. 2:mexeu TR.
  // 3:mexeu TD.
  int flag_50;			// Quando igual a:0=nada,1=Moveu
  // peao,2=Comeu,3=Peao Comeu. Entao zera
  // empate_50;
  int especial;			// 0:nada. 1:roque pqn. 2:roque grd.
  // 3:comeu enpassant.
  // promocao: 4=Dama, 5=Cavalo, 6=Torre, 7=Bispo.
  movimento *prox;
};
struct resultado
{
  int valor;
  movimento *plance;
};
struct listab
{
  int tab[8][8];
  int vez;
  int rep;			// quantidade de vezes que essa posicao ja 
  // 
  // repetiu comparada
  listab *prox;
  listab *ant;			// com as posicoes de traz da lista
  // encadeada.
};
resultado result;
listab *plcabeca;		// cabeca da lista encadeada de posicoes
				// repetidas
listab *plfinal;		// fim da lista encadeada
int xboard = 0;
FILE *fsaida;
int debug = 1;			// coloque zero para evitar gravar arquivo
enum
{ REI = 1000, DAMA = 100, TORRE = 50, BISPO = 32, CAVALO = 30, PEAO =
    10, VAZIA = 0
};
char primeiro = 'h', segundo = 'c';
char pausa;
int nivel = 1;			// nivel de profundidade
int profflag = 0;		// Flag de captura ou xeque para liberar
				// mais um nivel em profsuf
const int brancas = -1;
const int pretas = 1;
unsigned char gira = (unsigned char) 0;

// prototipos -----------------
void imptab (tabuleiro tabu);	// imprime o tabuleiro
void lance2movi (char *m, int *l, int flag_50, int especial);	// transforma 
									// 
									// lances 
									// 
									// int 
									// 
									// 0077 
									// 
									// em 
									// 
									// char 
									// 
									// tipo 
									// 
									// a1h8
int movi2lance (int *l, char *m);	// faz o contrario: char
						// b1-c3 em int 1022.
						// Retorna falso se nao
						// existe.
inline int adv (int vez);	// retorna o adversario de quem esta na
				// vez
inline int sinal (int x);	// retorna 1, -1 ou 0. Sinal de x
movimento *geramov (tabuleiro tabu);	// retorna lista de lances 
						// 
						// possiveis, ordenados.
void copitab (tabuleiro * dest, tabuleiro * font);	// copia
								// font
								// para
								// dest
int ataca (int cor, int col, int lin, tabuleiro tabu);	// retorna 
									// 
									// 1 
									// 
									// se 
									// 
									// "cor" 
									// 
									// ataca 
									// 
									// casa(col,lin) 
									// 
									// no 
									// 
									// tabuleiro 
									// 
									// tabu
int qataca (int cor, int col, int lin, tabuleiro tabu);	// retorna 
									// 
									// o 
									// 
									// numero 
									// 
									// de 
									// 
									// ataques 
									// 
									// da 
									// 
									// "cor" 
									// 
									// na 
									// 
									// casa(col,lin) 
									// 
									// de 
									// 
									// tabu
int xeque_rei_das (int cor, tabuleiro tabu);	// retorna 1 se o
							// rei de cor
							// "cor" esta em
							// xeque
movimento *valido (tabuleiro tabu, int *lanc);	// procura nos
							// movimentos de
							// geramov se o
							// lance em
							// questao eh
							// valido. Se nao, 
							// 
							// retorna NULL
int igual (int *lance1, int *lance2);	// compara dois
							// vetores de
							// lance[4]. Se
							// igual, retorna
							// 1
void mostra_lances (tabuleiro tabu);	// mostra na tela
						// informacoes do jogo
char situacao (tabuleiro tabu);	// retorna char que indica 
						// 
						// a situacao do
						// tabuleiro, como mate,
						// empate, etc...
void libera_lances (movimento * cabeca);	// libera da
							// memoria uma
							// lista encadeada 
							// 
							// de movimentos
void retira_tudo_listab (void);	// zera a lista de
						// tabuleiros
void gotoxy (int x, int y);
void clrscr (void);
void minimax (tabuleiro atual, int prof, int uso, int passo);	// coloca 
									// 
									// em 
									// 
									// result 
									// 
									// a 
									// 
									// melhor 
									// 
									// variante 
									// 
									// e 
									// 
									// seu 
									// 
									// valor.
int profsuf (int prof);		// retorna verdadeiro se
					// (prof>nivel) ou (prof==nivel e
					// nao houve captura ou xeque) ou
					// (houve Empate!)
int estatico (tabuleiro tabu);	// retorna um valor
						// estatico que avalia uma 
						// 
						// posicao do tabuleiro,
						// fixa
char joga_em (tabuleiro * tabu, movimento movi);	// joga o
								// movimento 
								// 
								// movi em 
								// 
								// tabuleiro 
								// 
								// tabu.
								// retorna 
								// 
								// situacao.
movimento *copimel (movimento pmovi, movimento * plance);	// retorna 
								// 
								// nova
								// lista
								// contendo 
								// 
								// o
								// movimento 
								// 
								// pmovi
								// mais a
								// sequencia 
								// 
								// de
								// movimentos 
								// 
								// plance. 
								// 
								// (para
								// melhor_caminho)
void copimov (movimento * dest, movimento * font);	// copia
								// os
								// itens
								// da
								// estrutura 
								// 
								// movimento, 
								// 
								// mas nao 
								// 
								// copia
								// ponteiro 
								// 
								// prox.
void copilistmovmel (movimento * dest, movimento * font);	// mantem 
									// 
									// dest, 
									// 
									// e 
									// 
									// copia 
									// 
									// para 
									// 
									// dest->prox 
									// 
									// a 
									// 
									// lista 
									// 
									// encadeada 
									// 
									// font
movimento *copilistmov (movimento * font);	// copia uma lista
						// encadeada para outra
						// nova. Retorna cabeca da 
						// 
						// lista destino
void insere_listab (tabuleiro tabu);	// posiciona plcabeca,
						// plfinal.  Para casos de 
						// 
						// empate de repeticao de
						// lances.
void retira_listab (void);	// posiciona plfinal no *ant. ou
					// seja: apaga o ultimo da lista.
char humajoga (tabuleiro * tabu);
char compjoga (tabuleiro * tabu);

// funcoes --------------------
int
main (int argc, char *argv[])
{
  tabuleiro tabu;
  char resultado, opc;
  int i, j;
  char comando[80];
  setbuf (stdout, NULL);

  // setbuf(stdin, inbuf);
  if (debug)
    fsaida = fopen ("xadreco.log", "w");
  printf ("Xadreco version 1.0, Copyright (C) 2004 Ruben Carlo Benante\n\
Xadreco comes with ABSOLUTELY NO WARRANTY;\n\
This is free software, and you are welcome to redistribute it \
under certain conditions; Please, visit http://www.fsf.org/licenses/gpl.html\n\
for details.\n\n");
  printf ("Xadreco versão 1.0, Copyright (C) 2004 Ruben Carlo Benante\n\
O Xadreco não possui QUALQUER GARANTIA;\n\
Ele é software livre e você está convidado a redistribui-lo \
sob certas condições; Por favor, visite http://www.fsf.org/licenses/gpl.html\n\
para obter detalhes.\n\n");
  if (debug)
    {
      fprintf (fsaida,
	       "Xadreco version 1.0, Copyright (C) 2004 Ruben Carlo Benante\n\
Xadreco comes with ABSOLUTELY NO WARRANTY;\n\
This is free software, and you are welcome to redistribute it \
under certain conditions; Please, visit http://www.fsf.org/licenses/gpl.html\n for details.\n\n");
      fprintf (fsaida,
	       "Xadreco versão 1.0, Copyright (C) 2004 Ruben Carlo Benante\n\
O Xadreco não possui QUALQUER GARANTIA;\n\
Ele é software livre e você está convidado a redistribui-lo \
sob certas condições; Por favor, visite http://www.fsf.org/licenses/gpl.html\n\
para obter detalhes.\n\n");
    }
  // scanf("%s",&comando);
  // int argc, char *argv[]
  if (argc > 1)
    if (!strcmp (argv[1], "xboard"))	// é xboard!
      {
	xboard = 1;

	// scanf("%s",&comando);
	// if(!strcmp(comando,"protover 2")) //protocolo versao 2.0 ou 
	// 
	// superior
	// {
	if (debug)
	  {
	    fprintf (fsaida, "xadreco: feature done=0\n");
	    fprintf (fsaida,
		     "xadreco: feature ping=0 setboard=0 playother=0 san=0 usermove=0 time=0 draw=1 sigint=1 sigterm=1 reuse=1 analyze=0 myname=\"Xadreco v.1, by Beco\" variants=\"normal\" colors=0 ics=0 name=0 pause=0\n");
	    fprintf (fsaida, "xadreco: feature done=1\n");
	  }
	printf ("feature done=0\n");
	printf
	  ("feature ping=0 setboard=0 playother=0 san=0 usermove=0 time=0 draw=1 sigint=1 sigterm=1 reuse=1 analyze=0 myname=\"Xadreco v.1, by Beco\" variants=\"normal\" colors=0 ics=0 name=0 pause=0\n");
	printf ("feature done=1\n");

	// printf("tellicsnoalias kibitz Hello from Xadreco v.5\n");

	// scanf("%s",&comando);
	// if(strcmp(comando,"accepted")==0) //aceitou os parâmetros
	// {
	// ;// printf("nop\n");
	// }
	// else
	// if(strcmp(comando,"rejected")==0) //rejeitou os parâmetros
	// exit(1);
	// }
	// else
	// if(!strcmp(comando,"new")) //protocolo antigo
	// ;
      }

  do
    {
      if (!xboard)
	clrscr ();
      pausa = 'n';
      result.valor = 0;
      result.plance = NULL;
      plcabeca = NULL;		// cabeca da lista de repeteco
      // inicializar variaveis do roque e peao_pulou
      tabu.roqueb = 1;		// 0:mexeu R e/ou 2T. 1:pode dos 2 lados.
      // 2:mexeu TR. 3:mexeu TD.
      tabu.roquep = 1;
      tabu.peao_pulou = -1;	// -1:o adversario nao andou duas com
      // peao.
      // 0-7:coluna do peao que andou duas.
      tabu.vez = brancas;
      tabu.empate_50 = 0.0;
      for (i = 0; i < 4; i++)
	tabu.lancex[i] = 0;
      tabu.numero = 0;
      for (i = 0; i < 8; i++)
	for (j = 2; j < 6; j++)
	  tabu.tab[i][j] = VAZIA;
      for (i = 0; i < 8; i++)
	{
	  tabu.tab[i][1] = -PEAO;
	  tabu.tab[i][6] = PEAO;
	}
      tabu.tab[0][0] = tabu.tab[7][0] = -TORRE;
      tabu.tab[0][7] = tabu.tab[7][7] = TORRE;
      tabu.tab[1][0] = tabu.tab[6][0] = -CAVALO;
      tabu.tab[1][7] = tabu.tab[6][7] = CAVALO;
      tabu.tab[2][0] = tabu.tab[5][0] = -BISPO;
      tabu.tab[2][7] = tabu.tab[5][7] = BISPO;
      tabu.tab[3][0] = -DAMA;
      tabu.tab[4][0] = -REI;
      tabu.tab[3][7] = DAMA;
      tabu.tab[4][7] = REI;
      insere_listab (tabu);

      // --------------------------------------------------------------------------------
      if (xboard)
	{
	  primeiro = 'h';
	  segundo = 'c';
	  if (debug)
	    fprintf (fsaida, "xadreco: sd 2\n");	// nível de
	  // dificuldade 2
	  printf ("sd 2\n");	// nível de dificuldade 2
	  nivel = 2;
	}

      else
	{
	  printf
	    ("\n\n\nQuem joga de brancas (c:Computador, h ou espaco:Humano)? ");
	  primeiro = getch ();
	  if (primeiro == ' ')
	    {
	      primeiro = 'h';
	      printf ("h");
	    }

	  else
	    printf ("%c", primeiro);
	  printf
	    ("\nQuem joga de  pretas (c ou espaco:Computador, h:Humano)? ");
	  segundo = getch ();
	  if (segundo == ' ')
	    {
	      segundo = 'c';
	      printf ("c");
	    }

	  else
	    printf ("%c", segundo);
	  if (primeiro == 'c' || segundo == 'c')
	    {
	      printf ("\n\nQual o nivel de dificuldade (1-6): ");
	      scanf ("%d", &nivel);
	    }
	  if (primeiro == segundo && primeiro == 'c')
	    {
	      printf ("\n\nPausa entre os lances? (s ou espaco / n): ");
	      pausa = getch ();
	      if (pausa == ' ')
		pausa = 's';
	    }
	  imptab (tabu);
	}
      if (!xboard && pausa == 's')
	{
	  gotoxy (10, 20);
	  printf ("Tecle algo para prosseguir. c-comando.");
	  pausa = getch ();
	  if (pausa != 'c')
	    pausa = 's';
	}
    joga_novamente:if (tabu.vez == brancas)
	// jogam as brancas
	if (primeiro == 'c')
	  resultado = compjoga (&tabu);

	else
	  resultado = humajoga (&tabu);

      else			// jogam as pretas
      if (segundo == 'c')
	resultado = compjoga (&tabu);

      else
	resultado = humajoga (&tabu);
      if (resultado != 'w')
	imptab (tabu);
      switch (resultado)
	{
	case 'M':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 0-1 {White is check-mated}\n");
	      printf ("0-1 {White is check-mated}\n");
	    }

	  else
	    printf ("\nAs brancas estao em xequemate.");	// tabu.situa=3
	  break;
	case 'm':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 1-0 {Black is check-mated}\n");
	      printf ("1-0 {Black is check-mated}\n");
	    }

	  else
	    printf ("\nAs pretas estao em xequemate.");	// tabu.situa=4
	  break;
	case 'a':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 1/2-1/2 {Stalemate}\n");
	      printf ("1/2-1/2 {Stalemate}\n");
	    }

	  else
	    printf ("\nEmpate por afogamento.");	// tabu.situa=1
	  break;
	case 'p':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 1/2-1/2 {endless check}\n");
	      printf ("1/2-1/2 {endless check}\n");
	    }

	  else
	    printf ("\nEmpate por xeque perpetuo.");	// tabu.situa=1
	  break;
	case 'c':
	  if (xboard)
	    {
	      if (debug)
		{
		  fprintf (fsaida, "xadreco: offer draw\n");
		  fprintf (fsaida,
			   "xadreco: 1/2-1/2 {both players agree to draw}\n");
		}
	      printf ("offer draw\n");
	      printf ("1/2-1/2 {both players agree to draw}\n");
	    }

	  else
	    printf ("\nEmpate por comum acordo.");
	  break;
	case 'i':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida,
			 "xadreco: 1/2-1/2 {insufficient mating material }\n");
	      printf ("1/2-1/2 {insufficient mating material }\n");
	    }

	  else
	    printf ("\nEmpate por insuficiencia de material.");	// tabu.situa=1
	  break;
	case '5':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida,
			 "xadreco: 1/2-1/2 {Draw by fifty moves rule}\n");
	      printf ("1/2-1/2 {Draw by fifty moves rule}\n");
	    }

	  else
	    printf ("\nEmpate apos 50 lances sem movimento de peao, e sem captura.");	// tabu.situa=1
	  break;		// Inclui o caso de Rei x Rei e Pecas:
	  // obrigatorio dar mate em 50 lances
	case 'r':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 1/2-1/2 {Draw by repetition}\n");
	      printf ("1/2-1/2 {Draw by repetition}\n");
	    }

	  else
	    printf ("\nEmpate por repeticao da mesma posicao por tres vezes.");	// tabu.situa=1
	  break;
	case 'T':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 0-1 {White flag fell}\n");
	      printf ("0-1 {White flag fell}\n");
	    }

	  else
	    printf ("\nAs brancas perderam por tempo.");	// tabu.situa=5
	  break;
	case 't':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 1-0 {Black flag fell}\n");
	      printf ("1-0 {Black flag fell}\n");
	    }

	  else
	    printf ("\nAs pretas perderam por tempo.");	// tabu.situa=6
	  break;
	case 'B':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 0-1 {White resigns}\n");
	      printf ("0-1 {White resigns}\n");
	    }

	  else
	    printf ("\nAs brancas abandonam.");
	  break;
	case 'b':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 1-0 {Black resigns}\n");
	      printf ("1-0 {Black resigns}\n");
	    }

	  else
	    printf ("\nAs pretas abandonam.");
	  break;
	case 'e':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida,
			 "xadreco: 1/2-1/2 {internal error aborted engine}\n");
	      printf ("1/2-1/2 {internal error aborted engine}\n");
	    }

	  else
	    printf
	      ("\nErro: O computador ficou sem lances! Confira o nivel de dificuldade.");
	  break;
	case 'w':
	  opc = 's';		// Novo jogo
	  libera_lances (result.plance);
	  retira_tudo_listab ();	// zera a lista de tabuleiros
	  continue;
	case 'V':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 1-0 {White wins}\n");
	      printf ("1-0 {White wins}\n");
	    }

	  else
	    printf ("\nAs Brancas ganham.");
	  break;
	case 'v':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 0-1 {Black wins}\n");
	      printf ("0-1 {Black wins}\n");
	    }

	  else
	    printf ("\nAs pretas ganham.");
	  break;
	case 'j':
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: 1/2-1/2 {Draw}\n");
	      printf ("1/2-1/2 {Draw}\n");
	    }

	  else
	    printf ("\nEmpate.");
	  break;
	case 'x':
	  if (!xboard)
	    {
	      printf ("\nXeque !");	// tabu.situa=2
	      gotoxy (1, 14);
	    }
	case 'y':		// retorna y: simplesmente gira o laço
	  // para jogar de novo. Troca de adv.
	default:
	  if (!xboard && (pausa == 's' || pausa == 'c'))
	    {
	      gotoxy (10, 20);
	      printf ("Tecle algo para prosseguir. c-comando.");
	      pausa = getch ();
	      if (pausa != 'c')
		pausa = 's';
	    }
	  goto joga_novamente;
	}
      if (!xboard)
	{
	  printf ("\nQuer recomecar? (s/n): ");

	  do
	    {
	      opc = getche ();
	    }
	  while (opc != 's' && opc != 'n');
	}

      else
	{

	  // scanf("%s",&comando);
	  // if(debug)
	  // fprintf(fsaida,"winboard: %s\n",comando);
	  // if(!strcmp(comando,"new"))
	  opc = 's';

	  // else
	  // opc='n';
	}
      libera_lances (result.plance);
      retira_tudo_listab ();	// zera a lista de tabuleiros
    }
  while (opc == 's');
  if (debug)
    fclose (fsaida);
}

void
mostra_lances (tabuleiro tabu)
{
  movimento *pmovi, *loop;
  char m[80];
  m[79] = '\0';
  int linha = 1, coluna = 34;
  pmovi = geramov (tabu);
  loop = pmovi;
  if (!xboard)
    gotoxy (coluna, linha);
  while (loop != NULL)
    {
      lance2movi (m, loop->lance, loop->flag_50, loop->especial);
      if (!xboard)
	printf ("%s", m);
      linha++;
      if (linha == 17)
	{
	  linha = 1;
	  coluna += 9;
	}
      if (!xboard)
	gotoxy (coluna, linha);
      loop = loop->prox;
    }
  if (!xboard)
    {
      gotoxy (40, 21);
      printf ("Nao move peao ou captura a %.1f lances", tabu.empate_50);
      gotoxy (40, 22);
      printf ("Valor do  Lance: %d", result.valor);
      gotoxy (40, 23);
      printf ("Melhor variante: ");
      loop = result.plance;
      while (loop != NULL)
	{
	  lance2movi (m, loop->lance, loop->flag_50, loop->especial);
	  printf ("%s,", m);
	  loop = loop->prox;
	}
      gotoxy (1, 16);
      printf ("                         ");	// linha de comando
      gotoxy (1, 16);
    }
  // libera memoria da lista de lances ja utilizada
  libera_lances (pmovi);
}

void
libera_lances (movimento * cabeca)
{
  movimento *loop, *aux;
  if (cabeca == NULL)
    return;
  loop = cabeca;
  aux = cabeca;
  while (loop != NULL)
    {
      aux = aux->prox;
      free (loop);
      loop = aux;
    }
  cabeca = NULL;
}

void
imptab (tabuleiro tabu)
{
  int col, lin, casacor;
  int colmax, colmin, linmax, linmin, inc;
  char movinito[80];
  movinito[79] = '\0';
  if (!xboard)
    {
      linmin = 7;
      linmax = -1;
      inc = -1;
      colmin = 0;
      colmax = 8;
      if (primeiro == 'c' && segundo == 'h')
	{
	  linmin = 0;
	  linmax = 8;
	  inc = 1;
	  colmin = 7;
	  colmax = -1;
	}
      // textbackground(0);
      clrscr ();
      printf ("\n\r  ");

      // textbackground(1); //azul
      printf ("+------------------------+");
      lin = linmin;

      do
	{

	  // textcolor(15); //branco
	  // textbackground(0); //preto
	  printf ("\n\r%d ", lin + 1);

	  // textbackground(1); //azul
	  printf ("|");
	  col = colmin;

	  do
	    {
	      if ((lin + col) % 2)
		// textbackground(2); //casa branca
		casacor = 1;

	      else
		// textbackground(14); //casa preta
		casacor = 2;

	      // if(tabu.tab[col][lin]<0) //cor da peca negativa? Sempre 
	      // 
	      // branca.
	      // if(tabu.vez==brancas && tabu.tab[col][lin]==-REI)
	      // textcolor(15 + 128); //Rei branco pisca
	      // else
	      // textcolor(15); //peca branca
	      // else //peca preta
	      // if(tabu.vez==pretas && tabu.tab[col][lin]==REI)
	      // textcolor(0 + 128); //Rei preto pisca
	      // else
	      // textcolor(0); //peca positiva: preta.
	      switch (tabu.tab[col][lin])
		{
		case PEAO:
		  printf (" p ");
		  break;
		case CAVALO:
		  printf (" c ");
		  break;
		case BISPO:
		  printf (" b ");
		  break;
		case TORRE:
		  printf (" t ");
		  break;
		case DAMA:
		  printf (" d ");
		  break;
		case REI:
		  printf (" r ");
		  break;

		  // pecas brancas
		case -PEAO:
		  printf (" P ");
		  break;
		case -CAVALO:
		  printf (" C ");
		  break;
		case -BISPO:
		  printf (" B ");
		  break;
		case -TORRE:
		  printf (" T ");
		  break;
		case -DAMA:
		  printf (" D ");
		  break;
		case -REI:
		  printf (" R ");
		  break;
		default:
		  if (casacor == 1)
		    printf (" . ");	// casa branca
		  else
		    {

		      // SetTextColor(hdc,RGB(250,10,10));
		      printf (" * ");	// casa preta
		    }
		  break;
		}
	      col -= inc;
	    }
	  while (col != colmax);

	  // textbackground(1);// azul
	  printf ("|");
	  lin += inc;
	}
      while (lin != linmax);

      // textcolor(15);
      // textbackground(0);
      printf ("\n\r  ");

      // textbackground(1); //azul
      printf ("+------------------------+");

      // textcolor(15);
      // textbackground(0);
      printf ("\n\r  ");
      if (primeiro == 'h' || primeiro == segundo)
	printf ("  a  b  c  d  e  f  g  h\n\n\r");
      if (primeiro == 'c' && segundo == 'h')
	printf ("  h  g  f  e  d  c  b  a\n\n\r");

      // textcolor(10);
      // textbackground(0);
    }				// nao eh xboard.

  // if(tabu.lancex[0]==0 && tabu.lancex[1]==0 && tabu.lancex[2]==0 &&
  // tabu.lancex[3]==0)
  if (tabu.numero == 0)
    {
      if (!xboard)
	printf ("Lance executado :       ");	// apagar o lance anterior 
      // 
      // da tela
    }

  else
    {
      lance2movi (movinito, tabu.lancex, -1, -1);
      if (!xboard)
	if (tabu.vez == pretas)
	  printf ("Lance executado :%d- %s", tabu.numero, movinito);

	else
	  printf ("Lance executado :%d- ...., %s", tabu.numero, movinito);

      else
	{
	  if ((tabu.vez == brancas && segundo == 'c')
	      || (tabu.vez == pretas && primeiro == 'c'))
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: %s\n", movinito);
	      printf ("move %s\n", movinito);
	    }
	}
    }
}

void
lance2movi (char *m, int *l, int flag_50, int espec)	// int para char
{
  int i;

  // char *n=" "; //aloca tam 9 a1-h8.ep'\0'
  // n[79]='\0';
  for (i = 0; i < 2; i++)
    {
      m[2 * i] = l[2 * i] + 'a';
      m[2 * i + 1] = l[2 * i + 1] + '1';
    }
  m[4] = '\0';

  // n[0]=m[0];n[1]=m[1]; //comentei pois não lembrei para que serve
  // n[2]='-'; //colocar um tracinho no meio
  // n[3]=m[2];n[4]=m[3];
  // n[2]=m[2];n[3]=m[3];//comentei pois não lembrei para que serve
  if (flag_50 == -1 && espec == -1)
    {

      // m=strcpy(m,n); //comentei pois não lembrei para que serve
      return;
    }
  // if(flag_50>1) // ou xizinho caso capturou
  // n[2]='x';
  switch (espec)		// promocao para 4,5,6,7 == D,C,T,B
    {
    case 0:
      break;
    case 1:			// n[0]='o';n[1]='-';n[2]='o';n[3]='\0';
      // //roque pequeno
      break;
    case 2:			// n[0]='o';n[1]='-';n[2]='o';n[3]='-';n[4]='o';n[5]='\0'; 
      // 
      // //roque grande
      break;
    case 3:			// n[5]='.';n[6]='e';n[7]='p';n[8]='\0';
      // //comeu en passant
      break;
    case 4:			// n[5]='=';n[6]='D';n[7]='\0'; //promoveu 
      // 
      // a Dama
      m[4] = 'q';
      m[5] = '\0';
      break;
    case 5:			// n[5]='=';n[6]='C';n[7]='\0'; //promoveu 
      // 
      // a Cavalo
      m[4] = 'n';
      m[5] = '\0';
      break;
    case 6:			// n[5]='=';n[6]='T';n[7]='\0'; //promoveu 
      // 
      // a Torre
      m[4] = 'r';
      m[5] = '\0';
      break;
    case 7:			// n[5]='=';n[6]='B';n[7]='\0'; //promoveu 
      // 
      // a Bispo
      m[4] = 'b';
      m[5] = '\0';
      break;
    }				// falta uma variavel para Xeque!
  // m=strcpy(m,n); //comentei pois não lembrei para que serve
}

int
movi2lance (int *l, char *m)	// transforma char de entrada em int
{
  int i;

  /*
   * if(!xboard) { for(i=0;i<2;i++) //as coordenadas origem e destino
   * estao no tabuleiro
   * if(m[3*i]<'a'||m[3*i]>'h'||m[3*i+1]<'1'||m[3*i+1]>'8') //3i e 3i+1
   * return(0); for(i=0;i<2;i++) { l[2*i]=(int)(m[3*i]-'a');
   * //l[0]=m[0]-'a' l[2]=m[3]-'a' l[2*i+1]=(int)(m[3*i+1])-'1';
   * //l[1]=m[1]-'1' l[3]=m[4]-'1' } } else { 
   */
  for (i = 0; i < 2; i++)	// as coordenadas origem e destino estao
    // no tabuleiro
    if (m[2 * i] < 'a' || m[2 * i] > 'h' || m[2 * i + 1] < '1' || m[2 * i + 1] > '8')	// 2i 
      // 
      // e 
      // 2i+1
      return (0);
  for (i = 0; i < 2; i++)
    {
      l[2 * i] = (int) (m[2 * i] - 'a');	// l[0]=m[0]-'a'
      // l[2]=m[3]-'a'
      l[2 * i + 1] = (int) (m[2 * i + 1]) - '1';	// l[1]=m[1]-'1'
      // l[3]=m[4]-'1'
    }
  // ---- }
  return (1);
}

inline int
adv (int vez)
{
  return ((-1) * vez);
}

inline int
sinal (int x)
{
  if (!x)
    return (0);
  return (abs (x) / x);
}

void
copitab (tabuleiro * dest, tabuleiro * font)
{
  int i, j;
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      dest->tab[i][j] = font->tab[i][j];
  dest->vez = font->vez;
  dest->peao_pulou = font->peao_pulou;
  dest->roqueb = font->roqueb;
  dest->roquep = font->roquep;
  dest->empate_50 = font->empate_50;
  for (i = 0; i < 4; i++)
    dest->lancex[i] = font->lancex[i];
  dest->numero = font->numero;
}

movimento *
geramov (tabuleiro tabu)
{
  tabuleiro tabaux;
  movimento *pmovi, *pmoviant, *cabeca;
  int i, j, k, l, m, flag;	// i==col ; j==lin ;
  int col, lin;
  int peca;
  cabeca = (movimento *) malloc (sizeof (movimento));
  if (cabeca == NULL)
    {
      if (!xboard)
	printf ("\nErro de alocacao de memoria em geramov 1");
      exit (0);
    }
  pmovi = cabeca;
  pmoviant = pmovi;
  pmovi->prox = NULL;
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      {
	if ((tabu.tab[i][j]) * (tabu.vez) <= 0)	// casa vazia, ou
	  // cor errada:
	  continue;		// (+)*(-)==(-) , (-)*(+)==(-)
	peca = abs (tabu.tab[i][j]);
	switch (peca)
	  {
	  case REI:
	    for (col = i - 1; col <= i + 1; col++)	// Rei anda
	      // normalmente
	      for (lin = j - 1; lin <= j + 1; lin++)
		{
		  if (lin < 0 || lin > 7 || col < 0 || col > 7)
		    continue;	// casa de indice invalido
		  if (sinal (tabu.tab[col][lin]) == tabu.vez)
		    continue;	// casa possui peca da mesma cor
		  // ou o proprio rei
		  if (ataca (adv (tabu.vez), col, lin, tabu))
		    continue;	// adversario esta protegendo a
		  // casa
		  copitab (&tabaux, &tabu);
		  tabaux.tab[i][j] = VAZIA;
		  tabaux.tab[col][lin] = REI * tabu.vez;
		  if (!xeque_rei_das (tabu.vez, tabaux))
		    {
		      pmovi->lance[0] = i;
		      pmovi->lance[1] = j;
		      pmovi->lance[2] = col;
		      pmovi->lance[3] = lin;
		      pmovi->peao_pulou = -1;
		      pmovi->roque = 0;	// nao pode mais fazer
		      // roque. Mexeu Rei
		      pmovi->especial = 0;
		      if (tabu.tab[col][lin] == VAZIA)
			pmovi->flag_50 = 0;

		      else
			pmovi->flag_50 = 2;	// Rei capturou
		      // peca
		      // adversaria.
		      pmovi->prox = (movimento *) malloc (sizeof (movimento));
		      if (pmovi->prox == NULL)
			{
			  if (!xboard)
			    printf
			      ("\nErro de alocacao de memoria em geramov 2");
			  exit (0);
			}
		      pmoviant = pmovi;
		      pmovi = pmovi->prox;
		      pmovi->prox = NULL;
		    }
		}

	    // ----------------------------- Roque de Brancas e Pretas
	    if (tabu.vez == brancas)
	      if (tabu.roqueb == 0)	// Se nao pode mais rocar: 
		// 
		// break!
		break;

	      else		// roque de brancas
		{
		  if (tabu.roqueb != 2 && tabu.tab[7][0] == -TORRE)	// Nao 
		    // 
		    // mexeu 
		    // TR 
		    // (e 
		    // ela 
		    // está 
		    // lá! 
		    // Adv 
		    // poderia 
		    // ter 
		    // comido 
		    // antes 
		    // de 
		    // mexer)
		    {		// roque pequeno
		      if (tabu.tab[5][0] == VAZIA && tabu.tab[6][0] == VAZIA)	// f1,g1
			{
			  flag = 0;
			  for (k = 4; k < 7; k++)	// colunas e,f,g
			    if (ataca (pretas, k, 0, tabu))
			      {
				flag = 1;
				break;
			      }
			  if (flag == 0)
			    {
			      pmovi->lance[0] = 4;	// e
			      pmovi->lance[1] = 0;	// 1
			      pmovi->lance[2] = 6;	// g
			      pmovi->lance[3] = 0;	// 1
			      pmovi->peao_pulou = -1;	// nao eh
			      // pulo de 
			      // peao
			      pmovi->roque = 0;	// nao pode mais
			      // fazer roque.
			      pmovi->especial = 1;	// roque
			      // pequeno.
			      pmovi->flag_50 = 0;	// continua
			      // contando
			      pmovi->prox = (movimento *)
				malloc (sizeof (movimento));
			      if (pmovi->prox == NULL)
				{
				  if (!xboard)
				    printf
				      ("\nErro de alocacao de memoria em geramov 3");
				  exit (0);
				}
			      pmoviant = pmovi;
			      pmovi = pmovi->prox;
			      pmovi->prox = NULL;
			    }
			}	// roque grande
		    }		// mexeu TR
		  if (tabu.roqueb != 3 && tabu.tab[0][0] == -TORRE)	// Nao 
		    // 
		    // mexeu 
		    // TD 
		    // (e 
		    // a 
		    // torre 
		    // existe!)
		    {
		      if (tabu.tab[1][0] == VAZIA && tabu.tab[2][0] == VAZIA && tabu.tab[3][0] == VAZIA)	// b1,c1,d1
			{
			  flag = 0;
			  for (k = 2; k < 5; k++)	// colunas c,d,e
			    if (ataca (pretas, k, 0, tabu))
			      {
				flag = 1;
				break;
			      }
			  if (flag == 0)
			    {
			      pmovi->lance[0] = 4;	// e
			      pmovi->lance[1] = 0;	// 1
			      pmovi->lance[2] = 2;	// c
			      pmovi->lance[3] = 0;	// 1
			      pmovi->peao_pulou = -1;	// nao
			      // pulou
			      // peao
			      pmovi->roque = 0;	// nao pode mais
			      // rocar. Mexeu
			      // rei.
			      pmovi->especial = 2;	// roque
			      // grande.
			      pmovi->flag_50 = 0;	// continua
			      // contando
			      pmovi->prox = (movimento *)
				malloc (sizeof (movimento));
			      if (pmovi->prox == NULL)
				{
				  if (!xboard)
				    printf
				      ("\nErro de alocacao de memoria em geramov 4");
				  exit (0);
				}
			      pmoviant = pmovi;
			      pmovi = pmovi->prox;
			      pmovi->prox = NULL;
			    }
			}
		    }		// mexeu TD
		}

	    else		// vez das pretas jogarem
	    if (tabu.roquep == 0)	// Se nao pode rocar: break!
	      break;

	    else		// roque de pretas
	      {
		if (tabu.roquep != 2 && tabu.tab[7][7] == TORRE)	// Nao 
		  // 
		  // mexeu 
		  // TR 
		  // (e 
		  // a 
		  // torre 
		  // não 
		  // foi 
		  // capturada)
		  {		// roque pequeno
		    if (tabu.tab[5][7] == VAZIA && tabu.tab[6][7] == VAZIA)	// f8,g8
		      {
			flag = 0;
			for (k = 4; k < 7; k++)	// colunas e,f,g
			  if (ataca (brancas, k, 7, tabu))
			    {
			      flag = 1;
			      break;
			    }
			if (flag == 0)
			  {
			    pmovi->lance[0] = 4;	// e
			    pmovi->lance[1] = 7;	// 8
			    pmovi->lance[2] = 6;	// g
			    pmovi->lance[3] = 7;	// 8
			    pmovi->peao_pulou = -1;	// nao pulou peao
			    pmovi->roque = 0;	// nao pode mais
			    // rocar.Mexeu
			    // rei.
			    pmovi->especial = 1;	// roque pequeno.
			    pmovi->flag_50 = 0;	// continua
			    // contando.
			    pmovi->prox = (movimento *)
			      malloc (sizeof (movimento));
			    if (pmovi->prox == NULL)
			      {
				if (!xboard)
				  printf
				    ("\nErro de alocacao de memoria em geramov 5");
				exit (0);
			      }
			    pmoviant = pmovi;
			    pmovi = pmovi->prox;
			    pmovi->prox = NULL;
			  }
		      }		// roque grande.
		  }		// mexeu TR
		if (tabu.roquep != 3 && tabu.tab[0][7] == TORRE)	// Nao 
		  // 
		  // mexeu 
		  // TD 
		  // (e 
		  // a 
		  // torre 
		  // está 
		  // lá)
		  {
		    if (tabu.tab[1][7] == VAZIA && tabu.tab[2][7] == VAZIA && tabu.tab[3][7] == VAZIA)	// b8,c8,d8
		      {
			flag = 0;
			for (k = 2; k < 5; k++)	// colunas c,d,e
			  if (ataca (brancas, k, 7, tabu))
			    {
			      flag = 1;
			      break;
			    }
			if (flag == 0)
			  {
			    pmovi->lance[0] = 4;	// e
			    pmovi->lance[1] = 7;	// 8
			    pmovi->lance[2] = 2;	// c
			    pmovi->lance[3] = 7;	// 8
			    pmovi->peao_pulou = -1;	// nao pulou peao
			    pmovi->roque = 0;	// nao pode mais
			    // rocar.Mexeu
			    // rei.
			    pmovi->especial = 2;	// roque grande.
			    pmovi->flag_50 = 0;	// continua
			    // contando
			    pmovi->prox = (movimento *)
			      malloc (sizeof (movimento));
			    if (pmovi->prox == NULL)
			      {
				if (!xboard)
				  printf
				    ("\nErro de alocacao de memoria em geramov 6");
				exit (0);
			      }
			    pmoviant = pmovi;
			    pmovi = pmovi->prox;
			    pmovi->prox = NULL;
			  }
		      }
		  }		// mexeu TD
	      }
	    break;
	  case PEAO:

	    // peao comeu en passant
	    if (tabu.peao_pulou != -1)	// Existe peao do adv que
	      // pulou...
	      {
		if (i == tabu.peao_pulou - 1 || i == tabu.peao_pulou + 1)	// este 
		  // 
		  // peao 
		  // esta 
		  // na 
		  // coluna 
		  // certa
		  {
		    copitab (&tabaux, &tabu);	// tabaux = tabu
		    if (tabu.vez == brancas)
		      {
			if (j == 4)	// E tambem esta na linha certa.
			  // Peao branco vai comer
			  // enpassant!
			  {
			    tabaux.tab[tabu.peao_pulou][5] = -PEAO;
			    tabaux.tab[tabu.peao_pulou][4] = VAZIA;
			    tabaux.tab[i][4] = VAZIA;
			    if (!xeque_rei_das (brancas, tabaux))	// nao 
			      // 
			      // deixa 
			      // rei 
			      // branco 
			      // em 
			      // xeque
			      {
				pmovi->lance[0] = i;
				pmovi->lance[1] = j;
				pmovi->lance[2] = tabu.peao_pulou;
				pmovi->lance[3] = 5;
				pmovi->peao_pulou = -1;
				pmovi->roque = 1;	// se pudesse,
				// continua
				// podendo.
				pmovi->especial = 3;	// comeu
				// "en
				// passant"
				pmovi->flag_50 = 3;	// Peao comeu
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 7");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
			      }
			  }
		      }

		    else	// vez das pretas.
		      {
			if (j == 3)	// Esta na linha certa. Peao preto 
			  // 
			  // vai comer enpassant!
			  {
			    tabaux.tab[tabu.peao_pulou][2] = PEAO;
			    tabaux.tab[tabu.peao_pulou][3] = VAZIA;
			    tabaux.tab[i][3] = VAZIA;
			    if (!xeque_rei_das (pretas, tabaux))
			      {
				pmovi->lance[0] = i;
				pmovi->lance[1] = j;
				pmovi->lance[2] = tabu.peao_pulou;
				pmovi->lance[3] = 2;
				pmovi->peao_pulou = -1;
				pmovi->roque = 1;	// se pudesse,
				// continua
				// podendo.
				pmovi->especial = 3;	// comeu
				// "en
				// passant"
				pmovi->flag_50 = 3;	// Peao Comeu
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 8");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
			      }	// deixa rei em xeque
			  }	// nao esta na fila correta
		      }		// fim do else da vez
		  }		// nao esta na coluna adjacente ao
		// peao_pulou
	      }			// nao peao_pulou.
	    // peao andando uma casa
	    copitab (&tabaux, &tabu);	// tabaux = tabu;
	    if (tabu.vez == brancas)
	      {
		if (j + 1 < 8)
		  {
		    if (tabu.tab[i][j + 1] == VAZIA)
		      {
			tabaux.tab[i][j] = VAZIA;
			tabaux.tab[i][j + 1] = -PEAO;
			if (!xeque_rei_das (brancas, tabaux))
			  {
			    k = 4;	// 4:dama, 5:cavalo, 6:torre,
			    // 7:bispo
			  promoveb:pmovi->lance[0] =
			      i;
			    pmovi->lance[1] = j;
			    pmovi->lance[2] = i;
			    pmovi->lance[3] = j + 1;
			    pmovi->peao_pulou = -1;
			    pmovi->roque = 1;	// se pudesse,
			    // continua
			    // podendo.
			    pmovi->flag_50 = 1;	// movimento de
			    // peao
			    if (j + 1 == 7)	// se promoveu
			      {
				pmovi->especial = k;
				k++;
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 9");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
				if (k < 8)
				  goto promoveb;
			      }

			    else	// nao promoveu
			      {
				pmovi->especial = 0;
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 10");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
			      }
			  }	// deixa rei em xeque
		      }		// casa ocupada
		  }		// passou da casa de promocao e caiu do
		// tabuleiro!
	      }

	    else		// vez das pretas andar com peao uma casa
	      {
		if (j - 1 > -1)
		  {
		    if (tabu.tab[i][j - 1] == VAZIA)
		      {
			tabaux.tab[i][j] = VAZIA;
			tabaux.tab[i][j - 1] = PEAO;
			if (!xeque_rei_das (pretas, tabaux))
			  {
			    k = 4;	// 4:dama, 5:cavalo, 6:torre,
			    // 7:bispo
			  promovep:pmovi->lance[0] =
			      i;
			    pmovi->lance[1] = j;
			    pmovi->lance[2] = i;
			    pmovi->lance[3] = j - 1;
			    pmovi->peao_pulou = -1;	// este peao nao
			    // pulou
			    pmovi->roque = 1;	// se pudesse,
			    // continua
			    // podendo.
			    pmovi->flag_50 = 1;	// movimento de
			    // peao
			    if (j - 1 == 0)	// se promoveu
			      {
				pmovi->especial = k;
				k++;
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 11");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
				if (k < 8)
				  goto promovep;
			      }

			    else	// nao promoveu
			      {
				pmovi->especial = 0;
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 12");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
			      }
			  }	// deixa rei em xeque
		      }		// casa ocupada
		  }		// passou da casa de promocao e caiu do
		// tabuleiro!
	      }

	    // peao anda duas casas
	    copitab (&tabaux, &tabu);
	    if (tabu.vez == brancas)	// vez das brancas
	      {
		if (j == 1)	// esta na linha inicial
		  {
		    if (tabu.tab[i][2] == VAZIA && tabu.tab[i][3] == VAZIA)
		      {
			tabaux.tab[i][1] = VAZIA;
			tabaux.tab[i][3] = -PEAO;
			if (!xeque_rei_das (brancas, tabaux))
			  {
			    pmovi->lance[0] = i;	// coluna i tipo
			    // e2e4
			    pmovi->lance[1] = 1;	// linha 2
			    pmovi->lance[2] = i;	// coluna i
			    pmovi->lance[3] = 3;	// linha 4
			    pmovi->peao_pulou = i;	// peao pulou duas 
			    // 
			    // casas na coluna 
			    // i
			    pmovi->roque = 1;	// se pudesse,
			    // continua
			    // podendo.
			    pmovi->especial = 0;	// nada
			    // acontece...
			    pmovi->flag_50 = 1;	// movimento de
			    // peao
			    pmovi->prox = (movimento *)
			      malloc (sizeof (movimento));
			    if (pmovi->prox == NULL)
			      {
				if (!xboard)
				  printf
				    ("\nErro de alocacao de memoria em geramov 13");
				exit (0);
			      }
			    pmoviant = pmovi;
			    pmovi = pmovi->prox;
			    pmovi->prox = NULL;
			  }	// deixa rei em xeque
		      }		// alguma das casas esta ocupada
		  }		// este peao ja mexeu antes
	      }

	    else		// vez das pretas andar com peao 2 casas
	      {
		if (j == 6)	// esta na linha inicial
		  {
		    if (tabu.tab[i][5] == VAZIA && tabu.tab[i][4] == VAZIA)
		      {
			tabaux.tab[i][6] = VAZIA;
			tabaux.tab[i][4] = PEAO;
			if (!xeque_rei_das (pretas, tabaux))
			  {
			    pmovi->lance[0] = i;	// coluna i tipo
			    // e7e5
			    pmovi->lance[1] = 6;	// linha 7
			    pmovi->lance[2] = i;	// coluna i
			    pmovi->lance[3] = 4;	// linha 5
			    pmovi->peao_pulou = i;	// peao pulou duas 
			    // 
			    // na coluna i
			    pmovi->roque = 1;	// se pudesse,
			    // continua
			    // podendo.
			    pmovi->especial = 0;	// nada
			    // acontece...
			    pmovi->flag_50 = 1;	// movimento de
			    // peao
			    pmovi->prox = (movimento *)
			      malloc (sizeof (movimento));
			    if (pmovi->prox == NULL)
			      {
				if (!xboard)
				  printf
				    ("\nErro de alocacao de memoria em geramov 14");
				exit (0);
			      }
			    pmoviant = pmovi;
			    pmovi = pmovi->prox;
			    pmovi->prox = NULL;
			  }	// deixa rei em xeque
		      }		// alguma das casas esta ocupada
		  }		// este peao ja mexeu antes
	      }

	    // peao comeu normalmente (i.e., nao eh en passant)
	    copitab (&tabaux, &tabu);
	    if (tabu.vez == brancas)	// vez das brancas
	      {
		k = i - 1;
		if (k < 0)
		  k = i + 1;

		do		// diagonal esquerda e direita do peao.
		  {
		    if (tabu.tab[k][j + 1] > 0)	// peca preta
		      {
			tabaux.tab[i][j] = VAZIA;
			tabaux.tab[k][j + 1] = -PEAO;
			if (!xeque_rei_das (brancas, tabaux))
			  {
			    l = 4;	// 4:dama, 5:cavalo, 6:torre,
			    // 7:bispo
			  comoveb:	// peao branco promove comendo;
			    pmovi->lance[0] = i;
			    pmovi->lance[1] = j;
			    pmovi->lance[2] = k;
			    pmovi->lance[3] = j + 1;
			    pmovi->peao_pulou = -1;
			    pmovi->roque = 1;	// se pudesse,
			    // continua
			    // podendo.
			    pmovi->flag_50 = 3;	// Peao comeu.
			    if (j + 1 == 7)	// se promoveu comendo
			      {
				pmovi->especial = l;	// 4,5,6,7 
				// 
				// ==
				// D,C,T,Bispo
				l++;
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 15");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
				if (l < 8)
				  goto comoveb;
			      }

			    else	// comeu sem promover
			      {
				pmovi->especial = 0;
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 16");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
			      }
			  }	// deixa rei em xeque
		      }		// nao tem peca preta para comer
		    k += 2;	// proxima diagonal do peao
		  }
		while (k <= i + 1 && k < 8);
	      }			// vez das pretas comer normalmente (i.e., 
	    // 
	    // nao eh en passant)
	    else
	      {
		k = i - 1;
		if (k < 0)
		  k = i + 1;

		do		// diagonal esquerda e direita do peao.
		  {
		    if (tabu.tab[k][j - 1] < 0)	// peca branca
		      {
			tabaux.tab[i][j] = VAZIA;
			tabaux.tab[k][j - 1] = PEAO;
			if (!xeque_rei_das (pretas, tabaux))
			  {
			    l = 4;	// 4:dama, 5:cavalo, 6:torre,
			    // 7:bispo
			  comovep:	// peao preto promove comendo
			    pmovi->lance[0] = i;
			    pmovi->lance[1] = j;
			    pmovi->lance[2] = k;
			    pmovi->lance[3] = j - 1;
			    pmovi->peao_pulou = -1;
			    pmovi->roque = 1;	// se pudesse,
			    // continua
			    // podendo.
			    pmovi->flag_50 = 3;	// Peao e comeu.
			    if (j - 1 == 0)	// se promoveu comendo
			      {
				pmovi->especial = l;	// 4,5,6,7 
				// 
				// ==
				// D,C,T,Bispo
				l++;
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 17");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
				if (l < 8)
				  goto comovep;
			      }

			    else	// comeu sem promover
			      {
				pmovi->especial = 0;
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 18");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
			      }
			  }	// rei em xeque
		      }		// nao tem peao branco para comer
		    k += 2;
		  }
		while (k <= i + 1 && k < 8);
	      }
	    break;
	  case CAVALO:
	    for (col = -2; col < 3; col++)
	      for (lin = -2; lin < 3; lin++)
		{
		  if (abs (col) == abs (lin) || col == 0 || lin == 0)
		    continue;
		  if (i + col < 0 || i + col > 7 || j + lin < 0
		      || j + lin > 7)
		    continue;
		  if (sinal (tabu.tab[i + col][j + lin]) == tabu.vez)
		    continue;	// casa possui peca da mesma cor.
		  copitab (&tabaux, &tabu);
		  tabaux.tab[i][j] = VAZIA;
		  tabaux.tab[i + col][j + lin] = CAVALO * tabu.vez;
		  if (!xeque_rei_das (tabu.vez, tabaux))
		    {
		      pmovi->lance[0] = i;
		      pmovi->lance[1] = j;
		      pmovi->lance[2] = col + i;
		      pmovi->lance[3] = lin + j;
		      pmovi->peao_pulou = -1;	// nao e' pulo
		      // duplo do peao
		      pmovi->roque = 1;	// ainda pode fazer roque.
		      pmovi->especial = 0;	// nao e' roque,
		      // enpassant,
		      // promocao.
		      if (tabu.tab[col + i][lin + j] == VAZIA)
			pmovi->flag_50 = 0;

		      else
			pmovi->flag_50 = 2;	// Cavalo capturou 
		      // 
		      // peca
		      // adversaria.
		      pmovi->prox = (movimento *) malloc (sizeof (movimento));
		      if (pmovi->prox == NULL)
			{
			  if (!xboard)
			    printf
			      ("\nErro de alocacao de memoria em geramov 19");
			  exit (0);
			}
		      pmoviant = pmovi;
		      pmovi = pmovi->prox;
		      pmovi->prox = NULL;
		    }		// deixa rei em xeque
		}
	    break;
	  case DAMA:
	  case TORRE:
	  case BISPO:

	    // Anda reto - So nao o bispo
	    if (peca != BISPO)
	      {
		for (k = -1; k < 2; k += 2)	// k= -1, 1
		  {
		    col = i + k;
		    lin = j + k;	// l=flag da horizontal para a
		    // primeira peca que trombar
		    l = 0;
		    m = 0;	// m=idem para a vertical
		    do
		      {
			if (col >= 0 && col <= 7 && sinal (tabu.tab[col][j]) != tabu.vez && l == 0)	// gira 
			  // 
			  // col, 
			  // mantem 
			  // lin
			  {	// inclui esta casa na lista
			    copitab (&tabaux, &tabu);
			    tabaux.tab[i][j] = VAZIA;
			    tabaux.tab[col][j] = peca * tabu.vez;
			    if (!xeque_rei_das (tabu.vez, tabaux))
			      {
				pmovi->lance[0] = i;
				pmovi->lance[1] = j;
				pmovi->lance[2] = col;
				pmovi->lance[3] = j;
				pmovi->peao_pulou = -1;
				pmovi->roque = 1;	// por enquanto
				// pode fazer o
				// roque,
				if (peca == TORRE)	// mas, se for
				  // torre ...
				  {
				    if (tabu.vez == brancas && j == 0)	// se 
				      // 
				      // for 
				      // vez 
				      // das 
				      // brancas 
				      // e 
				      // linha 
				      // 1
				      {
					if (i == 0)	// e coluna a
					  pmovi->roque = 3;	// mexeu 
					// 
					// TD
					if (i == 7)	// ou coluna h
					  pmovi->roque = 2;	// mexeu 
					// 
					// TR
				      }
				    if (tabu.vez == pretas && j == 7)	// se 
				      // 
				      // for 
				      // vez 
				      // das 
				      // pretas 
				      // e 
				      // linha 
				      // 8
				      {
					if (i == 0)	// e coluna a
					  pmovi->roque = 3;	// mexeu 
					// 
					// TD
					if (i == 7)	// ou coluna h
					  pmovi->roque = 2;	// mexeu 
					// 
					// TR
				      }
				  }
				pmovi->especial = 0;
				if (tabu.tab[col][j] == VAZIA)
				  pmovi->flag_50 = 0;

				else
				  {
				    pmovi->flag_50 = 2;	// Dama ou 
				    // 
				    // Torre
				    // capturou 
				    // peca
				    // adversaria.
				    l = 1;
				  }
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 20");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
			      }

			    else	// deixa rei em xeque
			    if (tabu.tab[col][j] != VAZIA)	// alem de 
			      // 
			      // nao
			      // acabar
			      // o
			      // xeque,
			      // nao
			      // pode
			      // mais
			      // seguir
			      // nessa
			      // direção 
			      // pois
			      // tem
			      // peça
			      l = 1;
			  }

			else	// nao inclui mais nenhuma casa
			  // desta linha; A casa esta fora
			  // do tabuleiro, ou tem peca de
			  // mesma cor ou capturou
			  l = 1;
			if (lin >= 0 && lin <= 7 && sinal (tabu.tab[i][lin]) != tabu.vez && m == 0)	// gira 
			  // 
			  // lin, 
			  // mantem 
			  // col
			  {	// inclui esta casa na lista
			    copitab (&tabaux, &tabu);
			    tabaux.tab[i][j] = VAZIA;
			    tabaux.tab[i][lin] = peca * tabu.vez;
			    if (!xeque_rei_das (tabu.vez, tabaux))
			      {
				pmovi->lance[0] = i;
				pmovi->lance[1] = j;
				pmovi->lance[2] = i;
				pmovi->lance[3] = lin;
				pmovi->peao_pulou = -1;
				pmovi->roque = 1;	// ainda pode
				// fazer roque.
				if (peca == TORRE)	// mas, se for
				  // torre ...
				  {
				    if (tabu.vez == brancas && j == 0)	// se 
				      // 
				      // for 
				      // vez 
				      // das 
				      // brancas 
				      // e 
				      // linha 
				      // 1
				      {
					if (i == 0)	// e coluna a
					  pmovi->roque = 3;	// mexeu 
					// 
					// TD
					if (i == 7)	// ou coluna h
					  pmovi->roque = 2;	// mexeu 
					// 
					// TR
				      }
				    if (tabu.vez == pretas && j == 7)	// se 
				      // 
				      // for 
				      // vez 
				      // das 
				      // pretas 
				      // e 
				      // linha 
				      // 8
				      {
					if (i == 0)	// e coluna a
					  pmovi->roque = 3;	// mexeu 
					// 
					// TD
					if (i == 7)	// ou coluna h
					  pmovi->roque = 2;	// mexeu 
					// 
					// TR
				      }
				  }
				pmovi->especial = 0;
				if (tabu.tab[i][lin] == VAZIA)
				  pmovi->flag_50 = 0;

				else
				  {
				    pmovi->flag_50 = 2;	// Dama ou 
				    // 
				    // Torre
				    // capturou 
				    // peca
				    // adversaria.
				    m = 1;
				  }
				pmovi->prox = (movimento *)
				  malloc (sizeof (movimento));
				if (pmovi->prox == NULL)
				  {
				    if (!xboard)
				      printf
					("\nErro de alocacao de memoria em geramov 21");
				    exit (0);
				  }
				pmoviant = pmovi;
				pmovi = pmovi->prox;
				pmovi->prox = NULL;
			      }

			    else	// deixa rei em xeque
			    if (tabu.tab[i][lin] != VAZIA)	// alem de 
			      // 
			      // nao
			      // acabar
			      // o
			      // xeque,
			      // nao
			      // pode
			      // mais
			      // seguir
			      // nessa
			      // direção 
			      // pois
			      // tem
			      // peça
			      m = 1;
			  }

			else	// nao inclui mais nenhuma casa
			  // desta coluna; A casa esta fora
			  // do tabuleiro, ou tem peca de
			  // mesma cor ou capturou
			  m = 1;
			col += k;
			lin += k;
		      }
		    while (((col >= 0 && col <= 7)
			    || (lin >= 0 && lin <= 7)) && (m == 0 || l == 0));
		  }		// roda o incremento
	      }			// a peca era bispo
	    if (peca != TORRE)	// anda na diagonal
	      {
		col = i;
		lin = j;
		flag = 0;
		for (k = -1; k < 2; k += 2)
		  for (l = -1; l < 2; l += 2)
		    {
		      col += k;
		      lin += l;
		      while (col >= 0 && col <= 7 && lin >= 0 && lin <= 7
			     && sinal (tabu.tab[col][lin]) != tabu.vez
			     && flag == 0)
			{	// inclui 
			  // 
			  // esta 
			  // casa 
			  // na 
			  // lista
			  copitab (&tabaux, &tabu);
			  tabaux.tab[i][j] = VAZIA;
			  tabaux.tab[col][lin] = peca * tabu.vez;
			  if (!xeque_rei_das (tabu.vez, tabaux))
			    {
			      pmovi->lance[0] = i;
			      pmovi->lance[1] = j;
			      pmovi->lance[2] = col;
			      pmovi->lance[3] = lin;
			      pmovi->peao_pulou = -1;
			      pmovi->roque = 1;	// ainda pode
			      // fazer roque.
			      pmovi->especial = 0;
			      if (tabu.tab[col][lin] == VAZIA)
				pmovi->flag_50 = 0;

			      else
				{
				  pmovi->flag_50 = 2;	// Dama ou 
				  // 
				  // Bispo
				  // capturou 
				  // peca
				  // adversaria.
				  flag = 1;
				}
			      pmovi->prox = (movimento *)
				malloc (sizeof (movimento));
			      if (pmovi->prox == NULL)
				{
				  if (!xboard)
				    printf
				      ("\nErro de alocacao de memoria em geramov 22");
				  exit (0);
				}
			      pmoviant = pmovi;
			      pmovi = pmovi->prox;
			      pmovi->prox = NULL;
			    }

			  else	// deixa rei em xeque
			  if (tabu.tab[col][lin] != VAZIA)	// alem 
			    // 
			    // de 
			    // nao 
			    // acabar 
			    // o 
			    // xeque, 
			    // nao 
			    // pode 
			    // mais 
			    // seguir 
			    // nessa 
			    // direção 
			    // pois 
			    // tem 
			    // peça
			    flag = 1;
			  col += k;
			  lin += l;
			}	// rodou toda a diagonal, ate tab acabar
		      // ou achar peca da mesma cor ou capturar
		      col = i;
		      lin = j;
		      flag = 0;
		    }		// for
	      }			// peca era torre
	  case VAZIA:
	    break;
	  default:
	    if (!xboard)
	      printf ("Achei uma peca esquisita! Geramov, default.");
	    exit (0);
	  }			// switch(peca)
      }				// loop das pecas todas do tabuleiro
  if (cabeca == pmovi)		// se a lista esta vazia
    {
      free (cabeca);
      return (NULL);
    }

  else				// ordena e retorna! flag_50: 0-nada,
    // 1-peao andou, 2-captura, 3-peao
    // capturou
    {				// especial: 0-nada, roque 1-pqn, 2-grd,
      // 3-enpassant
      // 4,5,6,7 promocao de Dama, Cavalo, Torre, Bispo
      free (pmovi);
      pmoviant->prox = NULL;
      pmoviant = cabeca;
      pmovi = cabeca->prox;
      while (pmovi != NULL)	// percorre, passa para cabeca tudo de
	// interessante.
	{
	  if (pmovi->flag_50 > 1 || pmovi->especial)	// achou captura
	    // ou especial
	    {			// move para a cabeca da lista!
	      pmoviant->prox = pmovi->prox;	// o anterior aponta para
	      // o prox.
	      pmovi->prox = cabeca;	// o do meio aponta para o
	      // primeiro.
	      cabeca = pmovi;	// o cabeca agora eh pmovi!
	      pmovi = pmoviant->prox;
	    }

	  else
	    {
	      pmoviant = pmovi;
	      pmovi = pmovi->prox;
	    }
	}			// fim do while, e consequentemente da
      // lista
      return (cabeca);
    }				// fim do else da lista vazia.
}				// fim de ----------- movimento

				// *geramov(tabuleiro tabu)
int
ataca (int cor, int col, int lin, tabuleiro tabu)
{				// retorna verdadeiro (1) ou falso (0)
  // cor==brancas => brancas atacam casa(col,lin)
  // cor==pretas => pretas atacam casa(col,lin)
  int icol, ilin, casacol, casalin;

  // torre ou dama atacam a casa...
  for (icol = col - 1; icol >= 0; icol--)	// desce coluna
    {
      if (tabu.tab[icol][lin] == VAZIA)
	continue;
      if (cor == brancas)
	if (tabu.tab[icol][lin] == -TORRE || tabu.tab[icol][lin] == -DAMA)
	  return (1);

	else
	  break;

      else			// pretas atacam a casa
      if (tabu.tab[icol][lin] == TORRE || tabu.tab[icol][lin] == DAMA)
	return (1);

      else
	break;
    }
  for (icol = col + 1; icol < 8; icol++)	// sobe coluna
    {
      if (tabu.tab[icol][lin] == VAZIA)
	continue;
      if (cor == brancas)
	if (tabu.tab[icol][lin] == -TORRE || tabu.tab[icol][lin] == -DAMA)
	  return (1);

	else
	  break;

      else if (tabu.tab[icol][lin] == TORRE || tabu.tab[icol][lin] == DAMA)
	return (1);

      else
	break;
    }
  for (ilin = lin + 1; ilin < 8; ilin++)	// direita na linha
    {
      if (tabu.tab[col][ilin] == VAZIA)
	continue;
      if (cor == brancas)
	if (tabu.tab[col][ilin] == -TORRE || tabu.tab[col][ilin] == -DAMA)
	  return (1);

	else
	  break;

      else if (tabu.tab[col][ilin] == TORRE || tabu.tab[col][ilin] == DAMA)
	return (1);

      else
	break;
    }
  for (ilin = lin - 1; ilin >= 0; ilin--)	// esquerda na linha
    {
      if (tabu.tab[col][ilin] == VAZIA)
	continue;
      if (cor == brancas)
	if (tabu.tab[col][ilin] == -TORRE || tabu.tab[col][ilin] == -DAMA)
	  return (1);

	else
	  break;

      else if (tabu.tab[col][ilin] == TORRE || tabu.tab[col][ilin] == DAMA)
	return (1);

      else
	break;
    }

  // cavalo ataca casa...
  for (icol = -2; icol < 3; icol++)
    for (ilin = -2; ilin < 3; ilin++)
      {
	if (abs (icol) == abs (ilin) || icol == 0 || ilin == 0)
	  continue;
	if (col + icol < 0 || col + icol > 7 || lin + ilin < 0
	    || lin + ilin > 7)
	  continue;
	if (cor == brancas)
	  if (tabu.tab[col + icol][lin + ilin] == -CAVALO)
	    return (1);

	  else
	    continue;

	else if (tabu.tab[col + icol][lin + ilin] == CAVALO)
	  return (1);
      }

  // bispo ou dama atacam casa...
  for (icol = -1; icol < 2; icol += 2)
    for (ilin = -1; ilin < 2; ilin += 2)
      {
	casacol = col;		// para cada diagonal, comece na casa
	// origem.
	casalin = lin;

	do
	  {
	    casacol = casacol + icol;
	    casalin = casalin + ilin;
	    if (casacol < 0 || casacol > 7 || casalin < 0 || casalin > 7)
	      break;
	  }
	while (tabu.tab[casacol][casalin] == 0);
	if (casacol >= 0 && casacol <= 7 && casalin >= 0 && casalin <= 7)
	  if (cor == brancas)
	    if (tabu.tab[casacol][casalin] == -BISPO
		|| tabu.tab[casacol][casalin] == -DAMA)
	      return (1);

	    else
	      continue;		// achou peca, mas esta nao anda
	// em diagonal ou e' peca propria
	  else if (tabu.tab[casacol][casalin] == BISPO
		   || tabu.tab[casacol][casalin] == DAMA)
	    return (1);
      }				// proxima diagonal

  // ataque de rei...
  for (icol = col - 1; icol <= col + 1; icol++)
    for (ilin = lin - 1; ilin <= lin + 1; ilin++)
      {
	if (icol == col && ilin == lin)
	  continue;
	if (icol < 0 || icol > 7 || ilin < 0 || ilin > 7)
	  continue;
	if (cor == brancas)
	  if (tabu.tab[icol][ilin] == -REI)
	    return (1);

	  else
	    continue;

	else if (tabu.tab[icol][ilin] == REI)
	  return (1);
      }
  if (cor == brancas)		// ataque de peao branco
    {
      if (lin > 1)
	{
	  ilin = lin - 1;
	  if (col - 1 >= 0)
	    if (tabu.tab[col - 1][ilin] == -PEAO)
	      return (1);
	  if (col + 1 <= 7)
	    if (tabu.tab[col + 1][ilin] == -PEAO)
	      return (1);
	}
    }

  else				// ataque de peao preto
    {
      if (lin < 6)
	{
	  ilin = lin + 1;
	  if (col - 1 >= 0)
	    if (tabu.tab[col - 1][ilin] == PEAO)
	      return (1);
	  if (col + 1 <= 7)
	    if (tabu.tab[col + 1][ilin] == PEAO)
	      return (1);
	}
    }
  return (0);
}				// fim de int ataca(int cor, int col, int

				// lin, tabuleiro tabu)

int
xeque_rei_das (int cor, tabuleiro tabu)
{
  int ilin, icol;
  for (ilin = 0; ilin < 8; ilin++)	// roda linha
    for (icol = 0; icol < 8; icol++)	// roda coluna
      if (tabu.tab[icol][ilin] == (REI * cor))	// achou o rei
	if (ataca (adv (cor), icol, ilin, tabu))	// alguem ataca o
	  // rei
	  return (1);

	else
	  return (0);		// ninguem ataca o rei
  return (0);			// nao achou o rei!!
}				// fim de int xeque_rei_das(int cor,

				// tabuleiro tabu)

char
humajoga (tabuleiro * tabu)	// ----------------------------------------------
{
  char movinito[80];		// desisto
  movimento *pval;
  char peca, res, aux;
  int tente, lanc[4];
  int i, j, casacor;		// nao
  float lan;			// nao
  listab *plaux;		// nao
  lan = -0.4;			// nao
  movinito[79] = '\0';

  do
    {
      tente = 0;
      if (!xboard)
	printf ("\n\nQual o lance? ");
      scanf ("%s", movinito);	// ---------------- Toda entrada do
      // Winboard
      if (debug)
	fprintf (fsaida, "winboard: %s\n", movinito);
      if (!strcmp (movinito, "lista") && !xboard)
	{
	  mostra_lances (*tabu);
	  printf ("Qual o lance? ");
	  scanf ("%s", movinito);
	}
      if (!strcmp (movinito, "empate") || !strcmp (movinito, "draw"))
	{
	  if (primeiro == segundo)
	    return ('c');

	  else if (estatico (*tabu) > 20)
	    return ('c');

	  else
	    {
	      if (!xboard)
		{
		  printf ("Empate nao foi aceito. Jogue!");
		  getch ();
		  imptab (*tabu);
		}
	      tente = 1;
	      continue;
	    }
	}
      if (!strcmp (movinito, "versao") || !strcmp (movinito, "version"))
	{
	  if (!xboard)
	    {
	      printf
		("Xadreco versao 5.0 para WinBoard\nBaseado no Algoritmo Minimax.\n\n		Por Ruben Carlo Benante, 04/06/04.");
	      getch ();
	      imptab (*tabu);
	    }

	  else
	    {
	      if (debug)
		fprintf (fsaida,
			 "tellopponent Xadreco v5.0 para WinBoard, baseado no Algoritmo Minimax, por Ruben Carlo Benante, 04/06/04.\n");
	      printf
		("tellopponent Xadreco v5.0 para WinBoard, baseado no Algoritmo Minimax, por Ruben Carlo Benante, 04/06/04.\n");
	    }
	  tente = 1;
	  continue;
	}
      if (movinito[0] == 's' && movinito[1] == 'd' && xboard)
	{
	  nivel = (int) (movinito[3] - '0');
	  nivel = (nivel > 6 || nivel < 0) ? 2 : nivel;
	  tente = 1;
	  continue;
	}
      if (!strcmp (movinito, "nivel") && !xboard)
	{
	  printf
	    ("\nNivel atual:%d. Qual o novo nivel de dificuldade (1-6)? ",
	     nivel);
	  scanf ("%d", &nivel);
	  imptab (*tabu);
	  tente = 1;
	  continue;
	}
      if (!strcmp (movinito, "go") && xboard)	// troca de lado. o mesmo
	// que "adv"
	{
	  aux = primeiro;
	  primeiro = segundo;
	  segundo = aux;

	  // joga quem tem que jogar
	  // if(tabu->vez==brancas && primeiro=='c' || tabu->vez==pretas 
	  // 
	  // && segundo=='c')
	  return ('y');		// return(compjoga(tabu));
	}
      if (!strcmp (movinito, "adv") && !xboard)
	{
	  printf
	    ("\n\n\nQuem joga de brancas (c:Computador, h ou espaco:Humano)? ");
	  primeiro = getch ();
	  if (primeiro == ' ')
	    {
	      primeiro = 'h';
	      printf ("h");
	    }

	  else
	    printf ("%c", primeiro);
	  printf
	    ("\nQuem joga de  pretas (c ou espaco:Computador, h:Humano)? ");
	  segundo = getch ();
	  if (segundo == ' ')
	    {
	      segundo = 'c';
	      printf ("c");
	    }

	  else
	    printf ("%c", segundo);
	  if (primeiro == segundo && primeiro == 'c')
	    {
	      printf ("\n\nPausa entre os lances? (s ou espaco / n): ");
	      pausa = getch ();
	      if (pausa == ' ')
		pausa = 's';
	    }

	  else
	    pausa = 'n';

	  // joga quem tem que jogar
	  // if(tabu->vez==brancas && primeiro=='c' || tabu->vez==pretas 
	  // 
	  // && segundo=='c')
	  // return(compjoga(tabu));
	  // else
	  // {
	  // tente=1;
	  // continue;
	  // }
	  return ('y');		// se for comp. ou humano, gira o laço
	  // assim mesmo.
	}
      if (!strcmp (movinito, "1-0"))
	return ('V');		// vitoria das brancas
      if (!strcmp (movinito, "0-1"))
	return ('v');		// vitoria das pretas
      if (!strcmp (movinito, "1/2-1/2"))
	return ('j');		// empate
      if (!strcmp (movinito, "new"))
	return ('w');		// novo jogo
      if (!strcmp (movinito, "desisto") || !strcmp (movinito, "resign"))
	if (tabu->vez == pretas)
	  return ('b');		// pretas abandonam
	else
	  return ('B');		// brancas abandonam
      if (!strcmp (movinito, "exit") || !strcmp (movinito, "quit"))
	{
	  if (debug)
	    fclose (fsaida);
	  exit (0);
	}
      if (!strcmp (movinito, "mem") && !xboard)	// So Deus explica 
	// 
	// esta funcao
	// funcionar!
	{

	  // printf("Memoria livre: \%lu bytes\n"/*, (unsigned long)
	  // coreleft()*/);
	  // getch();
	  imptab (*tabu);
	  tente = 1;
	  continue;
	}
      if (!strcmp (movinito, "repete") && !xboard)
	{
	  plaux = plcabeca;
	  while (plaux != NULL)
	    {
	      lan += .5;

	      // textbackground(0); //preto
	      clrscr ();
	      printf ("\n\r  ");

	      // textbackground(1); //azul
	      printf ("+------------------------+");
	      for (j = 7; j > -1; j--)
		{
		  printf ("\n");

		  // textcolor(15); //branco
		  // textbackground(0); //preto
		  printf ("%d ", j + 1);

		  // textbackground(1); //azul
		  printf ("|");
		  for (i = 0; i < 8; i++)
		    {
		      if ((i + j) % 2)
			// textbackground(2); //casa branca
			casacor = 1;

		      else
			casacor = 2;

		      // textbackground(14); //casa preta
		      // if(plaux->tab[i][j]<0) //cor da peca: branca.
		      // if(plaux->vez==brancas &&
		      // plaux->tab[i][j]==-REI)
		      // textcolor(15 + 128); //rei branco pisca
		      // else
		      // textcolor(15); //peca branca
		      // else
		      // if(plaux->vez==pretas && plaux->tab[i][j]==REI)
		      // textcolor(0 + 128); //rei preto pisca
		      // else
		      // textcolor(0); //peca positiva: preta.
		      switch (plaux->tab[i][j])
			{
			case PEAO:
			  printf (" p ");
			  break;
			case CAVALO:
			  printf (" c ");
			  break;
			case BISPO:
			  printf (" b ");
			  break;
			case TORRE:
			  printf (" t ");
			  break;
			case DAMA:
			  printf (" d ");
			  break;
			case REI:
			  printf (" r ");
			  break;
			case -PEAO:
			  printf (" P ");
			  break;
			case -CAVALO:
			  printf (" C ");
			  break;
			case -BISPO:
			  printf (" B ");
			  break;
			case -TORRE:
			  printf (" T ");
			  break;
			case -DAMA:
			  printf (" D ");
			  break;
			case -REI:
			  printf (" R ");
			  break;
			case VAZIA:
			  if (casacor == 1)
			    printf (" . ");	// casa branca
			  else
			    printf (" * ");	// casa preta
			}
		    }

		  // textbackground(1); //azul
		  printf ("|");
		}

	      // textcolor(15);
	      // textbackground(0);
	      printf ("\n\r  ");

	      // textbackground(1); //azul
	      printf ("+------------------------+");

	      // textcolor(15);
	      // textbackground(0);
	      printf ("\n\r  ");
	      printf ("  a  b  c  d  e  f  g  h\n\n\r");

	      // textcolor(10);
	      // textbackground(0);
	      printf ("repetiu: %d vez(es). Lance num.: %.0f\n",
		      plaux->rep, lan);
	      printf ("\nTecle algo para prosseguir.");
	      getch ();
	      plaux = plaux->prox;
	    }			// fim do while
	  printf ("\nA lista de tabuleiros acabou. Tecle algo, c-comando.");
	  getch ();
	  imptab (*tabu);
	  res = situacao (*tabu);
	  if (res == 'x')
	    {
	      printf ("\nXeque !!!");
	      gotoxy (1, 14);
	    }
	  tente = 1;
	  continue;
	}			// comando repete
      if (!movi2lance (lanc, movinito))	// se nao existe este
	// lance
	{
	  if (!xboard)
	    {
	      if (strcmp (movinito, "lista") && strcmp (movinito, "adv") && strcmp (movinito, "go"))	// se 
		// 
		// nao 
		// eh 
		// o 
		// comando 
		// lista 
		// e 
		// nao 
		// eh 
		// adv
		{
		  printf
		    ("\nPara jogar por exemplo C3BD (notacao descritiva),");
		  printf (" digite assim: b1c3<enter>");
		  printf ("\nb1 : casa origem  --> coluna b, linha 1.");
		  printf ("\nc3 : casa destino --> coluna c, linha 3.");
		  printf ("\n\n'desisto' para desistir.");
		  printf ("\n'empate' para pedir empate.");
		  printf ("\n'versao' para saber mais sobre...");
		  printf ("\n'lista' para ver a lista de lances possiveis.");
		  printf
		    ("\n'nivel' para mudar o nivel de dificuldade atual.");
		  printf ("\n'cor' para trocar de cor com o adversario.");
		  printf ("\n'repete' para ver a lista de tabuleiros.");
		  printf ("\n'adv' para novos adversarios neste mesmo jogo.");
		  printf
		    ("\n'inclui' para adicionar esta abertura no livro.");
		  printf ("\n'partida' lista os lances desta partida.");
		  printf ("\n'exit' para abortar o programa.");
		  printf ("\n'mem' para avaliar a memoria disponivel.");
		  printf
		    ("\n                          Tecle <enter> para continuar.");
		  getch ();
		}
	      imptab (*tabu);
	    }

	  else
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: Illegal move: %s\n", movinito);
	      printf ("Illegal move: %s\n", movinito);
	    }
	  tente = 1;
	  continue;
	}
      pval = valido (*tabu, lanc);	// lanc eh int lanc[4]; cria pval
      // com tudo preenchido
      if (pval == NULL)
	{
	  if (xboard)
	    {
	      if (debug)
		fprintf (fsaida, "xadreco: Illegal move: %s\n", movinito);
	      printf ("Illegal move: %s\n", movinito);
	    }

	  else
	    {
	      printf ("\n\nMovimento Ilegal. Tente outra vez.\n");
	      getch ();
	      imptab (*tabu);
	      res = situacao (*tabu);
	      if (res == 'x' && !xboard)
		{
		  printf ("\nXeque !!!");
		  gotoxy (1, 14);
		}
	    }
	  tente = 1;
	  continue;
	}
    }
  while (tente);
  if (pval->especial > 3)	// promocao do peao 4,5,6,7: Dama,Cavalo,
    // Torre, Bispo
    {
      if (xboard)
	{
	  switch (movinito[4])	// exemplo: e7e8q
	    {
	    case 'q':
	      pval->especial = 4;
	      break;		// dama
	    case 'r':
	      pval->especial = 6;
	      break;		// torre
	    case 'b':
	      pval->especial = 7;
	      break;		// bispo
	    case 'n':
	      pval->especial = 5;
	      break;		// cavalo
	    default:
	      pval->especial = 4;
	      break;		// dama
	    }
	}

      else
	{

	  do
	    {
	      printf
		("\n\nO peao foi promovido. Escolha uma peca (d,t,b,c)? ");
	      peca = getche ();
	      switch (peca)
		{
		case 'd':
		  pval->especial = 4;
		  break;
		case 't':
		  pval->especial = 6;
		  break;
		case 'b':
		  pval->especial = 7;
		  break;
		case 'c':
		  pval->especial = 5;
		  break;
		default:
		  continue;
		}
	    }
	  while (0);		// falso sempre!
	}
    }
  // insere_listab(*tabu);
  res = joga_em (tabu, *pval);	// a funcao joga_em deve inserir no listab
  // lancex.lance[i]
  free (pval);
  return (res);			// vez da outra cor jogar. retorna a
  // situacao.
}				// fim do huma_joga---------------

movimento *
valido (tabuleiro tabu, int *lanc)
{
  movimento *pmovi, *loop, *auxloop;
  pmovi = geramov (tabu);
  loop = pmovi;
  while (pmovi != NULL)		// vai comparando e...
    {
      if (igual (pmovi->lance, lanc))
	break;			// o lance valido eh o cabeca da lista que 
      // 
      // restou
      pmovi = pmovi->prox;
      free (loop);		// ...liberando os diferentes
      loop = pmovi;
    }

  // apagar a lista que restou da memoria
  if (pmovi != NULL)		// o lance esta na lista, logo eh valido,
    {
      loop = pmovi->prox;	// apaga o restante da lista.
      pmovi->prox = NULL;
    }
  while (loop != NULL)		// ou loop==pmovi==NULL e o lance nao vale
    {				// ou loop==pmovi->prox e apaga o restante
      auxloop = loop->prox;
      free (loop);
      loop = auxloop;
    }
  return (pmovi);
}				// fim da valido

int
igual (int *l1, int *l2)	// dois lances[4] sao iguais
{
  int j;
  for (j = 0; j < 4; j++)
    if (l1[j] != l2[j])
      return (0);
  return (1);
}

char
situacao (tabuleiro tabu)
{

  // pega o tabuleiro e retorna: M,m,a,p,i,5,r,T,t,x
  // tabu.situa: retorno da situacao:
  // 3,4 M,m: xeque-mate (nas Brancas e nas Pretas respec.)
  // 1 a: empate por afogamento
  // 1 p: empate por xeque-perpetuo
  // 1 i: empate por insuficiencia de material
  // 1 5: empate apos 50 lances sem movimento de peao e captura
  // 1 r: empate apos repetir uma mesma posicao 3 vezes
  // 5,6 T,t: o tempo acabou (das Brancas e das Pretas respec.)
  // 2 x: xeque
  movimento *cabeca;
  int insuf_branca = 0, insuf_preta = 0;
  int i, j;
  listab *plaux;
  if (tabu.empate_50 > 50.0)	// empate apos 50 lances sem captura ou
    // movimento de peao
    return ('5');
  for (i = 0; i < 8; i++)	// insuficiencia de material
    for (j = 0; j < 8; j++)
      {
	switch (tabu.tab[i][j])
	  {
	  case -DAMA:
	  case -TORRE:
	  case -PEAO:
	    insuf_branca += 3;
	    break;
	  case DAMA:
	  case TORRE:
	  case PEAO:
	    insuf_preta += 3;
	    break;
	  case -BISPO:
	    insuf_branca += 2;
	    break;
	  case BISPO:
	    insuf_preta += 2;
	    break;
	  case -CAVALO:
	    insuf_branca++;
	    break;
	  case CAVALO:
	    insuf_preta++;
	    break;
	  }
	if (insuf_branca > 2 || insuf_preta > 2)
	  break;
      }
  if (insuf_branca < 3 && insuf_preta < 3)
    return ('i');
  plaux = plfinal;		// posicao repetiu 3 vezes
  while (plaux != NULL)
    {
      if (plaux->rep == 3)
	return ('r');
      plaux = plaux->ant;
    }
  cabeca = geramov (tabu);	// criar funcao gera1mov() retorna v ou f
  if (cabeca == NULL)		// Sem lances: Mate ou Afogamento.
    if (!xeque_rei_das (tabu.vez, tabu))
      return ('a');		// empate por afogamento.
    else if (tabu.vez == brancas)
      return ('M');		// as brancas estao em xeque-mate
    else
      return ('m');		// as pretas estao em xeque-mate
  else				// Tem lances...
    {
      libera_lances (cabeca);
      if (xeque_rei_das (tabu.vez, tabu))
	return ('x');
    }
  return (' ');
}


// ------------------------------- jogo do computador
// -----------------------
char
compjoga (tabuleiro * tabu)
{
  char command[10], res, com;
  int i, j, casacor;		// nao
  float lan;			// nao
  listab *plaux;		// nao
  lan = -0.4;			// nao
  if (pausa == 'c' && !xboard)
    {

      do
	{
	  printf ("\ncomando:");
	  scanf ("%s", &command);
	  if (!strcmp (command, "exit"))
	    exit (0);

	  else if (!strcmp (command, "desista"))
	    if (tabu->vez == brancas)
	      return ('B');

	    else
	      return ('b');

	  else if (!strcmp (command, "lista"))
	    {
	      mostra_lances (*tabu);
	      com = getch ();
	    }

	  else if (!strcmp (command, "repete"))
	    {
	      plaux = plcabeca;
	      while (plaux != NULL)
		{
		  lan += .5;

		  // textbackground(0); //preto
		  clrscr ();
		  printf ("\n\r  ");

		  // textbackground(1); //azul
		  printf ("+------------------------+");
		  for (j = 7; j > -1; j--)
		    {
		      printf ("\n");

		      // textcolor(15); //branco
		      // textbackground(0); //preto
		      printf ("%d ", j + 1);

		      // textbackground(1); //azul
		      printf ("|");
		      for (i = 0; i < 8; i++)
			{
			  if ((i + j) % 2)
			    // textbackground(2); //casa branca
			    casacor = 1;

			  else
			    casacor = 2;

			  // textbackground(14); //casa preta
			  // if(plaux->tab[i][j]<0) //cor da peca:
			  // branca.
			  // if(plaux->vez==brancas &&
			  // plaux->tab[i][j]==-REI)
			  // textcolor(15 + 128); //rei branco pisca
			  // else
			  // textcolor(15); //peca branca
			  // else
			  // if(plaux->vez==pretas &&
			  // plaux->tab[i][j]==REI)
			  // textcolor(0 + 128); //rei preto pisca
			  // else
			  // textcolor(0); //peca positiva: preta.
			  switch (plaux->tab[i][j])
			    {
			    case PEAO:
			      printf (" p ");
			      break;
			    case CAVALO:
			      printf (" c ");
			      break;
			    case BISPO:
			      printf (" b ");
			      break;
			    case TORRE:
			      printf (" t ");
			      break;
			    case DAMA:
			      printf (" d ");
			      break;
			    case REI:
			      printf (" r ");
			      break;
			    case -PEAO:
			      printf (" P ");
			      break;
			    case -CAVALO:
			      printf (" C ");
			      break;
			    case -BISPO:
			      printf (" B ");
			      break;
			    case -TORRE:
			      printf (" T ");
			      break;
			    case -DAMA:
			      printf (" D ");
			      break;
			    case -REI:
			      printf (" R ");
			      break;
			    case VAZIA:
			      if (casacor == 1)
				printf (" . ");	// casa branca
			      else
				printf (" * ");	// casa preta
			    }
			}

		      // textbackground(1); //azul
		      printf ("|");
		    }

		  // textcolor(15);
		  // textbackground(0);
		  printf ("\n\r  ");

		  // textbackground(1); //azul
		  printf ("+------------------------+");

		  // textcolor(15);
		  // textbackground(0);
		  printf ("\n\r  ");
		  printf ("  a  b  c  d  e  f  g  h\n\n\r");

		  // textcolor(10);
		  // textbackground(0);
		  printf ("repetiu: %d vez(es). Lance num.: %.0f\n",
			  plaux->rep, lan);
		  printf ("\nTecle algo para prosseguir.");
		  getch ();
		  plaux = plaux->prox;
		}		// fim do while
	      printf
		("\nA lista de tabuleiros acabou. Tecle algo, c-comando.");
	      com = getch ();
	    }			// comando repete
	  else if (!strcmp (command, "nivel"))
	    {
	      printf
		("\nNivel atual:%d. Qual o novo nivel de dificuldade (1-6)? ",
		 nivel);
	      scanf ("%d", &nivel);
	      printf ("\nTecle algo, c-comando.");
	      com = getch ();
	    }

	  else
	    {
	      printf ("\nOs comandos sao:\n");
	      printf
		("	lista  : mostra os movimentos possiveis e o melhor lance.\n");
	      printf ("	desista: determina o fim deste jogo.\n");
	      printf ("	exit   : termino anormal do programa.\n");
	      printf ("	repete : mostra a lista de tabuleiros.\n");
	      printf ("	nivel  : muda o nivel de dificuldade atual.\n");
	      printf ("\nTecle algo para prosseguir, c-comando");
	      com = getch ();
	    }
	}
      while (com == 'c');	// outro comando
    }				// variavel pausa!='c'
  libera_lances (result.plance);
  result.plance = NULL;		// nao eh necessario
  result.valor = 0;
  profflag = 0;
  if (!xboard && 1 == 0)
    {
      gotoxy (30, 1);
      printf (":)");
    }
  // if(xboard) //está pedindo para jogar?
  minimax (*tabu, 0, 10001, -10001);	// retorna o melhor caminho a
  // partir de tab...
  if (result.plance == NULL)	// ...profundidade zero, limite maximo de
    // estatico
    // ...limite minimo de estatico
    return ('e');		// erro! computador sem lances!? nivel
  // deve ser > zero
  res = joga_em (tabu, *result.plance);

  // insere_listab(*tabu); //insere o lance jogado na lista de repeteco
  return (res);			// vez da outra cor jogar. retorna a
  // situacao(*tabu)
}


// --------------------------------------------------------------------------
void
minimax (tabuleiro atual, int prof, int uso, int passo)
{
  movimento *succ, *melhor_caminho, *cabeca_succ;
  int novo_valor;
  tabuleiro tab;
  if (!xboard && 1 == 0)
    {
      gotoxy (31, 1);
      gira++;
      if (gira == (unsigned char) 242)
	gira = (unsigned char) 1;
      switch (gira)
	{
	case (unsigned char) 1:
	  printf ("(");
	  break;
	case (unsigned char) 61:
	  printf ("|");
	  break;
	case (unsigned char) 121:
	  printf (")");
	  break;
	case (unsigned char) 181:
	  printf ("|");
	  break;
	}
    }
  succ = NULL;
  melhor_caminho = NULL;
  cabeca_succ = NULL;
  if (profsuf (prof))		// profundidade suficiente
    {
      result.plance = NULL;	// coloque um nulo no ponteiro plance
      result.valor = estatico (atual);
      return;
    }
  succ = geramov (atual);	// succ==NULL se alguem ganhou ou empatou
  if (succ == NULL)
    {
      result.plance = NULL;
      result.valor = estatico (atual);	// entao o estatico refletira
      // isso: "pat"
      return;
    }
  cabeca_succ = succ;
  while (succ != NULL)		// laco para analisar todos sucessores
    {
      copitab (&tab, &atual);
      joga_em (&tab, *succ);	// joga o lance atual, a funcao joga_em
      // deve inserir no listab
      profflag = succ->flag_50;	// flag_50== 2 ou 3 : houve
      // captura :Liberou
      switch (tab.situa)
	{
	case 0:
	  break;		// 0:nada... Quem decide eh flag_50;
	case 2:
	  profflag = 2;
	  break;		// 2:Xeque! Liberou
	default:
	  profflag = 0;		// 1:Empate, 3,4:Mate ou 5,6:Tempo. Nao
	  // pode passar o nivel
	}
      minimax (tab, prof + 1, -passo, -uso);	// analisa o novo
      // tabuleiro
      retira_listab ();
      novo_valor = -result.valor;
      if (novo_valor > passo /* || (novo_valor==passo && rand()%10>7) */ )
	{
	  passo = novo_valor;
	  libera_lances (melhor_caminho);
	  melhor_caminho = copimel (*succ, result.plance);	// cria
	  // nova
	  // cabeca
	}
      succ = succ->prox;
    }
  result.valor = passo;
  result.plance = copilistmov (melhor_caminho);	// funcao retorna
  // uma cabeca nova
  libera_lances (melhor_caminho);
  libera_lances (cabeca_succ);
}

int
profsuf (int prof)
{
  if (prof > nivel)		// se ja passou do nivel estipulado, pare
    // a busca incondicionalmente
    return 1;
  if (prof == nivel && profflag < 2)	// se esta no nivel estipulado e
    // nao liberou
    return 1;			// a profundidade ja eh sufuciente
  return 0;			// se OU Nem-Chegou-no-Nivel OU Liberou,
  // pode ir fundo
}

char
joga_em (tabuleiro * tabu, movimento movi)
{
  int i;
  char res;
  if (movi.especial == 7)	// promocao do peao: BISPO
    tabu->tab[movi.lance[0]][movi.lance[1]] = BISPO * tabu->vez;
  if (movi.especial == 6)	// promocao do peao: TORRE
    tabu->tab[movi.lance[0]][movi.lance[1]] = TORRE * tabu->vez;
  if (movi.especial == 5)	// promocao do peao: CAVALO
    tabu->tab[movi.lance[0]][movi.lance[1]] = CAVALO * tabu->vez;
  if (movi.especial == 4)	// promocao do peao: DAMA
    tabu->tab[movi.lance[0]][movi.lance[1]] = DAMA * tabu->vez;
  if (movi.especial == 3)	// comeu en passant
    tabu->tab[tabu->peao_pulou][movi.lance[1]] = VAZIA;
  if (movi.especial == 2)	// roque grande
    {
      tabu->tab[0][movi.lance[1]] = VAZIA;
      tabu->tab[3][movi.lance[1]] = TORRE * tabu->vez;
    }
  if (movi.especial == 1)	// roque pequeno
    {
      tabu->tab[7][movi.lance[1]] = VAZIA;
      tabu->tab[5][movi.lance[1]] = TORRE * tabu->vez;
    }
  if (movi.flag_50)		// empate de 50 lances sem mover peao ou
    // capturar
    tabu->empate_50 = 0;

  else
    tabu->empate_50 += .5;
  if (tabu->vez == brancas)
    switch (movi.roque)		// avalia o que este lance causa para
      // futuros roques
      {
      case 0:
	tabu->roqueb = 0;
	break;			// mexeu o rei
      case 1:
	break;			// nao fez lance que interfere o roque
      case 2:
	if (tabu->roqueb == 1 || tabu->roqueb == 2)	// mexeu torre do
	  // rei
	  tabu->roqueb = 2;

	else
	  tabu->roqueb = 0;
	break;
      case 3:
	if (tabu->roqueb == 1 || tabu->roqueb == 3)	// mexeu torre da
	  // dama
	  tabu->roqueb = 3;

	else
	  tabu->roqueb = 0;
	break;
      }

  else				// vez das pretas
    switch (movi.roque)		// avalia o que este lance causa para
      // futuros roques
      {
      case 0:
	tabu->roquep = 0;
	break;			// mexeu o rei
      case 1:
	break;			// nao fez lance que interfere o roque
      case 2:
	if (tabu->roquep == 1 || tabu->roquep == 2)	// mexeu torre do
	  // rei
	  tabu->roquep = 2;

	else
	  tabu->roquep = 0;
	break;
      case 3:
	if (tabu->roquep == 1 || tabu->roquep == 3)	// mexeu torre da
	  // dama
	  tabu->roquep = 3;

	else
	  tabu->roquep = 0;
	break;
      }
  tabu->peao_pulou = movi.peao_pulou;
  tabu->tab[movi.lance[2]][movi.lance[3]] =
    tabu->tab[movi.lance[0]][movi.lance[1]];
  tabu->tab[movi.lance[0]][movi.lance[1]] = VAZIA;
  for (i = 0; i < 4; i++)
    tabu->lancex[i] = movi.lance[i];
  if (tabu->vez == brancas)
    tabu->numero++;
  tabu->vez = adv (tabu->vez);
  insere_listab (*tabu);	// insere o lance no listab
  res = situacao (*tabu);
  switch (res)
    {
    case 'a':			// afogado
    case 'p':			// perpetuo
    case 'i':			// insuficiencia
    case '5':			// 50 lances
    case 'r':
      tabu->situa = 1;
      break;			// repeticao
    case 'x':
      tabu->situa = 2;
      break;			// xeque
    case 'M':
      tabu->situa = 3;
      break;			// Brancas perderam por Mate
    case 'm':
      tabu->situa = 4;
      break;			// Pretas perderam por mate
    case 'T':
      tabu->situa = 5;
      break;			// Brancas perderam por Tempo
    case 't':
      tabu->situa = 6;
      break;			// Pretas perderam por tempo
    default:
      tabu->situa = 0;		// nada...
    }
  return (res);
}				// fim da joga_em


// copia pmovi no comeco da lista *plan, e retorna a cabeca desta NOVA
// lista (mel)
movimento *
copimel (movimento ummovi, movimento * plan)
{
  movimento *mel;
  mel = (movimento *) malloc (sizeof (movimento));
  if (mel == NULL)
    {
      if (!xboard)
	printf ("\nErro de alocacao de memoria em copimel 1");
      exit (0);
    }
  copimov (mel, &ummovi);
  copilistmovmel (mel, plan);
  return (mel);
}


// copia do segundo lance para frente, mantendo o primeiro
void
copilistmovmel (movimento * dest, movimento * font)
{
  movimento *loop;
  if (font == dest)
    {
      if (!xboard)
	printf ("Funcao copilistmovmel precisa de duas listas DIFERENTES.");
      exit (0);
    }
  if (font == NULL)
    {
      dest->prox = NULL;
      return;
    }
  dest->prox = (movimento *) malloc (sizeof (movimento));
  if (dest->prox == NULL)
    {
      if (!xboard)
	printf ("\nErro de alocacao de memoria em copilistmovmel 1");
      exit (0);
    }
  loop = dest->prox;
  while (font != NULL)
    {
      copimov (loop, font);
      font = font->prox;
      if (font != NULL)
	{
	  loop->prox = (movimento *) malloc (sizeof (movimento));
	  if (loop->prox == NULL)
	    {
	      if (!xboard)
		printf ("\nErro de alocacao de memoria em copilistmovmel 2");
	      exit (0);
	    }
	  loop = loop->prox;
	}
    }
  loop->prox = NULL;
}

void
copimov (movimento * dest, movimento * font)	// nao compia o ponteiro
						// prox
{
  int i;
  for (i = 0; i < 4; i++)
    dest->lance[i] = font->lance[i];
  dest->peao_pulou = font->peao_pulou;
  dest->roque = font->roque;
  dest->flag_50 = font->flag_50;
  dest->especial = font->especial;
}

movimento *
copilistmov (movimento * font)	// cria nova lista de movimentos para
				// destino
{
  movimento *cabeca, *pmovi;
  if (font == NULL)
    return NULL;
  cabeca = (movimento *) malloc (sizeof (movimento));	// valor novo que
  // sera do
  // result.plance
  if (cabeca == NULL)
    {
      if (!xboard)
	printf ("\nErro ao alocar memoria em copilistmov 1");
      exit (0);
    }
  pmovi = cabeca;
  while (font != NULL)
    {
      copimov (pmovi, font);
      font = font->prox;
      if (font != NULL)
	{
	  pmovi->prox = (movimento *) malloc (sizeof (movimento));
	  if (pmovi->prox == NULL)
	    {
	      if (!xboard)
		printf ("\nErro ao alocar memoria em copilistmov 2");
	      exit (0);
	    }
	  pmovi = pmovi->prox;
	}
    }
  pmovi->prox = NULL;
  return (cabeca);
}

int
estatico (tabuleiro tabu)
{
  int totb = 0, totp = 0;
  int i, j, cor, peca;
  int peaob, peaop, isob, isop;

  // levando em conta a situacao do tabuleiro
  switch (tabu.situa)
    {
    case 3:			// Brancas tomaram mate
      if (tabu.vez == brancas)
	return -10000;		// ‚ vez das brancas, mas
      else			// elas estao em xeque-mate. Conclusao:
	return +10000;		// pessimo negocio, ganha -10000.
    case 4:			// Pretas tomaram mate
      if (tabu.vez == pretas)
	return -10000;

      else
	return +10000;
    case 1:			// Empatou
      return 0;			// valor de uma posicao empatada.
    case 2:			// A posicao eh de XEQUE!
      if (tabu.vez == brancas)
	totp++;

      else
	totb++;

      // break;
      // default: break;
    }

  // levando em conta o valor material de cada peca, e a quantidade de
  // ataques nelas
  // falta explicar que Peca Maior atacada por Peca Menor nao deve ser
  // defendida e sim retirada.
  // ou seja: nao so o numero de ataques importam, mas a ordem tambem!
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      {
	peca = tabu.tab[i][j];
	if (peca == VAZIA)
	  continue;
	if (peca < 0)		// se peca branca:
	  {
	    if (tabu.vez == brancas || qataca (brancas, i, j, tabu) >= qataca (pretas, i, j, tabu))	// vez 
	      // 
	      // das 
	      // brancas 
	      // ou 
	      // defesab>=ataquep
	      totb += abs (peca);	// pecas brancas: total=total +
	    // material. So soma se nao
	    // estiver pendurada!
	  }

	else			// se peca preta:
	if (tabu.vez == pretas || qataca (pretas, i, j, tabu) >= qataca (brancas, i, j, tabu))	// vez 
	  // 
	  // das 
	  // pretas 
	  // ou 
	  // defesap>=ataqueb
	  totp += peca;		// pecas pretas: total=total + material.
	// So soma se nao estiver pendurada!
      }

  // avaliando os peoes avancados
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      {
	if (tabu.tab[i][j] == VAZIA)
	  continue;
	if (tabu.tab[i][j] == -PEAO)	// Peao branco
	  {
	    if (i > 3)		// faltando 3 casas ou menos para promover 
	      // 
	      // ganha +1
	      totb += 1;
	    if (i > 4)
	      totb += 3;	// faltando 2 casas ou menos para promover 
	    // 
	    // ganha +1+3
	  }
	if (tabu.tab[i][j] == PEAO)
	  {
	    if (i < 4)
	      totp += 1;	// faltando 3 casas ou menos para promover 
	    // 
	    // ganha +1
	    if (i < 3)
	      totp += 3;	// faltando 2 casas ou menos para promover 
	    // 
	    // ganha +1+3
	  }
      }

  // peao dobrado e isolado.
  isob = 1;
  isop = 1;			// Maquina de Estados de isob
  for (j = 1; j < 7; j++)	// isob|achou peao na col| nao achou
    {				// 0 | isob = 0 | isob = 1
      peaob = 0;		// inicio: 1 | isob = 2 | isob = 1
      peaop = 0;		// 2 | isob = 0 | isob = 3
      for (i = 0; i < 8; i++)	// PEAO ISOLADO! 3 | isob = 2 | isob = 1
	{
	  if (tabu.tab[i][j] == VAZIA)
	    continue;
	  if (tabu.tab[i][j] == -PEAO)
	    peaob++;
	  if (tabu.tab[i][j] == PEAO)
	    peaop++;
	}			// peaob e peaop tem o total de peoes na
      // coluna (um so, dobrado, trip...)
      if (peaob != 0)
	totb -= ((peaob - 1) * 2);	// penalidade: para um so=0, para
      // dobrado=2, trip=4!
      if (peaop != 0)
	totp -= ((peaop - 1) * 2);
      switch (isob)
	{
	case 0:
	  if (!peaob)		// se nao achou, peaob==0
	    isob++;		// isob=1, se achou isob continua 0
	  break;
	case 1:
	  if (peaob)		// se achou, peaob !=0
	    isob++;		// isob=2, se nao achou isob continua 1
	  break;
	case 2:
	  if (peaob)		// se achou...
	    isob = 0;		// isob volta a zero
	  else			// senao...
	    isob++;		// isob=3 => PEAO ISOLADO!
	  break;
	}
      if (isob == 3)
	{
	  totb -= 3;		// penalidade para peao isolado
	  isob = 1;
	}
      switch (isop)
	{
	case 0:
	  if (!peaop)		// se nao achou, peaop==0
	    isop++;
	  break;
	case 1:
	  if (peaop)		// se achou, peaop !=0
	    isop++;
	  break;
	case 2:
	  if (peaop)		// se achou...
	    isop = 0;

	  else			// senao...
	    isop++;		// isop=3 => PEAO ISOLADO!
	  break;
	}
      if (isop == 3)
	{
	  totp -= 3;		// penalidade para peao isolado
	  isop = 1;
	}
    }

  // controle do centro
  for (cor = -1; cor < 2; cor += 2)
    for (i = 2; i < 6; i++)	// c,d,e,f
      for (j = 2; j < 6; j++)	// 3,4,5,6
	if (ataca (cor, i, j, tabu))
	  if (cor == brancas)
	    {
	      totb++;
	      if ((i == 3 || i == 4) && (j == 3 || j == 4))
		totb++;		// mais pontos para d4,d5,e4,e5
	    }

	  else
	    {
	      totp++;
	      if ((i == 3 || i == 4) && (j == 3 || j == 4))
		totp++;		// mais pontos para d4,d5,e4,e5
	    }
  // bonificacao para quem nao mexeu a dama na abertura
  if (tabu.numero < 6)
    {
      if (tabu.tab[3][0] == -DAMA)
	totb += 5;
      if (tabu.tab[3][7] == DAMA)
	totp += 5;
    }
  // bonificacao para quem fez roque na abertura
  if (tabu.numero < 11)
    {
      if (tabu.tab[6][0] == -REI && tabu.tab[5][0] == -TORRE)	// brancas 
	// 
	// com
	// roque
	// pequeno
	totb += 7;
      if (tabu.tab[2][0] == -REI && tabu.tab[3][0] == -TORRE)	// brancas 
	// 
	// com
	// roque
	// grande
	totb += 5;
      if (tabu.tab[6][7] == REI && tabu.tab[5][7] == TORRE)	// pretas
	// com
	// roque
	// pequeno
	totp += 7;
      if (tabu.tab[2][7] == REI && tabu.tab[3][7] == TORRE)	// pretas
	// com
	// roque
	// grande
	totp += 5;
    }
  // bonificacao para rei protegido na abertura com os peoes do Escudo
  // Real
  if (tabu.numero < 16)
    {
      if (tabu.tab[6][0] == -REI && tabu.tab[5][1] == tabu.tab[6][1] == tabu.tab[7][1] == -PEAO && tabu.tab[7][0] != -TORRE)	// brancas 
	// 
	// com 
	// roque 
	// pequeno
	totb += 5;
      if (tabu.tab[2][0] == -REI && tabu.tab[1][1] == tabu.tab[2][1] == -PEAO)	// brancas 
	// 
	// com 
	// roque 
	// grande
	totb += 3;
      if (tabu.tab[6][7] == REI && tabu.tab[5][6] == tabu.tab[6][6] == tabu.tab[7][6] == PEAO && tabu.tab[7][7] != TORRE)	// pretas 
	// 
	// com 
	// roque 
	// pequeno
	totp += 5;
      if (tabu.tab[2][7] == REI && tabu.tab[1][6] == tabu.tab[2][6] == PEAO)	// pretas 
	// 
	// com 
	// roque 
	// grande
	totp += 3;
    }
  // penalidade se mexer o peao do bispo, cavalo ou da torre no comeco
  // da abertura!
  if (tabu.numero < 8)
    {

      // caso das brancas------------------
      if (tabu.tab[5][1] == VAZIA)	// PBR
	totb -= 5;
      if (tabu.tab[6][1] == VAZIA)	// PCR
	totb -= 4;
      if (tabu.tab[7][1] == VAZIA)	// PTR
	totb -= 3;
      if (tabu.tab[0][1] == VAZIA)	// PTD
	totb -= 3;
      if (tabu.tab[1][1] == VAZIA)	// PCD
	totb -= 4;

      // if(tabu.tab[2][1]==VAZIA) //PBD nao eh penalizado!
      // totb-=1;
      // caso das pretas-------------------
      if (tabu.tab[5][6] == VAZIA)	// PBR
	totp -= 5;
      if (tabu.tab[6][6] == VAZIA)	// PCR
	totp -= 4;
      if (tabu.tab[7][6] == VAZIA)	// PTR
	totp -= 3;
      if (tabu.tab[0][6] == VAZIA)	// PTD
	totp -= 3;
      if (tabu.tab[1][6] == VAZIA)	// PCD
	totp -= 4;

      // if(tabu.tab[2][6]==VAZIA) //PBD nao eh penalizado!
      // totp-=1;
    }
  // prepara o retorno:----------------
  if (tabu.vez == brancas)	// se vez das brancas
    return totb - totp;		// retorna positivo se as brancas estao
  // ganhando (ou negativo c.c.)
  else				// se vez das brancas
    return totp - totb;		// retorna positivo se as brancas estao
  // ganhando (ou negativo c.c.)
}

void
insere_listab (tabuleiro tabu)
{
  int i, j, flag;
  listab *plaux;
  if (plcabeca == NULL)
    {
      plcabeca = (listab *) malloc (sizeof (listab));
      if (plcabeca == NULL)
	{
	  if (!xboard)
	    printf ("Erro ao alocar memoria em insere_listab 1");
	  exit (0);
	}
      plfinal = plcabeca;
      plcabeca->ant = NULL;
      plfinal->prox = NULL;
      for (i = 0; i < 8; i++)
	for (j = 0; j < 8; j++)
	  plfinal->tab[i][j] = tabu.tab[i][j];
      plfinal->vez = tabu.vez;
      plfinal->rep = 1;
    }

  else
    {
      plfinal->prox = (listab *) malloc (sizeof (listab));
      if (plfinal->prox == NULL)
	{
	  if (!xboard)
	    printf ("Erro ao alocar memoria em insere_listab 2");
	  exit (0);
	}
      plaux = plfinal;
      plfinal = plfinal->prox;
      plfinal->ant = plaux;
      plfinal->prox = NULL;

      // ponteiros prox e ant corretos, agora acertar as outras
      // variaveis da estrutura: tab, vez, rep
      for (i = 0; i < 8; i++)
	for (j = 0; j < 8; j++)
	  plfinal->tab[i][j] = tabu.tab[i][j];
      plfinal->vez = tabu.vez;

      // compara o inserido agora com todos os anteriores...
      plaux = plfinal->ant;
      while (plaux != NULL)
	{
	  flag = 0;
	  for (i = 0; i < 8; i++)
	    {
	      for (j = 0; j < 8; j++)
		{
		  if (plfinal->tab[i][j] != plaux->tab[i][j])
		    flag = 1;	// flag==1:tabuleiro diferente nao 
		  // 
		  // eh posicao repetida.
		  if (flag)	// flag == 1?
		    break;
		}
	      if (flag)
		break;
	    }
	  if (plfinal->vez != plaux->vez)
	    flag = 1;		// flag==1: vez diferente nao eh posicao
	  // repetida
	  if (!flag)		// flag == 0?
	    {

	      // printf("Esta posicao repetiu: %d.",plaux->rep+1);
	      // getch();
	      plfinal->rep = plaux->rep + 1;
	      break;
	    }
	  plaux = plaux->ant;
	}
      if (flag)			// flag == 1?
	// {
	// printf("Primeira vez que vejo essa!");
	// getch();
	plfinal->rep = 1;

      // }
    }
}
void
retira_listab (void)		// retira o plfinal;
{
  listab *plaux;
  if (plfinal == NULL)
    return;
  plaux = plfinal->ant;
  free (plfinal);
  if (plaux == NULL)
    {
      plcabeca = NULL;
      plfinal = NULL;
      return;
    }
  plfinal = plaux;
  plfinal->prox = NULL;
}

void
retira_tudo_listab (void)	// zera a lista de tabuleiros
{
  while (plcabeca != NULL)
    retira_listab ();
}

int
qataca (int cor, int col, int lin, tabuleiro tabu)
{				// retorna o numero de ataques que a "cor" 
  // 
  // faz na casa(col,lin)
  // cor==brancas => brancas atacam casa(col,lin)
  // cor==pretas => pretas atacam casa(col,lin)
  // quem= 0:ninguem, +1:P, +2:BouC, +4:T, +8:D, +16:R
  // 3:BouC,P; 5:T,P; 6:T,BouC; 7:T,BouC,P; 9:D,P; 10:D,BouC;
  // 11:D,BouC,P;
  // 12:D,T; 13:D,T,P; 14:D,T,BouC; 15:D,T,BouC,P; 17:R,P; 18:R,BouC;
  // 19:R,BouC,P; 20:R,T; 21:R,T,P; 22:R,T,BouC; 23:R,T,BouC,P;
  // 24:R,D; 25:R,D,P; 26:R,D,BouC; 27:R,D,BouC,P; 28:R,D,T;
  // 29:R,D,T,P; 30:R,D,T,BouC; 31:R,D,T,BouC,P;
  int icol, ilin, casacol, casalin;
  int total = 0;

  // torre ou dama atacam a casa...
  for (icol = col - 1; icol >= 0; icol--)	// desce coluna
    {
      if (tabu.tab[icol][lin] == VAZIA)
	continue;
      if (cor == brancas)
	if (tabu.tab[icol][lin] == -TORRE || tabu.tab[icol][lin] == -DAMA)
	  {
	    total++;
	    break;
	  }

	else
	  break;

      else			// pretas atacam a casa
      if (tabu.tab[icol][lin] == TORRE || tabu.tab[icol][lin] == DAMA)
	{
	  total++;
	  break;
	}

      else
	break;
    }
  for (icol = col + 1; icol < 8; icol++)	// sobe coluna
    {
      if (tabu.tab[icol][lin] == VAZIA)
	continue;
      if (cor == brancas)
	if (tabu.tab[icol][lin] == -TORRE || tabu.tab[icol][lin] == -DAMA)
	  {
	    total++;
	    break;
	  }

	else
	  break;

      else if (tabu.tab[icol][lin] == TORRE || tabu.tab[icol][lin] == DAMA)
	{
	  total++;
	  break;
	}

      else
	break;
    }
  for (ilin = lin + 1; ilin < 8; ilin++)	// direita na linha
    {
      if (tabu.tab[col][ilin] == VAZIA)
	continue;
      if (cor == brancas)
	if (tabu.tab[col][ilin] == -TORRE || tabu.tab[col][ilin] == -DAMA)
	  {
	    total++;
	    break;
	  }

	else
	  break;

      else if (tabu.tab[col][ilin] == TORRE || tabu.tab[col][ilin] == DAMA)
	{
	  total++;
	  break;
	}

      else
	break;
    }
  for (ilin = lin - 1; ilin >= 0; ilin--)	// esquerda na linha
    {
      if (tabu.tab[col][ilin] == VAZIA)
	continue;
      if (cor == brancas)
	if (tabu.tab[col][ilin] == -TORRE || tabu.tab[col][ilin] == -DAMA)
	  {
	    total++;
	    break;
	  }

	else
	  break;

      else			// pecas pretas atacam
      if (tabu.tab[col][ilin] == TORRE || tabu.tab[col][ilin] == DAMA)
	{
	  total++;
	  break;
	}

      else
	break;
    }

  // cavalo ataca casa...
  for (icol = -2; icol < 3; icol++)
    for (ilin = -2; ilin < 3; ilin++)
      {
	if (abs (icol) == abs (ilin) || icol == 0 || ilin == 0)
	  continue;
	if (col + icol < 0 || col + icol > 7 || lin + ilin < 0
	    || lin + ilin > 7)
	  continue;
	if (cor == brancas)	// cavalo branco ataca?
	  if (tabu.tab[col + icol][lin + ilin] == -CAVALO)
	    total++;		// sim,ataca!
	  else
	    continue;

	else			// cavalo preto ataca?
	if (tabu.tab[col + icol][lin + ilin] == CAVALO)
	  total++;		// sim,ataca!
      }

  // bispo ou dama atacam casa...
  for (icol = -1; icol < 2; icol += 2)
    for (ilin = -1; ilin < 2; ilin += 2)
      {
	casacol = col;		// para cada diagonal, comece na casa
	// origem.
	casalin = lin;

	do
	  {
	    casacol = casacol + icol;
	    casalin = casalin + ilin;
	    if (casacol < 0 || casacol > 7 || casalin < 0 || casalin > 7)
	      break;
	  }
	while (tabu.tab[casacol][casalin] == 0);
	if (casacol >= 0 && casacol <= 7 && casalin >= 0 && casalin <= 7)
	  if (cor == brancas)
	    if (tabu.tab[casacol][casalin] == -BISPO
		|| tabu.tab[casacol][casalin] == -DAMA)
	      {
		total++;
		continue;	// proxima diagonal
	      }

	    else
	      continue;		// achou peca, mas esta nao anda
	// em diagonal ou e' peca propria
	  else			// ataque de peca preta
	  if (tabu.tab[casacol][casalin] == BISPO
		|| tabu.tab[casacol][casalin] == DAMA)
	    {
	      total++;
	      continue;		// proxima diagonal
	    }
      }				// proxima diagonal

  // ataque de rei...
  for (icol = col - 1; icol <= col + 1; icol++)
    for (ilin = lin - 1; ilin <= lin + 1; ilin++)
      {
	if (icol == col && ilin == lin)
	  continue;
	if (icol < 0 || icol > 7 || ilin < 0 || ilin > 7)
	  continue;
	if (cor == brancas)
	  if (tabu.tab[icol][ilin] == -REI)
	    {
	      total++;
	      break;
	    }

	  else
	    continue;

	else if (tabu.tab[icol][ilin] == REI)
	  {
	    total++;
	    break;
	  }
      }
  if (cor == brancas)		// ataque de peao branco
    {
      if (lin > 1)
	{
	  ilin = lin - 1;
	  if (col - 1 >= 0)
	    if (tabu.tab[col - 1][ilin] == -PEAO)
	      total++;
	  if (col + 1 <= 7)
	    if (tabu.tab[col + 1][ilin] == -PEAO)
	      total++;
	}
    }

  else				// ataque de peao preto
    {
      if (lin < 6)
	{
	  ilin = lin + 1;
	  if (col - 1 >= 0)
	    if (tabu.tab[col - 1][ilin] == PEAO)
	      total++;
	  if (col + 1 <= 7)
	    if (tabu.tab[col + 1][ilin] == PEAO)
	      total++;
	}
    }
  return (total);
}

void
gotoxy (int x, int y)
{
}

void
clrscr (void)
{
}
