Xadreco v.5.7
Copyright (C) 2007 by Ruben Carlo Benante                             *
dr.beco@gmail.com                                                     *
date: 17/07/2007
http://www.geocities.com/pag_sax/xadreco/
http://codigolivre.org.br/projects/xadreco/

----
Table of contents:

1- Copyright notice (Nota de direitos)
2- Description of each file inside the package (descrição dos arquivos)
3- Credits (Créditos)
4- Install process for xboard / linux (Processo de instação xboard/linux)
5- Install process for winboard / windows (Processo de instalação winboard/windows)
6- Install process for UCI GUI; like fritz / probably windows (Processo de instalação para GUI UCI; como o fritz, provavelmente windows).
7- Change log of this version (Mudanças desta versão).
8- Know bugs (Bugs conhecidos)
9- To do (afazeres)

----
1- Copyright notice
This file is part of GPL program Xadreco v.5.7

    GNU program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.

    GNU program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with program; see the file COPYING. If not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

----
2- Description of each file inside the package
Files included in GPL (Arquivos incluídos na GPL):
Files inside xadrecov57.tar.gz (arquivos dentro do xadrecov57.tar.gz)

COPYING and GPL-pt_BR.txt - GNU/GPL License (licença GNU/GPL em inglês e português)
engines.ini - Arena config file (configuração do Arena)
livro.txt - Opening Book (livro de aberturas)
readme.txt - this file: installation help (este arquivo: ajuda da instalação)
winboard.ini - Winboard config file example (configuração do winboard)
xadreco-logo1.jpg - Xadreco's Logo (Logotipo do Xadreco)
xadrecov57.cpp - source code (código fonte) (for linux/windows)
xadreco - linux binary application (executável linux)
xadreco.exe - windows application (executável windows)
xadreco-icon.png - Xadreco's icon (ícone do xadreco)

Another third part files:
wb2uci.eng - UCI configuration file for Wb2Uci.exe translation app. (Arquivo de configuração UCI para a aplicação tradutora WB2Uci.exe)
Wb2Uci.exe - Translation application for Winboard to UCI, by Odd Gunnar Malin (see bellow). Aplicação tradutora do Winboard para UCi, por Odd Gunnar Malin (Veja abaixo).
----
3- Credits

People who helped in someway, and I should thank you:

Filipe Maia <fmaia@gmx.net>
Bug reports and the "Move now!" command

Dann Corbit <DCorbit@connx.com>
A lot of tips, bugs, and optimizations

Jim Ablett <deckard8@phreaker.net>
Compile windows version

Alexandre Oliveira <afmo74@uol.com.br>
Beautifull logo

David Dahlem <http://bilder11.parsimony.net/forum16635/ddahlem.html>
Another beautifull logo

Joshua Shriver <jshriver@csee.wvu.edu>
Some tips about windows and linux

Samuel Goto <samuelgoto@gmail.com>
Ideas from your own engine Sinapse

Sergio Batista <sergio@personalchesstrainer.com>
Beta-tester number #000000001

Leo Dijksman <leo.dijksman@hccnet.nl>
Added Xadreco to his championship home page

Olivier Deville <odeville@tiscali.fr>
Added Xadreco to his championship home page

Renato Soares <rsosilva@yahoo.com>
Played some internal championships to test Xadreco

Michael Casadevall <sonicmctails@ssonicnet.com>
Helped with license questions about GNU/GPL

Odd Gunnar Malin.
http://home.online.no/~malin/sjakk/Wb2Uci/
Protocol translation from Winboard to UCI

----
4- Install process for xboard

4.1 - Make a bash script:
4.1.1- Unpack the tarball (xadrecov57.tar.gz) to the folder of your preference (lik /home/user/programs/xadrecov57)

4.1.2 Create a bash script with the 2 command lines and save it with the name xxadreco.sh:
---
#!/bin/bash
xboard -tc 5 -inc 0 -mps 40 -size Petite -coords -highlight -fcp "/home/user/programs/xadrecov57/xadreco" -fd "/home/user/programs/xadrecov57/" -scp "/home/user/programs/xadrecov57/xadreco" -sd "/home/user/programs/xadrecov57/"
---
4.1.3 Now just start xxadreco. Make sure you give execution rights to xxadreco file.
./xxadreco

