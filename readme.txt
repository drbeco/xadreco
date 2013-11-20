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

xadreco version 5.0. Motor de xadrez compat�vel com o XBoard/WinBoard (C)
Copyright (C) 2004, Ruben Carlo Benante.
   
Este programa � software livre; voc� pode redistribu�-lo e/ou
modific�-lo sob os termos da Licen�a P�blica Geral GNU, conforme
publicada pela Free Software Foundation; tanto a vers�o 2 da
Licen�a como (a seu crit�rio) qualquer vers�o mais nova.

Este programa � distribu�do na expectativa de ser �til, mas SEM
QUALQUER GARANTIA; sem mesmo a garantia impl�cita de
COMERCIALIZA��O ou de ADEQUA��O A QUALQUER PROP�SITO EM
PARTICULAR. Consulte a Licen�a P�blica Geral GNU para obter mais
detalhes.
 
Voc� deve ter recebido uma c�pia da Licen�a P�blica Geral GNU
junto com este programa; se n�o, escreva para a Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307, USA.

Se voc� redistribuir ou modificar este programa, voc� deve citar
o c�digo-fonte original e o nome do autor:
Ruben Carlo Benante.
email: benante@ig.com.br
p�gina web: http://www.geocities.com/pag_sax/xadreco/
endere�o: 
Universidade Federal de Pernambuco
Centro de Inform�tica
Sala de P�s-gradua��o 7-1
Caixa Postal 7851
CEP 50670-970
Recife, PE, Brasil
FAX: +55 (81) 2126-8438
FONE: +55 (81) 2126-8430  ramal.4067

-----------------------------------------------------------------------------


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%% Direitos Autorais segundo normas do c�digo-livre: (GNU - GPL)
//%% Universidade Estadual Paulista - UNESP, Universidade Federal de Pernambuco - UFPE
//%% Jogo de xadrez: xadreco
//%% Arquivo readme.txt
//%% T�cnica: MiniMax
//%% Autor do original: Ruben Carlo Benante
//%% Criacao: 19/08/1994
//%% Vers�o para Winboard: 26/09/04
//%% Vers�o 5.0: 19/10/04
//%% e-mail do autor: benante@ig.com.br
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

-----------------------------------------------------------------------------


Para instalar, para linux ou windows, voc� deve fazer assim:


1- Instale o xboard (linux) ou winboard (windows).
Pode intalanar na pasta que ele quiser, por exemplo, 
linux: /bin/xadreco
ou
windows: c:\Arquivos de Programas\WinBoard

P�gina oficial: <http://www.tim-mann.org/chess.html>


2- Copie o arquivo xadreco (linux) ou xadreco.exe (windows) para a mesma pasta
da instala��o do XBoard ou Winboard.


3- Copie o arquivo de configura��o xboard.ini ou winboard.ini para a mesma pasta.
O Sistema-Operacional vai perguntar se quer sobreescrever. Diga que sim!

(Aten��o, se voc� usa outras engines al�m do
GNUChess, voc� ir� desconfigurar elas. Neste caso, � melhor
ler a parte abaixo, em vez de substituir o xboard.ini ou winboard.ini, 
simplesmente adicionar a linha correta nele)


4- Agora inicie o xboard ou winboard! 
(exemplos:
linux: digite xboard. 
Windows: No menu do bot�o "iniciar/programas/
winboard/WinBoard Startup Dialog").

Selecione no menu
"Play against a chess engine or match two engines"
(conforme figura menu-winboard.JPG)

Deve aparecer selecionado o xadreco na primeira caixa de di�logo. 
(Se n�o aparecer, selecione nele antes de clicar em OK nesta janelinha.)

Pronto, clique em ok. 
Ele deve ler minha engine e ainda vai dizer que ela n�o faz an�lise! (Por enquanto!)

Mande-me um email com cr�ticas, sugest�es e elogios.
rcb74@ig.com.br

Abra�os,
Ruben.

PS. Se der errado:
(Ou se voc� usa outras engines)


Com o bloco-de-notas, abra o arquivo
xboard.ini ou winboard.ini que tamb�m est� na mesma pasta.


L� no final desse arquivo tem umas
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


Em �ltimo caso, ou se voc� quiser criar um atalho, utilize no
prompt (aqui � C:>) do Sistema Operacional (Linux ou DOS) os seguintes comandos:
(O exemplo est� para o WinBoard apenas, mas � an�logo para XBoard)

v� para a pasta do Winboard e digite o comando:

C:>winboard.exe -cp -fcp "xadreco.exe xboard" -fd "."


Assim vai iniciar s� o xadreco.
(O xadreco.exe deve estar na mesma pasta do winboard.exe)


Para iniciar por exemplo o Xadreco e o Crafty, tente:
(nesse caso, o meu crafty est� na pasta indicada abaixo...)


C:>winboard.exe -cp -fcp "xadreco.exe xboard" -fd "." 
-cp -scp "crafty-19.2.exe winboard" -sd "C:\Arquivos de Programas\xadrez\crafty"
