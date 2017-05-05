# Como contribuir

Sua contribuição ao código é essencial para tornar o este programa cada vez melhor. Além disso, é difícil para apenas um desenvolvedor testar a imensa quantidade de plataformas e a miríade de configurações existentes. Este guia está aqui para tornar mais simples sua contribuição e aumentar as chances dela ser aceita. Há apenas algumas regras que precisamos para que consigamos manter a organização e o controle do código, então basta seguir este pequeno guia de 'Como contribuir'.

# Iniciando

Tenha certeza de que sua conta está devidamente configurada. Cheque os seguintes itens:

* Cadastre-se no GitHub :)
* Nas configurações do git em sua máquina local confira se seu nome e email são os mesmos que cadastrou no GitHub.
* Siga o código de [conduta do bom programador BecoSystems](BECOSYSTEMS_CODE_OF_CONDUCT.md) 
* Faça um _fork_, implemente e teste o programa.
* Para um _pull request_ ser aceito é **essencial** que as alterações sejam em doses bem pequenas (homeopáticas), mudando apenas um bloco lógico consistente em cada requisição.
* A requisição (_pull request_) deve ser feita preferencialmente de _master_ para _master_, para facilitar a comparação de códigos e padronizar os _forks_
* Faça testes à vontade no ramo _develop_ ou outro qualquer. Mas tente manter o ramo _master_ fiel ao _upstream_.

# Arquivos no repositório

* COPYING
* GPL-pt_BR.txt
* engines.ini
* livro.txt
* wb2uci.eng
* winboard.ini
* xadreco-icon.png
* xadreco-logo1.jpg
* xadreco.c: código fonte em C
* xadreco.h: biblioteca
* zippy-frases.txt
* AUTHORS: lista os autores do programa (em grupo ou individual)
* LICENSE: a licença do programa (prefira GNU GPL v2, MIT ou BSD-3)
* VERSION: contém o número da versão atual (modificado automaticamente pelo _make_)
* makefile: o _script_ rodado pelo comando _make_
* readme.txt
* README.md: em sintaxe de Markdown, contém a "capa" que aparece no GitHub. Deve ter uma explicação sobre:
    - O que é o programa
    - Como compilar
    - Quais opções para iniciar a execução
    - Como funciona, quais opções durante execução
    - Lista de autores
    - Licença
    - Referências: Links para páginas relevantes
* Índice _ctags_ opcional
* _.gitignore_ : faz o git ignorar certos arquivos
* _.gitattributes_ : permite configurar atributos por arquivo (veja abaixo)

# Formatação

## O estilo de indentação

### Linguagem C

* A indentação é feita de acordo com o estilo **A1** descrito pelo _astyle_, também conhecido por _--style=allman_, _--style=bsd_ ou _--style=break_. 

O estilo _Allman_ é um entre mais de 15 estilos e variantes disponíveis e encontradas em vários tipos de código. Não misture os estilos. Este estilo utiliza a seguinte sintaxe, chamada de _broken brackets_ (ou chaves quebradas). Atente para os espaços entre operadores e mudanças de linhas no exemplo abaixo:

```
/**
 * @ingroup GrupoUnico
 * @brief Breve descricao blablabla
 * ...
 */
int foonc(int is_bar[MAX], int x)
{
    int p; /* a really though condition */
    
    p = !(is_bar[0] || isBar[1]);

    /* check if it's beer number 2 and not in some funny condition */
    if(is_bar[x] == 2 && !p)
    {
        bar(p + 1);
        return 1;
    }
    else
        return 0;
}

```