4.2 - Create a application link in you KDE desktop area:
4.2.1 - Right click at the desktop area.
4.2.2 - Click at "creat new" > Link to application
4.2.3 - In the General tab, name the link: xadreco v.5.7, change the icon to xadreco-icon.png
4.2.4 - In the Application tab, fulfill:
Description: Xadreco Chess Engine
Comments: Version 5.7
Command: 
xboard -tc 5 -inc 0 -mps 40 -size Petite -coords -highlight -fcp "/home/user/programs/xadrecov57/xadreco" -fd "/home/user/programs/xadrecov57/" -scp "/home/user/programs/xadrecov57/xadreco" -sd "/home/user/programs/xadrecov57/"
Work directory: /home/user/programs/xadrecov57/
4.2.5 - Click Ok. Now you just double click the icon to play.

----
5- Install process for winboard

5.1- Download and install winboard on your system. You will probably install it at folder:
C:\Program files\WinBoard
(C:\Arquivos de programas\WinBoard)

5.2- Create the 2 new folder (crie as 2 novas pastas) engines and (e) xadrecov57:

C:\Program files\WinBoard\engines\xadrecov57
(C:\Arquivos de programas\WinBoard\engines\xadrecov57

5.3- unzip the file into this folder (descompacte o arquivo nesta pasta):
xadreco.tar.gz

5.4- Follow 5.4.1 for a easy new installation. Follow 5.4.2 if you have others engines working with winboard already.
(Siga 5.4.1 para uma instalação nova e simples. Siga 5.4.2 se vc já tem outras engines instaladas com o winboard).

5.4.1- If you have a fresh winboard install, you can just copy the file
C:\Program files\WinBoard\engines\xadrecov57\winboard.ini
to the winboard folder:
C:\Program files\WinBoard\

(Se sua instalação do winboard é nova, basta copiar o arquivo winboard.ini que acompanha
o xadreco para a pasta do winboard)

5.4.2- If you have already set up another engines to your winboard.ini, then just open your winboard.ini and add the lines:

winboard.exe -cp -fcp "engines\xadrecov57\xadreco.exe" -fd "engines\xadrecov57\"
to your /firstChessProgramNames= group;
and 

winboard.exe -cp -scp "engines\xadrecov57\xadreco.exe" -sd "engines\xadrecov57\"
/secondChessProgramNames= group;

Notice that this version do not need the parameter "xboard" to work.


(Se você já tem algumas outras engines configuradas no seu winboard.ini, então adicione as linhas:

winboard.exe -cp -fcp "engines\xadrecov57\xadreco.exe" -fd "engines\xadrecov57\"
no grupo /firstChessProgramNames= 

e

winboard.exe -cp -scp "engines\xadrecov57\xadreco.exe" -sd "engines\xadrecov57\"
no grupo /secondChessProgramNames= 

Repare que a partir desta versão não é mais necessário o parâmetro "xboard" para o seu funcionamento.

----
6- Install process for UCI GUI (fritz)
(Instalar no Fritz 7 ou superior)

* Create the folder (crie a pasta):
C:\Program files\ChessBase\Engines\xadreco
(C:\Arquivos de programas\ChessBase\Engines\xadreco)

* unzip the file into this folder (descompacte o arquivo nesta pasta):
xadreco.tar.gz

* Start Fritz (Inicie o Fritz)

* Click at menu (Clique no menu):
- ENGINE
- CREATE UCI ENGINE

* Click at button (Clique no botão):
- BROWSE

* Go to folder (Vá para a pasta):
C:\Program files\ChessBase\Engines\xadreco
(C:\Arquivos de programas\ChessBase\Engines\xadreco)

* Select (Selecione):
Wb2Uci.exe

* Click open (clique em ABRIR):

* Click Ok (Clique em OK):

* That is it. You are ready to use the engine. 
(Pronto, a engine está disponível).

--
To play your first game do this
(Para jogar uma primeira partida, faça assim):

* Click at menu (Clique no menu):
- ENGINE
- CHANGE MAIN ENGINE... F3

* Select Xadreco (Selecione o Xadreco):

* Click Ok (Clique em Ok):

* Do you move as white. (Faça seu lance de brancas).

--
More info about WINBOARD to UCI at Odd Gunnar Malin page.
(Maiores informações sobre WINBOARD para UCI), veja
a página de Odd Gunnar Malin.

----
7- Change log of this version (Mudanças desta versão).

* Função estática simplificada
* Eliminada toda estrutura para jogar usando apenas o xadreco; Agora só com o xboard
* Não há mais necessidade do parâmetro "xboard" na inicialização.

----
8- Know bugs (Bugs conhecidos)
* Parece haver algum erro de sinal que o faz jogar alguns lances suicidas
* Memory leak - Memória não está sendo devolvida corretamente

----
9- To do (afazeres)
* Simplificar o processo de representação do tabuleiro
* Simplificar a função minimax para usar sinal (+) para brancas e (-) para pretas sempre.

------------------------

Have Fun!
Bom divertimento!
Beco.
