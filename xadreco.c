/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* %% Xadreco versao 0.2 - Jogo de Xadrez / Chess engine                     */
/* %% Descricao:                                                             */
/* %%   Versao 0.2 reescrita a partir da original historica 0.1, para linux  */
/* %%   Jogo de xadrez Xadreco compativel com protocolo Xboard               */
/* %% Autor: Ruben Carlo Benante (C) Copyright 1994-2018                     */
/* %% E-mail do autor: rcb@beco.cc                                           */
/* %% Licenca e Direitos Autorais segundo normas do codigo-livre (GNU/GPLv2) */
/* %% Arquivo: xadreco.c                                                     */
/* %% Tecnica de IA: Sorteio da peca (sem IA)                                */
/* %% Web page: http://www.github.com/drbeco/xadreco                         */
/* %% Criacao: 19/08/1994 (v0.1)                                             */
/* %% Primeira versao: 27/03/97                                              */
/* %% Reescrita: 2018-06-24 (v0.2...)                                        */
/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* %% This program is free software; you can redistribute it and/or          */
/* %% modify it under the terms of the GNU General Public License            */
/* %% as published by the Free Software Foundation; either version 2         */
/* %% of the License, or (at your option) any later version.                 */
/* %%                                                                        */
/* %% This program is distributed in the hope that it will be useful,        */
/* %% but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/* %% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/* %% GNU General Public License for more details.                           */
/* %%                                                                        */
/* %% You should have received a copy of the GNU General Public License      */
/* %% along with this program; if not, write to the Free Software            */
/* %% Foundation, Inc., 59 Temple Place - S.330, Boston, MA 02111-1307, USA. */
/* %%                                                                        */
/* %% If you redistribute it and/or modify it, you need to                   */
/* %% cite the original source-code and the author's name.                   */
/* %% ---------------------------------------------------------------------- */
/* %% Este programa e software livre; voce pode redistribui-lo e/ou          */
/* %% modifica-lo sob os termos da Licenca Publica Geral GNU, conforme       */
/* %% publicada pela Free Software Foundation; tanto a versao 2 da           */
/* %% Licenca como (a seu criterio) qualquer versao mais nova.               */
/* %%                                                                        */
/* %% Este programa e distribuido na expectativa de ser util, mas SEM        */
/* %% QUALQUER GARANTIA; sem mesmo a garantia implicita de                   */
/* %% COMERCIALIZACAO ou de ADEQUACAO A QUALQUER PROPOSITO EM                */
/* %% PARTICULAR. Consulte a Licenca Publica Geral GNU para obter mais       */
/* %% detalhes.                                                              */
/* %%                                                                        */
/* %% Voce deve ter recebido uma copia da Licenca Publica Geral GNU          */
/* %% junto com este programa; se nao, escreva para a Free Software          */
/* %% Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA               */
/* %% 02111-1307, USA.                                                       */
/* %%                                                                        */
/* %% Se voce redistribuir ou modificar este programa, voce deve citar       */
/* %% o codigo-fonte original e o nome do autor.                             */

/* ------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ------------------------------------------------------------------------- */
int valido(int p);
char situacao(int vez);
int estatico(int p);
int movi2lance(char *s);
void lance2movi(char *s);
int peca_ataca(int col, int lin, int prof);
void imptab(void);
int rei_xeque(int humano_joga);

/* ------------------------------------------------------------------------- */
enum
{
    RC = 200, DC = 40, TC = 25, BC = 16, CC = 15, PC = 5, VZ = 0, PH = -5, CH =
        -15, BH = -16, TH = -25, DH = -40, RH = -200
};
int tab[8][8];
char primeiro = 'h', segundo = 'c';
int nivel = 0;
int lance[4] = { 0, 0, 0, 0 };
char vez;
int ENPASSANT, ANDOURH, ANDOURC, ANDOUTHR, ANDOUTHD, ANDOUTCR, ANDOUTCD;

/* ------------------------------------------------------------------------- */
int main(void)
{
    char humajoga(void);
    char compjoga(void);
    char resultado;
    int i, j;
    randomize();

    do
    {
        clrscr();

        /*  inicializar variaveis do roque e enpassant */
        ANDOURH = ANDOURC = ANDOUTHR = ANDOUTHD = ANDOUTCR = ANDOUTCD = 0;
        ENPASSANT = -1;
        for(i = 0; i < 8; i++)
            for(j = 2; j < 6; j++)
                tab[i][j] = VZ;
        for(i = 0; i < 8; i++)
        {
            tab[i][1] = PH;
            tab[i][6] = PC;
        }
        tab[0][0] = tab[7][0] = TH;
        tab[0][7] = tab[7][7] = TC;
        tab[1][0] = tab[6][0] = CH;
        tab[1][7] = tab[6][7] = CC;
        tab[2][0] = tab[5][0] = BH;
        tab[2][7] = tab[5][7] = BC;
        tab[3][0] = DH;
        tab[4][0] = RH;
        tab[3][7] = DC;
        tab[4][7] = RC;
        printf("\n\n\nQuem comeca? (c/h): ");
        primeiro = getchar();
        printf("\n\nQual a dificuldade (1-6): ");
        scanf("%d", &nivel);
        if(primeiro == 'c')
        {
            for(i = 0; i < 8; i++)
                for(j = 0; j < 8; j++)
                    tab[i][j] = tab[i][j] * (-1);
            segundo = +5;
        }
        printf("\n\n\n");
        vez = primeiro;
        imptab();
joga_novamente:
        if(vez == 'c')
            resultado = compjoga();
        else
            resultado = humajoga();
        imptab();
        switch(resultado)
        {
            case 'M':
                printf("\nAs brancas estao em xequemate");
                break;
            case 'm':
                printf("\nAs pretas estao em xequemate");
                break;
            case 'a':
                printf("\nEmpate por afogamento");
                break;
            case 'p':
                printf("\nEmpate por xeque perpetuo");
                break;
            case 'c':
                printf("\nEmpate por comum acordo");
                break;
            case 'i':
                printf("\nEmpate por insuficiencia de material");
                break;
            case '5':
                printf("\nEmpate apos 50 lances sem movimento de Peao");
                break;
            case 'r':
                printf("\nEmpate por repeticao da mesma posicao por tres vezes");
                break;
            case 'T':
                printf("\nAs brancas perderam por tempo");
                break;
            case 't':
                printf("\nAs pretas perderam por tempo");
                break;
            case 'B':
                printf("\nAs brancas abandonam.");
                break;
            case 'b':
                printf("\nAs pretas abandonam.");
                break;
            default:
                goto joga_novamente;
        }
        printf("\nQuer jogar de novo (s/n): ");
        vez = getchar();
    }
    while(vez == 's');
}