Conheça todos os estilos na [página de documentação do astyle](http://astyle.sourceforge.net/astyle.html).

* Para aplicar o estilo no seu código, pode-se rodar o programa _$astyle -A1 ex11.c_

## Dicas para um bom código fonte:

### Comentários

* Use comentários /* estilo C */
* Coloque um espaço após o /* e um antes do */
* Faça comentários _em linha_ nas declarações de variável. `int i; /* indice geral */`
* Evite comentários em linha em outros locais
* Na medida do possível e do seu conhecimento, escreva tudo em **inglês**, desde nomes de variáveis até comentários. Se for importante usar português, então escreva as variáveis em inglês e os comentários nas **duas** línguas. Caso não seja possível, tudo bem usar apenas português.
* Coloque os comentários na linha _anterior_ ao bloco ou comando a comentar
* Não use acentos (nem c-cedilhas) no código ou comentários
* Use as palavras nos comentários:
    - /* TODO: alguma tarefa */ : para indicar algo que falta fazer
    - /* BUG: esta errado assim e assado */ : para indicar um bug conhecido que precisa ser corrigido no futuro.

### Comentários DOXYGEN

Antes de cada função, coloque comentários na sintaxe _DOXYGEN_. O mínimo necessário para bem explicar uma função é detalhar:

* Grupo do módulo: `@ingroup`
* Resumo: `@brief`
* Detalhes: `@details`
* Opcional, pré-condições: `@pre`
* Parâmetros de entrada: `@param[in]`
* Parâmetros de saída: `@param[out]`
* Parâmetros de entrada e saída: `@param[in, out]` 
* Valor de retorno: `@return` ou `@retval` (ou ambos)
* Opcional, um aviso: `@warning`
* Opcional, se a função está depreciada: `@deprecated`
* Opcional, indicar funções semelhantes: `@see`
* Opcional, indicar _bugs_: `@bug`
* Opcional, indicar _to do_: `@todo`
* Opcional, escrever uma nota: `@note`
* Autor: `@author`
* Opcional, colocar versão: `@version` 
* Data que foi escrita/modificada: `@date` (formato YYYY-MM-DD, ano, mes, dia)
* Opcional, caso diferente do resto do código, colocar a licença de _copyright_: `@copyright`

O exemplo abaixo mostra somente os campos _obrigatórios_, e descreve hipotéticas variáveis _i_, _o_ e _z_:

```

/**
 * @ingroup Modulo
 * @brief Breve descricao
 * @details Descricao detalhada
 * que pode estar em multiplas linhas
 *
 * @param[in] i indice de entrada
 * @param[out] o indice recalculado de saida 
 * @param[in,out] z Entra com um valor blablabla e devolve blebleble
 *
 * @return Descricao do que a funcao retorna
 * @author Ruben Carlo Benante
 * @date 2016-06-05
 */
```

### No meio do código

* Apague espaços sobrando ao final da linha
* Não inclua espaços após `(`, `[`, `{` ou antes de `}`, `]` e `)`
* Use espaços após vírgulas e ponto-e-vírgulas
* Use espaços em volta de operadores (exceto os unários como `!`)
* Não cometa erros de grafia (**não** use acentos!)
* Não alinhe tokens verticalmente em linhas consecutivas
* Use indentação de 4 espaços (não use _TABS_)
* Se quebrar uma linha, indente-a 4 espaços
* Use uma linha em branco entre métodos, funções ou blocos lógicos importantes
* Use final de linha de um byte (`\n`) **estilo Unix** (\*)
* Deixe uma linha em branco ao final do arquivo [conforme padrão POSIX](http://unix.stackexchange.com/questions/23903/should-i-end-my-text-script-files-with-a-newline)

### Variáveis

* Variáveis com letras maiúsculas somente se:
    - São PROLOG, claro ;)
    - São MACROS (logo não são _variáveis_) em C (exemplo: `#define CINCO 1`)
    - São _MUIIIITO_ importantes e precisam de algum destaque. Não tente usar assim, pois você provavelmente não vai convencer ninguém. ;)
