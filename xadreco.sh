#!/bin/bash
#***************************************************************************
#*   xadreco.sh                    Build 20160606.173220, see VERSION file *
#*                                                                         *
#*   Xadreco chess engine                                                  *
#*   Copyright (C) 1994-2016 by Ruben Carlo Benante <rcb@beco.cc>          *
#***************************************************************************
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License as published by  *
#*   the Free Software Foundation version 2 of the License.                *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU General Public License for more details.                          *
#*                                                                         *
#*   You should have received a copy of the GNU General Public License     *
#*   along with this program; if not, write to the                         *
#*   Free Software Foundation, Inc.,                                       *
#*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
#***************************************************************************
#*   To contact the author, please write to:                               *
#*   Ruben Carlo Benante                                                   *
#*   Email: rcb@beco.cc                                                    *
#*   Webpage: http://xadreco.beco.cc/                                      *
#***************************************************************************

xboard -firstIsUCI false -secondIsUCI false -tc 5 -inc 5 -mps 40 -size petite -coords -highlight -fd "." -fcp "./xadreco" -sd "." -scp "./xadreco" -debug

#end of xadreco.sh file