/* ------------------------------------------------------------------------- */
void imptab(void)
{
    int col, lin;
    int colmax, colmin, linmax, linmin, inc;
    char *movinito = "desisto.";
    linmin = 7;
    linmax = -1;
    inc = -1;
    colmin = 0;
    colmax = 8;
    if(primeiro == 'c')
    {
        linmin = 0;
        linmax = 8;
        inc = 1;
        colmin = 7;
        colmax = -1;
    }
    textbackground(0);
    clrscr();
    cprintf("\n\r  ");
    textcolor(5);
    textbackground(1);
    cprintf("                          ");
    lin = linmin;

    do				/* for(lin=linmin;lin>=linmax;lin+=inc) */
    {
        textcolor(15);
        textbackground(0);
        cprintf("\n\r%d ", lin + 1);
        textcolor(5);
        textbackground(1);
        cprintf(" ");
        col = colmin;

        do			/* for(col=colmin;col<=colmax;col-=inc) */
        {
            if((lin + col) % 2)
                textbackground(2);	/* casa marrom */
            else
                textbackground(14);	/* casa amarela */
            if(tab[col][lin] < 0)	/* cor da peca */
                if(primeiro == 'h')	/* peca do humano, e humano de brancas */
                    textcolor(15);	/* peca branca */
                else
                    textcolor(0);	/* peca preta */
            else
                if(primeiro == 'c')
                    textcolor(15);
                else
                    textcolor(0);
            switch(abs(tab[col][lin]))
            {
                case 5:
                    cprintf(" p ");
                    break;
                case 15:
                    cprintf(" c ");
                    break;
                case 16:
                    cprintf(" b ");
                    break;
                case 25:
                    cprintf(" t ");
                    break;
                case 40:
                    cprintf(" d ");
                    break;
                case 200:
                    cprintf(" R ");
                    break;
                default:
                    cprintf("   ");
                    break;
            }
            col -= inc;
        }
        while(col != colmax);
        textbackground(1);
        cprintf(" ");
        lin += inc;
    }
    while(lin != linmax);
    textcolor(15);
    textbackground(0);
    cprintf("\n\r  ");
    textcolor(5);
    textbackground(1);
    cprintf("                          ");
    textcolor(15);
    textbackground(0);
    cprintf("\n\r  ");
    if(primeiro == 'h')
        cprintf("  a  b  c  d  e  f  g  h\n\n\r");

    else
        cprintf("  h  g  f  e  d  c  b  a\n\n\r");
    textcolor(6);
    textbackground(0);
    lance2movi(movinito);
    cprintf("Lance executado : %c%c%c%c", movinito[0], movinito[1],
            movinito[2], movinito[3]);
}