* Use nomes claros para as variáveis
* O nome da variável deve indicar sua _intenção_, i.é., o dado que ela carrega.
* Prefira nomes que demonstrem o conceito geral do domínio, ao invés dos padrões que eles implementam.
* Não faça nomes gigantes. Um tamanho _máximo_ que já não fica tão bom é até 15 caracteres. Tente nomes menores, até 10 no pior caso.
* Use sublinhados para _sufixos_ apenas se for criar variáveis que tenham alguma característica em comum. Exemplo: `salario_antes` e `salario_depois`. Se existirem muitas características em comum em um tipo de dados, considere agrupar com _struct_
* Não coloque o _tipo_ no nome da variável. Exemplo: `vetor_de_alunos`.
* Coloque `t_` no início de _typedef_. Exemplo: `typedef int t_meuint;`
* Coloque `st_` no início de _structs_. Exemplo: `struct st_concreto { ... };`
* Coloque `s` no início de variáveis _string_. Exemplo: `char snome[MAX];`
* Coloque `p` no início de ponteiros. Exemplo: `t_carro *pcarro;` ou `char *pnome;`.
* Nomeie uma _enum_ (enumeração) pelo substantivo comum do objeto que ela enumera.

### Organização

A ordem que o esqueleto do programa deve ter é como segue:

#### Em linguagem C

* Comentário com _copyright_, descrição do programa, autor, contato e data
* Comentários com uma introdução à leitura do código, explicações gerais, compilação, etc.
* O(s) `#include <...>`
* O(s) `#include "..."`
* Se o programa **não** tem uma biblioteca própria, seguem:
    - Os _defines_
    - Os TAD's (tipos abstratos de dados): _struct_, _enum_, _typedef_, etc.
    - Os tipos globais
    - Os protótipos das funções
* Se o programa tem biblioteca própria, estes ficam no `.h`
* A função `int main(void)` ou `int main(int argc, char *argv[])`
* As outras funções, de preferência em uma ordem que faça um mínimo de sentido com o próprio fluxo do programa

# Ciclo de trabalho

* Ligou o computador, **primeira** vez neste repositório
    - Tudo configurado no GitHub e na sua máquina local?
    - Repositório criado no GitHub com _fork_
    - Faça o _clone_ na sua máquina
    - Crie seu ramo _feature-nome_ ou vá para _develop_
    - Siga a partir do passo (1)
* Ligou o computador para trabalhar do **segundo** dia em diante
    - Atualize todos os ramos remotos (_master_ e _develop_)
    - Resolva conflitos se houver
    - Vá para seu ramo de trabalho _feature-nome_ ou _develop_ 
    - (1) Edite o arquivo fonte no _vi_
    - (2) Compile (_gcc_ ou _make_, ou teste no _swipl_)
    - (3) Erros: volte para passo (1)
    - (4) Compilou sem erros: faça _commit_
    - (5) Vai trabalhar mais, volte para o passo (1)
    - (6) Fim do dia de trabalho:
        * Vá para o _develop_ e atualize-o
        * Faça _merge_ do _feature-nome_ com o _develop_
        * Resolva conflitos se houver
        * Teste o programa muitas vezes e corrija se necessário
        * Faça o _push_ do _develop_ para o GitHub
        * Ou, se usou apenas o _develop_, faça o _push_
    - (7) Dia de finalizar o _master_ e pedir um _pull request_
        * Vá para o _master_ e atualize-o
        * Faça _merge_ do _develop_ com o _master_
        * Resolva conflitos se houver
        * Teste o programa muitas vezes e corrija se necessário
        * Faça o _push_ do _master_ para o GitHub
        * Tudo pronto, faça o _pull request_ e torça para ser aceito

# Teste

* Não tenha pressa de fazer _push_! O _push_ é irreversível, manda para o GitHub. Enquanto estiver só na sua máquina você pode corrigir quaisquer _bugs_
* Compile. Use todas as chaves de aviso que o _gcc_ pode oferecer. O _gcc_ é seu amigo!
* Crie páginas com mensagens de erro com o comando _sprunge_ para discutir nos _issues_ se necessário
* Use _makefile_ para compilar seus testes
    - Não deixe de conhecer as opções do _gcc_ para compilação. Este é um comando fundamental em vários sistemas operacionais, inclusive embarcados (arduino, pic, raspberry pi, etc.)
