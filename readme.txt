xadreco version 5.0. Chess engine compatible with XBoard/WinBoard Protocol (C)
Copyright (C) 2004, Ruben Carlo Benante.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

If you redistribute it and/or modify it, you need to
cite the original source-code and author name:
Ruben Carlo Benante.
email: benante@ig.com.br
web page: http://www.geocities.com/pag_sax/xadreco/
address: 
Federal University of Pernambuco
Center of Computer Science
PostGrad Room 7-1
PO Box 7851
Zip Code 50670-970
Recife, PE, Brazil
FAX: +55 (81) 2126-8438
PHONE: +55 (81) 2126-8430  x.4067

-----------------------------------------------------------------------------

xadreco version 5.0. Motor de xadrez compatível com o XBoard/WinBoard (C)
Copyright (C) 2004, Ruben Carlo Benante.
   
Este programa é software livre; você pode redistribuí-lo e/ou
modificá-lo sob os termos da Licença Pública Geral GNU, conforme
publicada pela Free Software Foundation; tanto a versão 2 da
Licença como (a seu critério) qualquer versão mais nova.

Este programa é distribuído na expectativa de ser útil, mas SEM
QUALQUER GARANTIA; sem mesmo a garantia implícita de
COMERCIALIZAÇÃO ou de ADEQUAÇÃO A QUALQUER PROPÓSITO EM
PARTICULAR. Consulte a Licença Pública Geral GNU para obter mais
detalhes.
 
Você deve ter recebido uma cópia da Licença Pública Geral GNU
junto com este programa; se não, escreva para a Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307, USA.

Se você redistribuir ou modificar este programa, você deve citar
o código-fonte original e o nome do autor:
Ruben Carlo Benante.
email: benante@ig.com.br
página web: http://www.geocities.com/pag_sax/xadreco/
endereço: 
Universidade Federal de Pernambuco
Centro de Informática
Sala de Pós-graduação 7-1
Caixa Postal 7851
CEP 50670-970
Recife, PE, Brasil
FAX: +55 (81) 2126-8438
FONE: +55 (81) 2126-8430  ramal.4067

-----------------------------------------------------------------------------


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%% Direitos Autorais segundo normas do código-livre: (GNU - GPL)
//%% Universidade Estadual Paulista - UNESP, Universidade Federal de Pernambuco - UFPE
//%% Jogo de xadrez: xadreco
//%% Arquivo readme.txt
//%% Técnica: MiniMax
//%% Autor do original: Ruben Carlo Benante
//%% Criacao: 19/08/1994
//%% Versão para Winboard: 26/09/04
//%% Versão 5.0: 19/10/04
//%% e-mail do autor: benante@ig.com.br
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

-----------------------------------------------------------------------------


Para instalar, para linux ou windows, você deve fazer assim:


1- Instale o xboard (linux) ou winboard (windows).
Pode intalanar na pasta que ele quiser, por exemplo, 
linux: /bin/xadreco
ou
windows: c:\Arquivos de Programas\WinBoard

Página oficial: <http://www.tim-mann.org/chess.html>


2- Copie o arquivo xadreco (linux) ou xadreco.exe (windows) para a mesma pasta
da instalação do XBoard ou Winboard.


3- Copie o arquivo de configuração xboard.ini ou winboard.ini para a mesma pasta.
O Sistema-Operacional vai perguntar se quer sobreescrever. Diga que sim!

(Atenção, se você usa outras engines além do
GNUChess, você irá desconfigurar elas. Neste caso, é melhor
ler a parte abaixo, em vez de substituir o xboard.ini ou winboard.ini, 
simplesmente adicionar a linha correta nele)


4- Agora inicie o xboard ou winboard! 
(exemplos:
linux: digite xboard. 
Windows: No menu do botão "iniciar/programas/
winboard/WinBoard Startup Dialog").

Selecione no menu
"Play against a chess engine or match two engines"
(conforme figura menu-winboard.JPG)

Deve aparecer selecionado o xadreco na primeira caixa de diálogo. 
(Se não aparecer, selecione nele antes de clicar em OK nesta janelinha.)

Pronto, clique em ok. 
Ele deve ler minha engine e ainda vai dizer que ela não faz análise! (Por enquanto!)

Mande-me um email com críticas, sugestões e elogios.
rcb74@ig.com.br

Abraços,
Ruben.

PS. Se der errado:
(Ou se você usa outras engines)


Com o bloco-de-notas, abra o arquivo
xboard.ini ou winboard.ini que também está na mesma pasta.


Lá no final desse arquivo tem umas
linhas assim:
-----------------------------------------------------------
/firstChessProgramNames={"GNUChess"
"GNUChes5 xboard"
}
/secondChessProgramNames={"GNUChes5 xboard"
"GNUChess"
}
-----------------------------------------------------------
(*) acrescente a linhas, em
firstChessProgramNames e em secondChessProgramNames, 
respectivamente:

para linux:
xboard -cp -fcp "xadreco xboard" -fd "."
xboard -scp "xadreco xboard" -sd "."

para windows:
winboard.exe -cp -fcp "xadreco.exe xboard" -fd "."
winboard.exe -scp "xadreco.exe xboard" -sd "."


E portanto, no final das contas, vai ficar assim:
Para linux:
-----------------------------------------------------------
/firstChessProgramNames={xboard -cp -fcp "xadreco xboard" -fd "."
"GNUChess"
"GNUChes5 xboard"
}
/secondChessProgramNames={xboard -scp "xadreco xboard" -sd "."
"GNUChes5 xboard"
"GNUChess"
}
-----------------------------------------------------------

Para Windows:
-----------------------------------------------------------
/firstChessProgramNames={winboard.exe -cp -fcp "xadreco.exe xboard" -fd "."
"GNUChess"
"GNUChes5 xboard"
}
/secondChessProgramNames={winboard.exe -scp "xadreco.exe xboard" -sd "."
"GNUChes5 xboard"
"GNUChess"
}
-----------------------------------------------------------


Em último caso, ou se você quiser criar um atalho, utilize no
prompt (aqui é C:>) do Sistema Operacional (Linux ou DOS) os seguintes comandos:
(O exemplo está para o WinBoard apenas, mas é análogo para XBoard)

vá para a pasta do Winboard e digite o comando:

C:>winboard.exe -cp -fcp "xadreco.exe xboard" -fd "."


Assim vai iniciar só o xadreco.
(O xadreco.exe deve estar na mesma pasta do winboard.exe)


Para iniciar por exemplo o Xadreco e o Crafty, tente:
(nesse caso, o meu crafty está na pasta indicada abaixo...)


C:>winboard.exe -cp -fcp "xadreco.exe xboard" -fd "." 
-cp -scp "crafty-19.2.exe winboard" -sd "C:\Arquivos de Programas\xadrez\crafty"