/* ------------------------------------------------------------------------- */
char humajoga(void)
{
    char *movinito = "        ";
    int val;
    char peca;

    do
    {
        printf("\n\nQual o lance? ");
        scanf("%s", movinito);
        if(!strcmp(movinito, "empate?"))
            if(estatico(0) <= -65)
                return ('c');
            else
            {
                printf("\nEmpate nao foi aceito. Jogue!");
                getchar();
                imptab();
                val = 4;
                continue;
            }
        if(!strcmp(movinito, "desisto."))
            if(primeiro == 'c')
                return ('b');		/* pretas abandonam */
            else
                return ('B');		/* brancas abandonam */
        if(!movi2lance(movinito))
        {
            printf("\nPara jogar por exemplo C3BD (notacao descritiva),");
            printf(" digite assim: b1c3<enter>");
            printf("\nb1 : casa origem  --> coluna b, linha 1.");
            printf("\nc3 : casa destino --> coluna c, linha 3.");
            printf("\ndigite: 'desisto.' para desistir");
            printf(", e 'empate?' para pedir empate.\n\n");
            printf("\n\n Tecle <enter> para continuar.");
            getchar();
            imptab();
            val = 4;
            continue;
        }
        val = valido(1);		/* 1 e' impar: peca humana deve mecher. */
        switch(val)
        {
            case 1:
                printf("\n\nA peca nao se move desta maneira.\n");
                break;
            case 2:
                printf("\n\nMovimento deixa o rei em xeque.\n");
                break;
            case 3:
                printf("\n\nCasa esta ocupada por outra peca da mesma cor.\n");
                break;
            case 4:
                printf("\n\nNao ha peca na casa indicada como origem.\n");
                break;
            case 5:
                printf("\n\nEsta peca nao lhe pertence.\n");
                break;
            case 6:
                ENPASSANT = lance[0];
                break;
            case 7:
                ANDOUTHR = 1;
                break;
            case 8:
                ANDOUTHD = 1;
                break;
            case 9:
                ANDOUTCR = 1;
                break;
            case 10:
                ANDOUTCD = 1;
                break;
            case 11:
                ANDOURC = 1;
                break;
            case 12:
                ANDOURH = 1;
                break;
            case 13:
                do
                {
                    printf
                    ("\n\nO peao foi promovido. Escolha uma peca (d,t,b,c)? ");
                    peca = getchar();
                    switch(peca)
                    {
                        case 'd':
                            tab[lance[0]][lance[1]] = -40;
                            break;
                        case 't':
                            tab[lance[0]][lance[1]] = -25;
                            break;
                        case 'b':
                            tab[lance[0]][lance[1]] = -16;
                            break;
                        case 'c':
                            tab[lance[0]][lance[1]] = -15;
                            break;
                        default:
                            continue;
                    }
                }
                while(0);
                break;

            /*    case 14: ANDOURC=ANDOUTCR=1;
                         tab[7][lance[1]]=0;
                         tab[5][lance[1]]=25;
                         break;
                case 15: ANDOURC=ANDOUTCD=1;
                         tab[0][lance[1]]=0;
                         tab[3][lance[1]]=25;
                         break;                   Roque do computador */
            case 16:
                ANDOURH = ANDOUTHR = 1;
                tab[7][lance[1]] = 0;
                tab[5][lance[1]] = -25;
                break;
            case 17:
                ANDOURH = ANDOUTHD = 1;
                tab[0][lance[1]] = 0;
                tab[3][lance[1]] = -25;
                break;
            case 18:
                tab[ENPASSANT][lance[1]] = VZ;
                break;
        }
        if(val < 6)
        {
            getchar();
            imptab();
        }
    }
    while(val < 6);
    if(val != 6)
        ENPASSANT = -1;
    tab[lance[2]][lance[3]] = tab[lance[0]][lance[1]];
    tab[lance[0]][lance[1]] = VZ;
    vez = 'c';
    return (situacao(0));	/* vez do computador jogar, por isso o: zero. */
}

