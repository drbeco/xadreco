xadreco version 5.5. Chess engine compatible with XBoard/WinBoard Protocol (C)
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

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%% Copyright reserved: (GNU - GPL)
//%% Universidade Estadual Paulista - UNESP, Universidade Federal de Pernambuco - UFPE
//%% Chess game: xadreco
//%% File readme-en.txt
//%% Technique: MiniMax
//%% Author: Ruben Carlo Benante
//%% Created at: 19/08/1994
//%% Winboard version: 26/Sep/04
//%% English site version: 19/Oct/04
//%% e-mail do autor: benante@ig.com.br
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

-----------------------------------------------------------------------------
Index:

1- Upgrading from older versions
2- How to install
-----------------------------------------------------------------------------
1- Upgrading from older versions


The only thing you should do after downloading the new chess engine is
to save (re-write) the old one and put the new one in the same folder that
is XBoard/Winboard.



-----------------------------------------------------------------------------
2- How to install


To install, under Linux or Windows, you should do this:


1- Install the XBoard (linux) or WinBoard (Windows).
You can put it on the folder it wants, for example:
Linux: /bin/xboard
or
windows: c:\Program Files\WinBoard

Oficial site: <http://www.tim-mann.org/chess.html>


2- Copy the bin file xadreco (linux) or xadreco.exe (windows) to
the same folder of the XBoard ou Winboard.

3- Copy the configuration file xboard.ini or winboard.ini to the same
folder. The Operational System will ask if it could re-write the file.
Say yes.

(Be careful, if you use other engines already, besides GNUChess,
you will misconfigure it. In this case, it is better to read the last
part of this file, do not re-write the xboard.ini or winboard.ini file, 
but open it in some text-editor and add the correct line)

4- Now start the "Startup Dialog" of the XBoard/WinBoard,

select the option
"Play against a chess engine or match two engines"
(you can see a pic of this menu in the Xadreco's site
called menu-winboard.JPG)

It should open the first dialog box with Xadreco selected. If not,
select it and click ok.


Now you clicked ok, and it is ready to play!

Mande-me um email com críticas, sugestões e elogios.
benante@ig.com.br

Take care you,
Ruben.

PS. If it did not worked:
(Or if you use other engines)


With notepade, open the file
xboard.ini or winboard.ini in the same folder of the
winboard.


There in the end you will find this lines:
-----------------------------------------------------------
/firstChessProgramNames={"GNUChess"
"GNUChes5 xboard"
}
/secondChessProgramNames={"GNUChes5 xboard"
"GNUChess"
}
-----------------------------------------------------------

(*) Add the new lines in the 
firstChessProgramNames and secondChessProgramNames, 
respectivally:

for linux:
xboard -cp -fcp "xadreco xboard" -fd "."
xboard -scp "xadreco xboard" -sd "."

for windows:
winboard.exe -cp -fcp "xadreco.exe xboard" -fd "."
winboard.exe -scp "xadreco.exe xboard" -sd "."


And now, your file should look like this:
For linux:
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

For Windows:
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


In the last case, or if you want to create a new shortcut to it,
go to the command prompt (#) of linux or DOS, and type the
following command line:


Go to XBoard/WinBoard folder and type:

#winboard.exe -cp -fcp "xadreco.exe xboard" -fd "."


This command will initialize only xadreco. If you want
to initialize one more chess engine, try this form:


#winboard.exe -cp -fcp "xadreco.exe xboard" -fd "." 
-cp -scp "crafty-19.2.exe winboard" -sd "C:\Arquivos de Programas\xadrez\crafty"