* Teste! Rode o programa, faça entrada de dados, veja se o que acabou de programar realmente faz o que você deseja, e se não quebrou nada em outra função do programa.
* Tudo ok? Então faça o _push_!
* Faça suas modificações chegarem até o _master_ e então faça o _pull request_ do seu _master_ (_origin_) para o _master_ do _upstream_.
* No _pull request_ coloque um título descritivo. 

# Faça uma boa mensagem de commit

* Se é um pequeno _commit_ tudo bem abreviar a mensagem com apenas um título com o comando `git commit -am "uma descricao do que este commit faz em ate 50 caracteres"`
* Mas se é um _commit_ que vai influenciar muito o código de outros _ramos_ ou que vai ser usado para _pull request_ então é importante escrever uma boa mensagem. Neste caso, **não** use o comando abreviado. Use os comandos:
    - `git add arquivo` : se necessário adicionar algum arquivo
    - `git commit` : sem abreviação e sem mensagem. Este comando vai abrir o _vi_ (ou seu editor preferido) para você digitar uma mensagem de _commit_ maior.
        * Na primeira linha, escreva um título do _commit_ com até 50 caracteres, como você já está acostumado.
        * Pule uma linha.
        * Da terceira linha para baixo, descreva com mais detalhes tudo o que você fez, o que este _commit_ inclui, o que muda, qual funcionalidade é acrescentada ou _bug_ é resolvido.
        * Saia com `:x` (no _vi_) para salvar e fazer o _commit_
        * Saia com `:q!` (no _vi_) para abortar a edição e **não** fazer o _commit_
        * Linhas iniciadas com \# são ignoradas pelo _git_ como sendo comentários e não são escritas no _commit_.
* Exemplo de uma boa mensagem de _commit_:

```
Faz um exemplo para demonstrar a clareza do commit

Sem este commit a explicação sobre mensagens de commit ficaria falha, sem um exemplo concreto. 
Este é um problema, porque assim fica apenas na imaginação o que seria um bom commit. 
Este commit corrige este problema ao fazer aqui um exemplo concreto e imperativo.

A primeira linha é um título imperativo descritivo do essencial. Os dois parágrafos seguintes, 
o de cima e este, são explicações mais aprofundadas. É importante descrever (1) _o que ocorria_ 
antes do commit, (2) _qual_ é o problema que este commit resolve, (3) _porque_ este comportamento 
é problemático, e (3) _como_ este commit corrige este problema.

Também é possível usar enumerações para clarificar algumas mudanças em pontos 
específicos, como:

* mudafunc() : agora não retorna mais void e sim o código do cliente
* funcoutra() : criada para cumprir outra finalidade que estava faltando
* divideaqui() : criada para cumprir uma finalidade tal e tal que antes 
estava sendo feita na função mudafunc() de modo a ficar mais clara a divisão do que cada função faz.
```

Lembre que a primeira linha é importante para ver o _log_ e acompanhar a evolução do programa.

* Referência: [A Note About Git Commit Messages](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html)

# Dicas de configuração

## Final de linha estilo Unix (`\n`)

* Para o final de linha `\n`, use em suas configurações
    - Linux, Mac ou Unix: `git configure --global core.autocrlf input`
    - Windows: `git configure --global core.autocrlf true`
* Outra opção para configurar `\n` é por repositório. Inclua um novo arquivo _.gitattributes_ com os comandos:


```
# Arquivo .gitattributes

* text=auto
*.c text
*.h text
*.txt text
*.ini text
*.eng text
*.x binary
*.jpg binary
*.png binary
```

* Veja detalhes de como atualizar o repositório após alterar estas configurações na [página de help do GitHub](https://help.github.com/articles/dealing-with-line-endings/#platform-all)
* Veja também [configurando o Git](https://git-scm.com/book/en/v2/Customizing-Git-Git-Configuration)

# Obrigado!

Agora é com você! Divirta-se programando!

Atenciosamente,

Prof. Dr. Ruben Carlo Benante \<<rcb@beco.cc>\>
Autor do Xadreco