/* ------------------------------------------------------------------------- */
int valido(int prof)
{
    /*  duvida: retorna (0 ou 1) ou um status dos lances? */
    int peca, casa, peca_do_comp, humano_joga, icol, ilin, casacol, casalin;
    int movi = 19;		/* menor ou igual a 5: erros. de 6 a 18: status. */
    int tabu[8][8];
    peca = tab[lance[0]][lance[1]];
    if(peca == VZ)
        return (4);			/* nao ha peca na casa origem. */
    humano_joga = prof % 2;	/* se prof impar =>humano joga. prof%2==1; */
    peca_do_comp = peca > 0 ? 1 : 0;
    if(!((humano_joga || peca_do_comp) && !(humano_joga && peca_do_comp)))
        return (5);			/*  se humano_joga xor peca_do_comp: (A ou B) e nao (A e B). */
    /*  erro 5: pegou peca   humano_joga     => peca negativa. */
    /*  da cor errada        computador joga => peca positiva. */
    if(tab[lance[2]][lance[3]])
        if(!((peca_do_comp || tab[lance[2]][lance[3]] > 0) && !(peca_do_comp && tab[lance[2]][lance[3]] > 0)))
            return (3);		/*  se nao(origem_posit xor destino_posit) */
    /*  casa ocupada destino por peca de mesma cor. */

    /*  fazer erro de peca nao move assim. return(1) */
    peca = abs(peca);
    switch(peca)			/*  esta funcao so e' usada para ver se um lance e' valido. */
    {
        case 200:
            if(lance[0] == 4 && lance[2] == 6)	/* Roque pequeno tentando ser feito. */
            {
                if((lance[1] != 0 && lance[1] != 7) || lance[1] != lance[3])
                    return (1);
                if(!humano_joga)	/*  se a vez e' do computador. */
                {
                    if(ANDOURC)
                        return (1);
                    if(ANDOUTCR)
                        return (1);
                    if(primeiro == 'c')

                    {
                        if(peca_ataca(4, 0, 1))	/*  Rei branco do comp esta em xeque. */
                            return (1);
                        if(tab[5][0] || tab[6][0])	/* Tem pecas entre rei e torre. */
                            return (1);
                    }

                    else		/*  primeiro e' o humano. Comp joga com as pretas. */
                    {
                        if(peca_ataca(4, 7, 1))	/*  Rei preto do comp esta em xeque. */
                            return (1);
                        if(tab[5][7] || tab[6][7])	/* Tem pecas entre rei e torre pretos. */
                            return (1);
                    }
                    movi = 14;	/* roque pequeno do comp. */
                }
                else			/* vez e' do humano */
                {
                    if(ANDOURH)
                        return (1);
                    if(ANDOUTHR)
                        return (1);
                    if(primeiro == 'h')
                    {
                        if(peca_ataca(4, 0, 0))	/*  Rei branco do huma esta em xeque. */
                            return (1);
                        if(tab[5][0] || tab[6][0])	/* Tem pecas entre rei e torre. */
                            return (1);
                    }
                    else		/*  primeiro e' o comp. Huma joga com as pretas. */
                    {
                        if(peca_ataca(4, 7, 0))	/*  Rei preto do comp esta em xeque. */
                            return (1);
                        if(tab[5][7] || tab[6][7])	/* Tem pecas entre rei e torre pretos. */
                            return (1);
                    }
                    movi = 16;	/* roque pequeno do humano. */
                }			/*  fim do else vez e' do humano. */
            }
            if(lance[0] == 4 && lance[2] == 2)	/* Roque grande tentando ser feito. */
            {
                if((lance[1] != 0 && lance[1] != 7) || lance[1] != lance[3])
                    return (1);
                if(!humano_joga)	/*  vez e' do computador */
                {
                    if(ANDOURC)
                        return (1);
                    if(ANDOUTCD)
                        return (1);
                    if(primeiro == 'c')
                    {
                        if(peca_ataca(4, 0, 1))	/*  Rei branco do comp esta em xeque. */
                            return (1);
                        if(tab[3][0] || tab[2][0] || tab[1][0])	/* Pecas entre rei e torre. */
                            return (1);
                    }

                    else		/*  primeiro e' o humano. Comp joga com as pretas. */
                    {
                        if(peca_ataca(4, 7, 1))	/*  Rei preto do comp esta em xeque. */
                            return (1);
                        if(tab[3][7] || tab[2][7] || tab[1][7])	/* Pecas entre rei e torre. */
                            return (1);
                    }
                    movi = 15;	/* roque grande do computador. */
                }			/*  vez e' do humano */
                else
                {
                    if(ANDOURH)
                        return (1);
                    if(ANDOUTHD)
                        return (1);
                    if(primeiro == 'h')
                    {
                        if(peca_ataca(4, 0, 0))	/*  Rei branco do huma esta em xeque. */
                            return (1);
                        if(tab[3][0] || tab[2][0] || tab[1][0])	/* Pecas entre rei e torre. */
                            return (1);
                    }
                    else		/*  primeiro e' o comp. Huma joga com as pretas. */
                    {
                        if(peca_ataca(4, 7, 0))	/*  Rei preto do huma esta em xeque. */
                            return (1);
                        if(tab[3][7] || tab[2][7] || tab[1][7])	/* Pecas entre rei e torre. */
                            return (1);
                    }
                    movi = 17;	/* roque grande do humano */
                }
            }
            if(movi == 19)		/*  se nao jogou roque de tipo nenhum... */
            {
                if(lance[2] > lance[0] + 1 || lance[2] < lance[0] - 1
                        || lance[3] > lance[1] + 1 || lance[3] < lance[1] - 1)
                    return (1);
                if(humano_joga)
                    movi = 12;		/* rei humano andou; */
                else
                    movi = 11;		/* rei computador andou; */
            }
            break;
        case 5:
            if(lance[0] == lance[2])	/* movimento, sem captura. */
            {
                if(humano_joga)
                {
                    if(primeiro == 'h')	/* jogam as brancas. */
                    {
                        if(lance[1] > lance[3])
                            return (1);
                    }

                    else		/* jogam as pretas. */
                        if(lance[1] < lance[3])
                            return (1);
                }
                else
                {
                    if(primeiro == 'c')	/* jogam as brancas. */
                    {
                        if(lance[1] > lance[3])
                            return (1);
                    }
                    else		/* jogam as pretas. */
                        if(lance[1] < lance[3])
                            return (1);
                }
                if(tab[lance[2]][lance[3]])	/* casa destino ja ocupada. */
                    return (1);
                if(abs(lance[3] - lance[1]) == 2)	/* andou duas casas ---- erro arrumado ---- */
                {
                    if(lance[1] != 1 && lance[1] != 6)
                        return (1);
                    if(tab[lance[0]][(lance[1] + lance[3]) / 2])	/* se casa do meio ocupada. */
                        return (1);
                    movi = 6;		/* peao andou duas casas. */
                }
                else			/* nao andou duas casas. */
                {
                    if(abs(lance[3] - lance[1]) != 1)	/* nao andou uma casa. */
                        return (1);
                }
            }
            else			/* movimento de captura do peao. */
            {
                if(humano_joga)
                {
                    if(primeiro == 'h')
                    {
                        if(!tab[lance[2]][lance[3]])	/* se casa destino == 0 */
                        {
                            if(ENPASSANT == -1)
                                return (1);
                            if(lance[1] != 5)
                                return (1);
                            if(lance[0] != ENPASSANT + 1
                                    && lance[0] != ENPASSANT - 1)
                                return (1);
                            if(lance[2] != ENPASSANT || lance[3] != 6)
                                return (1);
                            movi = 18;	/* comeu 'en passant'.     parte 1 ! */
                        }
                        else		/* comeu peca ou peao normalmente. */
                        {
                            if(lance[0] != lance[2] + 1
                                    && lance[0] != lance[2] - 1)
                                return (1);
                            if(lance[3] != lance[1] + 1)	/* --------- erro arrumado! ----- */
                                return (1);
                        }
                    }
                    else		/* primeiro e' o computador. */
                    {
                        if(!tab[lance[2]][lance[3]])	/* se casa destino == 0 */
                        {
                            if(ENPASSANT == -1)
                                return (1);
                            if(lance[1] != 3)
                                return (1);
                            if(lance[0] != ENPASSANT + 1
                                    && lance[0] != ENPASSANT - 1)
                                return (1);
                            if(lance[2] != ENPASSANT || lance[3] != 2)
                                return (1);
                            movi = 18;	/* comeu 'en passant'. e agora?  Alguem. Humano Pretas. */
                        }

                        else		/* comeu peca ou peao normalmente. */
                        {
                            if(lance[0] != lance[2] + 1
                                    && lance[0] != lance[2] - 1)
                                return (1);
                            if(lance[3] != lance[1] - 1)	/* erro arrumado. */
                                return (1);
                        }
                    }
                }
                else			/* computador joga */
                {
                    if(primeiro == 'c')
                    {
                        if(!tab[lance[2]][lance[3]])	/* se casa destino == 0 */
                        {
                            if(ENPASSANT == -1)
                                return (1);
                            if(lance[1] != 5)
                                return (1);
                            if(lance[0] != ENPASSANT + 1
                                    && lance[0] != ENPASSANT - 1)
                                return (1);
                            if(lance[2] != ENPASSANT || lance[3] != 6)
                                return (1);
                            movi = 18;	/* comeu 'en passant'.             AH NAO NOVAMENTE! */
                        }
                        else		/* comeu peca ou peao normalmente. */
                        {
                            if(lance[0] != lance[2] + 1
                                    && lance[0] != lance[2] - 1)
                                return (1);
                            if(lance[3] != lance[1] + 1)	/* erro arrumado. */
                                return (1);
                        }
                    }
                    else		/* primeiro e' o humano. */
                    {
                        if(!tab[lance[2]][lance[3]])	/* se casa destino == 0 */
                        {
                            if(ENPASSANT == -1)
                                return (1);
                            if(lance[1] != 3)
                                return (1);
                            if(lance[0] != ENPASSANT + 1
                                    && lance[0] != ENPASSANT - 1)
                                return (1);
                            if(lance[2] != ENPASSANT || lance[3] != 2)
                                return (1);
                            movi = 18;	/* comeu 'en passant'.   Para que perdi as contas... */
                        }
                        else		/* comeu peca ou peao normalmente. */
                        {
                            if(lance[0] != lance[2] + 1
                                    && lance[0] != lance[2] - 1)
                                return (1);
                            if(lance[3] != lance[1] - 1)
                                return (1);
                        }
                    }
                }			/*  else do computador joga */
            }			/*  else do movimento de captura do peao */
            if(lance[3] == 0 || (lance[3] == 7 && peca == 5))
                movi = 13;		/* peao promoveu */
            break;
        case 40:
        case 25:
        case 16:
            if(lance[0] != lance[2] && lance[1] != lance[3])	/* andou na diagonal. */
            {
                if(peca == 25)	/*  se for torre, retorne: esta peca nao move assim. */
                    return (1);
                icol = lance[0] > lance[2] ? -1 : 1;
                ilin = lance[1] > lance[3] ? -1 : 1;
                casacol = lance[0] + icol;
                casalin = lance[1] + ilin;
                while(casacol != lance[2] && casalin != lance[3])
                {
                    if(tab[casacol][casalin] != VZ)
                        return (1);
                    casacol += icol;
                    casalin += ilin;
                }
                if(casacol != lance[2] || casalin != lance[3])	/*  ao final do loop, casalin */
                    return (1);		/*  e casacol devem ser iguais aa casa destino. */
            }
            else			/* andou reto. */
            {
                if(peca == 16)	/*  se for bispo, retorne: esta peca nao move assim. */
                    return (1);
                icol = lance[0] > lance[2] ? -1 : (lance[0] < lance[2] ? 1 : 0);
                ilin = lance[1] > lance[3] ? -1 : (lance[1] < lance[3] ? 1 : 0);
                casacol = lance[0] + icol;
                casalin = lance[1] + ilin;
                while(casacol != lance[2] || casalin != lance[3])
                {
                    if(tab[casacol][casalin] != VZ)
                        return (1);
                    casacol += icol;
                    casalin += ilin;
                }
                if(peca == 25)	/*  andou com a torre. */
                {
                    if(lance[0] == 7)	/*  se for torre do rei... */
                        if(humano_joga && ANDOUTHR != 1)
                            if(primeiro == 'h')
                                if(lance[1] == 0)
                                    movi = 7;	/*  Andou pela primeira vez a THR. */
                                else;	/*  humano de brancas andou com THD pela coluna h; */
                            else
                                if(lance[1] == 7)
                                    movi = 7;
                                else;		/*  humano de pretas andou com THD pela coluna h; */
                        else		/*  nao e' humano ou humano ja tinha andado a torre do rei. */
                            if(!humano_joga && ANDOUTCR != 1)
                                if(primeiro == 'c')
                                    if(lance[1] == 0)
                                        movi = 9;	/*  Andou pela primeira vez a TCR. */
                                    else;	/*  comp de brancas andou com TCD pela coluna h; */
                                else
                                    if(lance[1] == 7)
                                        movi = 9;
                                    else;		/*  comp de pretas andou com TCR pela coluna h; */
                            else;		/* nao e' comp ou comp ja tinha andado a torre do rei */
                    if(lance[0] == 0)	/*  se for torre da dama */
                        if(humano_joga && ANDOUTHD != 1)
                            if(primeiro == 'h')
                                if(lance[1] == 0)
                                    movi = 8;	/*  Andou pela primeira vez a THD. */
                                else;	/*  humano de brancas andou com THR pela coluna a; */
                            else
                                if(lance[1] == 7)
                                    movi = 8;
                                else;		/*  humano de pretas andou com THR pela coluna a; */
                        else		/*  nao e' humano ou humano ja tinha andado a torre do rei. */
                            if(!humano_joga && ANDOUTCD != 1)
                                if(primeiro == 'c')
                                    if(lance[1] == 0)
                                        movi = 10;	/*  Andou pela primeira vez a TCD. */
                                    else;	/*  comp de brancas andou com TCR pela coluna a; */
                                else
                                    if(lance[1] == 7)
                                        movi = 10;
                                    else;		/*  comp de pretas andou com TCR pela coluna a; */
                            else;		/* nao e' comp ou comp ja tinha andado a torre do rei */
                }			/*  nao andou com torre. */
            }
            break;
        case 15:
            icol = abs(lance[0] - lance[2]);
            ilin = abs(lance[1] - lance[3]);
            if(icol != 1 && icol != 2)
                return (1);
            if(ilin != 1 && ilin != 2)
                return (1);
            if(icol == ilin)
                return (1);
            break;
    }				/*  chave do fim do switch(peca) */

    /*  procura se o movimento deixara rei em xeque. */
    for(ilin = 0; ilin < 8; ilin++)
        for(icol = 0; icol < 8; icol++)
            tabu[icol][ilin] = tab[icol][ilin];
    tab[lance[2]][lance[3]] = tab[lance[0]][lance[1]];
    tab[lance[0]][lance[1]] = VZ;
    if(movi == 18)		/*  comeu en passant */
        tab[ENPASSANT][lance[1]] = VZ;	/*  apaga peao adversario do tabuleiro. */
    for(ilin = 0; ilin < 8; ilin++)
        for(icol = 0; icol < 8; icol++)
            if(humano_joga)
            {
                if(tab[icol][ilin] == -200)
                    if(peca_ataca(icol, ilin, 0))
                        movi = 2;
                    else
                        break;
            }
            else
                if(tab[icol][ilin] == 200)
                    if(peca_ataca(icol, ilin, 1))
                        movi = 2;
                    else
                        break;
    for(ilin = 0; ilin < 8; ilin++)
        for(icol = 0; icol < 8; icol++)
            tab[icol][ilin] = tabu[icol][ilin];
    return (movi);
}

/* ------------------------------------------------------------------------- */
char situacao(int humano_joga)
{

    /*  pega o tabuleiro e retorna: M,m,a,p,i,5,r,T,t,B,b */
    int i, val;
    int tentativa = 0;
    int lancebak[4];
    humano_joga %= 2;
    for(i = 0; i < 4; i++)
        lancebak[i] = lance[i];

    do
    {
        for(i = 0; i < 4; i++)
            lance[i] = rand() % 8;	/* lance e' variavel global usada por valido(); */
        tentativa++;
        val = valido(humano_joga);
    }
    while(val < 6 && tentativa < 32767);
    for(i = 0; i < 4; i++)
        lance[i] = lancebak[i];
    if(val < 6)
        if(rei_xeque(humano_joga))
            if(primeiro == 'h')	/* Se humano e' brancas */
                if(humano_joga)	/* e esta em xeque */
                    return ('M');
                else			/* se comput esta em xeque... */
                    return ('m');
            else			/* se humano e' pretas */
                if(humano_joga)		/* e esta em xeque */
                    return ('m');
                else			/* senao comput de brancas esta */
                    return ('M');		/* em xeque. */
        else			/* sem lances, mas nao esta em xeque... */
            return ('a');		/* empate por afogamento */
    return (' ');
}

/* ------------------------------------------------------------------------- */
int estatico(int prof)
{
    int col, lin, peca_comp = 0, peca_huma = 0;

    /* analisa tab do ponto de vista de: prof par   --> computador */
    /*                                   prof impar --> humano */
    /*   retorna pecas_proprias - pecas_adversarias. */
    /*  se eu estou bom, retorna numero positivo. */
    for(col = 0; col < 8; col++)
        for(lin = 0; lin < 8; lin++)
            if(tab[col][lin] > 0)	/* peca_comp ou peca_huma e' um valor positivo */
                peca_comp += tab[col][lin];

            else
                peca_huma -= tab[col][lin];

    /* procura peoes dobrados */
    /* procura peoes isolados */
    /* pontuacao para roque pequeno */
    /* pontuacao para roque grande */
    /* peoes do roque estao nas casas iniciais */
    /* cavalo do roque esta na melhor casa */
    /* torre do roque esta na melhor casa */
    /* cavalos desenvolvidos */
    /* bispos desenvolvidos */
    /* controle do centro */
    /* torres nas colunas */
    /* dama nao se movimenta antes do sexto lance */
    /* controle das casas:mobilidade */

    /*  retorna o resultado... */
    if(prof % 2)			/* se impar, ponto de vista humano. */
        return (peca_huma - peca_comp);	/* impar */
    else
        return (peca_comp - peca_huma);	/* par */
}

/* ------------------------------------------------------------------------- */
char compjoga(void)
{
    int i, j, peca, lances[30][6], tabu[8][8];
    int val, troca, lan;
    int aux[6];
    for(j = 0; j < 30; j++)
    {

        do
        {
            for(i = 0; i < 4; i++)
                lance[i] = rand() % 8;	/* tecnica de sorteio de peca! Nao e tao inteligente... */
            val = valido(0);
        }
        while(val < 6);		/* enquanto o lance nao for valido (val>=6) */
        for(i = 0; i < 4; i++)
            lances[j][i] = lance[i];
        lances[j][5] = val;
    }
    for(lan = 0; lan < 30; lan++)
    {
        for(i = 0; i < 8; i++)
            for(j = 0; j < 8; j++)
                tabu[i][j] = tab[i][j];
        peca = tab[lances[lan][0]][lances[lan][1]];
        tab[lances[lan][0]][lances[lan][1]] = VZ;
        tab[lances[lan][2]][lances[lan][3]] = peca;
        switch(lances[lan][5])
        {
            case 13:		/* promocao */
                tab[lances[lan][2]][lances[lan][3]] = 40;
                break;
            case 14:		/* roque */
                tab[7][lances[lan][1]] = 0;
                tab[5][lances[lan][1]] = 25;
                break;
            case 15:		/* roque */
                tab[0][lances[lan][1]] = 0;
                tab[3][lances[lan][1]] = 25;
                break;
            case 18:		/* enpassant */
                tab[ENPASSANT][lances[lan][1]] = VZ;
                break;
        }
        lances[lan][4] = estatico(0);
        for(i = 0; i < 8; i++)
            for(j = 0; j < 8; j++)
                tab[i][j] = tabu[i][j];
    }

    do
    {
        troca = 0;
        for(lan = 0; lan < 29; lan++)
        {
            if(lances[lan][4] < lances[lan + 1][4])
            {
                troca = 1;
                for(j = 0; j < 6; j++)
                {
                    aux[j] = lances[lan][j];
                    lances[lan][j] = lances[lan + 1][j];
                    lances[lan + 1][j] = aux[j];
                }
            }
        }
    }
    while(troca);
    peca = tab[lances[0][0]][lances[0][1]];
    tab[lances[0][0]][lances[0][1]] = VZ;
    tab[lances[0][2]][lances[0][3]] = peca;
    switch(lances[0][5])
    {
        case 6:
            ENPASSANT = lance[0];
            break;
        case 7:
            ANDOUTHR = 1;
            break;
        case 8:
            ANDOUTHD = 1;
            break;
        case 9:
            ANDOUTCR = 1;
            break;
        case 10:
            ANDOUTCD = 1;
            break;
        case 11:
            ANDOURC = 1;
            break;
        case 12:
            ANDOURH = 1;
            break;
        case 13:
            tab[lance[2]][lance[3]] = 40;
            break;			/* mudei de tab[la[0]][la[1]] */
        case 14:
            ANDOURC = ANDOUTCR = 1;
            tab[7][lance[1]] = 0;
            tab[5][lance[1]] = 25;
            break;
        case 15:
            ANDOURC = ANDOUTCD = 1;
            tab[0][lance[1]] = 0;
            tab[3][lance[1]] = 25;
            break;
        case 18:
            tab[ENPASSANT][lance[1]] = VZ;
            break;
        case 19:
            break;
        default:
            printf("Erro na funcao compjoga. val=%d", val);
            getchar();
    }
    if(lances[0][5] != 6)
        ENPASSANT = -1;
    vez = 'h';
    val = situacao(1);		/* vez do huma jogar, por isso o: um. */
    for(i = 0; i < 4; i++)
        lance[i] = lances[0][i];
    return (val);
}

/* ------------------------------------------------------------------------- */
int peca_ataca(int col, int lin, int peca_do_huma)
{

    /* peca_do_huma==par   => peca do comp ataca casa(col,lin) */
    /* peca_do_huma==impar => peca do huma ataca casa(col,lin) */
    int icol, ilin, casacol, casalin;
    peca_do_huma = peca_do_huma % 2;	/* se peca_do_huma impar =>peca_do_huma da peca e' humano. */

    /* torre ou dama atacam a casa... */
    for(icol = col - 1; icol >= 0; icol--)	/* desce coluna */
    {
        if(tab[icol][lin] == VZ)
            continue;
        if(peca_do_huma)		/*  humano ataca casa... */
            if(tab[icol][lin] == -25 || tab[icol][lin] == -40)
                return (1);
            else
                break;
        else			/*  computador ataca casa... */
            if(tab[icol][lin] == 25 || tab[icol][lin] == 40)
                return (1);
            else
                break;
    }
    for(icol = col + 1; icol < 8; icol++)	/* sobe coluna */
    {
        if(tab[icol][lin] == VZ)
            continue;
        if(peca_do_huma)		/*  humano ataca casa... */
            if(tab[icol][lin] == -25 || tab[icol][lin] == -40)
                return (1);
            else
                break;
        else			/*  computador ataca casa... */
            if(tab[icol][lin] == 25 || tab[icol][lin] == 40)
                return (1);
            else
                break;
    }
    for(ilin = lin + 1; ilin < 8; ilin++)	/*  direita na linha */
    {
        if(tab[col][ilin] == VZ)
            continue;
        if(peca_do_huma)		/*  humano ataca casa... */
            if(tab[col][ilin] == -25 || tab[col][ilin] == -40)
                return (1);
            else
                break;
        else			/*  computador ataca casa... */
            if(tab[col][ilin] == 25 || tab[col][ilin] == 40)
                return (1);
            else
                break;
    }
    for(ilin = lin - 1; ilin >= 0; ilin--)	/*  esquerda na linha */
    {
        if(tab[col][ilin] == VZ)
            continue;
        if(peca_do_huma)		/*  humano ataca casa... */
            if(tab[col][ilin] == -25 || tab[col][ilin] == -40)
                return (1);
            else
                break;
        else			/*  computador ataca casa... */
            if(tab[col][ilin] == 25 || tab[col][ilin] == 40)
                return (1);
            else
                break;
    }

    /*  cavalo ataca casa... */
    for(icol = -2; icol < 3; icol++)
        for(ilin = -2; ilin < 3; ilin++)
        {
            if(abs(icol) == abs(ilin) || icol == 0 || ilin == 0)
                continue;
            if(col + icol < 0 || col + icol > 7 || lin + ilin < 0
                    || lin + ilin > 7)
                continue;
            if(peca_do_huma)	/* humano ataca casa... */
                if(tab[col + icol][lin + ilin] == -15)
                    return (1);
                else
                    continue;
            else
                if(tab[col + icol][lin + ilin] == 15)
                    return (1);
        }

    /*  bispo ou dama atacam casa... */
    for(icol = -1; icol < 2; icol += 2)
        for(ilin = -1; ilin < 2; ilin += 2)
        {
            casacol = col;		/* para cada diagonal, comece na casa origem. */
            casalin = lin;

            do
            {
                casacol = casacol + icol;
                casalin = casalin + ilin;
                if(casacol < 0 || casacol > 7 || casalin < 0 || casalin > 7)
                    break;
            }
            while(tab[casacol][casalin] == 0);
            if(casacol >= 0 && casacol <= 7 && casalin >= 0 && casalin <= 7)
                if(peca_do_huma)
                    if(tab[casacol][casalin] == -16 || tab[casacol][casalin] == -40)
                        return (1);
                    else
                        continue;		/*  achou peca, mas esta nao anda em diagonal ou e' peca propria */
                else
                    if(tab[casacol][casalin] == 16 || tab[casacol][casalin] == 40)
                        return (1);
        }				/*  proxima diagonal */

    /*  ataque de rei... */
    for(icol = col - 1; icol <= col + 1; icol++)
        for(ilin = lin - 1; ilin <= lin + 1; ilin++)
        {
            if(icol == col && ilin == lin)
                continue;
            if(icol < 0 || icol > 7 || ilin < 0 || ilin > 7)
                continue;
            if(peca_do_huma)
                if(tab[icol][ilin] == -200)
                    return (1);
                else
                    continue;
            else
                if(tab[icol][ilin] == 200)
                    return (1);
        }

    /*  ataque de peao */
    /*  branco */
    if(lin > 1)
    {
        ilin = lin - 1;
        if(peca_do_huma && primeiro == 'h')
        {
            if(col - 1 >= 0)
                if(tab[col - 1][ilin] == -5)
                    return (1);
            if(col + 1 <= 7)
                if(tab[col + 1][ilin] == -5)
                    return (1);
        }
        if(!peca_do_huma && primeiro == 'c')
        {
            if(col - 1 >= 0)
                if(tab[col - 1][ilin] == 5)
                    return (1);
            if(col + 1 <= 7)
                if(tab[col + 1][ilin] == 5)
                    return (1);
        }
    }

    /* peao preto */
    if(lin < 6)
    {
        ilin = lin + 1;
        if(peca_do_huma && primeiro == 'c')
        {
            if(col - 1 >= 0)
                if(tab[col - 1][ilin] == -5)
                    return (1);
            if(col + 1 <= 7)
                if(tab[col + 1][ilin] == -5)
                    return (1);
        }
        if(!peca_do_huma && primeiro == 'h')
        {
            if(col - 1 >= 0)
                if(tab[col - 1][ilin] == 5)
                    return (1);
            if(col + 1 <= 7)
                if(tab[col + 1][ilin] == 5)
                    return (1);
        }
    }
    return (0);
}

/* ------------------------------------------------------------------------- */
void lance2movi(char *s)
{
    int i;
    for(i = 0; i < 2; i++)
    {
        s[2 * i] = lance[2 * i] + 'a';
        s[2 * i + 1] = lance[2 * i + 1] + '1';
    }
}

/* ------------------------------------------------------------------------- */
int movi2lance(char *s)
{
    int i;
    for(i = 0; i < 2; i++)
        if(s[2 * i] < 'a' || s[2 * i] > 'h' || s[2 * i + 1] < '1'
                || s[2 * i + 1] > '8')
            return (0);
    for(i = 0; i < 2; i++)
    {
        lance[2 * i] = (int)(s[2 * i] - 'a');
        lance[2 * i + 1] = (int)(s[2 * i + 1]) - '1';
    }
    return (1);
}

/* ------------------------------------------------------------------------- */
int rei_xeque(int humano_joga)
{
    int ilin, icol, movi;
    humano_joga = humano_joga % 2;
    for(ilin = 0; ilin < 8; ilin++)
        for(icol = 0; icol < 8; icol++)
            if(tab[icol][ilin] == -200 * (humano_joga * 2 - 1))
                if(peca_ataca(icol, ilin, !humano_joga))
                    return (1);
                else
                    break;
    return (0);
}


/* ------------------------------------------------------------------------- */
/* vi: set ai cin et ts=4 sw=4 tw=0 wm=0 fo=croqltn : C config Vim modeline  */
/* Template by Dr. Beco <rcb at beco dot cc>  Version 20160714.153029        */

