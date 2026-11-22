# Roteiro de Apresentação — Trabalho de Grafos (Parte 3)

**Autores:** Arthur Sampaio e Carlos Bruno

**Disciplina:** COS 242 — Teoria dos Grafos (PESC/COPPE/UFRJ)

**Tempo total estimado:** 7 a 8 minutos (9 slides)

## Slide 1 — Capa (0:00 – 0:20)

* Olá, professor. Eu (Arthur Sampaio) e o Carlos Bruno vamos apresentar agora a Parte 3 do nosso trabalho de Teoria dos Grafos, cobrindo a extensão da nossa biblioteca em C++17 para grafos direcionados com pesos arbitrários, os algoritmos de Bellman-Ford e Floyd-Warshall, nossas otimizações e os resultados dos estudos de caso.

## Slide 2 — Extensão para Grafos Direcionados no CSR (0:20 – 1:15)

* Partindo da estrutura `CSRGraph` consolidada na Parte 2, adicionamos o suporte completo a grafos direcionados através da flag `directed_` e do método `isDirected()`.

* Quando `directed = true`, cada linha `u v w` do arquivo de entrada grava apenas o arco orientado $u \to v$ na lista de saída de $u$, sem inserir o arco reverso. Além de preservar a orientação correta, isso reduz o tamanho dos vetores `edges_` e `weights_` de $2m$ para $m$, consumindo apenas $8n + 8m + 16$ bytes.

* Como nossos algoritmos anteriores — **BFS, DFS e Dijkstra** — foram construídos iterando exclusivamente sobre a lista de saída do CSR, todos eles funcionam nativamente em grafos direcionados sem alteração de lógica, e incluímos testes unitários específicos para os três no CTest com `assert` ativo mesmo em `Release`.

## Slide 3 — Bellman-Ford: Grafo Transposto e Otimizações (1:15 – 2:30)

* O enunciado pede que o Bellman-Ford encontre as distâncias e caminhos mínimos de **todos os vértices para um vértice específico**, e no Item 1 pede as distâncias de `10`, `20` e `30` até o destino `100`.

* Em vez de rodar o algoritmo três vezes independentes (uma para cada origem), implementamos o **grafo transposto $G^T$** em tempo linear $\mathcal{O}(V+E)$, invertendo todos os arcos. Com isso, uma **única execução** de Bellman-Ford partindo do vértice `100` sobre $G^T$ resolve simultaneamente todos os caminhos até `100`, reduzindo o custo da consulta em 3 vezes.

* Também implementamos as otimizações de Bellman-Ford:
  1. **Parada antecipada** na versão padrão, interrompendo as $V-1$ iterações assim que uma passada inteira não relaxa nenhuma aresta;
  2. **Fila de vértices ativos (SPFA)**, relaxando apenas os arcos que saem de vértices cuja distância acabou de diminuir, com vetor `in_queue` para evitar duplicatas;
  3. E na **detecção de ciclo negativo**, em vez de esperar $V$ reinserções na fila (o que custaria milhões de voltas se um ciclo negativo pequeno existisse num grafo gigante), detectamos o ciclo diretamente pela cadeia de predecessores.

## Slide 4 — Floyd-Warshall: Otimização de Memória (2:30 – 3:30)

* No algoritmo de Floyd-Warshall, aplicamos a otimização clássica de memória discutida em aula: substituímos a tabela 3D $D^{(k)}[i][j]$ de espaço $\Theta(V^3)$ por uma atualização *in-place* sobre uma única matriz 2D de espaço $\Theta(V^2)$.

* Além disso, fizemos três otimizações práticas em C++:
  * Achatamos as matrizes `dist` e `next` (usada para reconstruir os caminhos mínimos) em dois vetores unidimensionais contíguos de tamanho $(V+1)^2$, favorecendo o uso da cache da CPU;
  * Armazenamos `dist` em `float` (4 bytes) e `next` em `int` (4 bytes), reduzindo o consumo para **8 bytes por entrada** ($33\%$ a menos que usando `double`);
  * Colocamos uma poda no laço intermediário (`if (dist[i][k] == INF) continue`) para pular o laço interno sobre $j$ em pares inalcançáveis. A detecção de ciclo negativo ocorre ao final checando se existe algum `dist[v][v] < 0` na diagonal principal.

## Slide 5 — Estudo de Caso (Item 1): Distâncias até o Vértice 100 (3:30 – 4:25)

* Aqui temos os resultados do Item 1 para os cinco grafos da Parte 3, indo de 25 mil vértices (`W_1` e `W_2`) até 10 milhões de vértices (`W_5`).

* O primeiro resultado marcante está no **`grafo_W_1`**: ele é o único grafo do conjunto com arestas de peso negativo, e nossa busca em $G^T$ identificou um **ciclo negativo alcançável** para `10`, `20` e `30` até `100` — logo, não existe distância mínima finita bem definida.

* Já em `W_2` a `W_5`, não há pesos negativos e todas as distâncias até 100 convergiram: no `W_2` temos $10{,}47$, $8{,}48$ e $8{,}71$; no `W_3`, $4{,}63$, $5{,}42$ e $4{,}67$; no `W_4`, $9{,}04$, $8{,}83$ e $9{,}23$; e no `W_5`, $37{,}85$, $42{,}63$ e $37{,}86$.

## Slide 6 — Estudo de Caso (Item 2): Floyd-Warshall e Limites (4:25 – 5:25)

* No Item 2, ao tentar executar Floyd-Warshall nos grafos completos, encontramos a barreira física do $\Theta(V^2)$ em RAM e $\Theta(V^3)$ em tempo.

* Mesmo com nossa otimização para $8\text{ bytes} \cdot V^2$, o `W_1` e o `W_2` ($25.000$ vértices) já exigem $\approx 5\text{ GB}$ de RAM e $1{,}56 \times 10^{13}$ operações por rodada (horas por rodada). No `W_3` ($10^5$ vértices) já seriam $80\text{ GB}$ de RAM; no `W_4`, $8\text{ TB}$; e no `W_5`, $800\text{ TB}$ de memória.

* Por isso, executamos o Floyd-Warshall experimentalmente sobre o **subgrafo induzido pelos primeiros 1.000 vértices**, que inclui `{10, 20, 30, 40, 50}` e cabe em 8 MB de RAM.

* No `W_1`, mesmo nesse subgrafo de 1.000 vértices, o Floyd-Warshall detectou o **ciclo negativo alcançável**. Nos cinco grafos, os pares entre `{10, 20, 30, 40, 50}` apareceram como *Inalcançáveis* nesse recorte quando não existiam caminhos que permanecessem dentro dos primeiros 1.000 vértices. Esses resultados não devem ser apresentados como distâncias do grafo completo.

## Slide 7 — Estudo de Caso (Item 3): Tempos Médios em 10 Rodadas (5:25 – 6:15)

* No Item 3, medimos a média de **10 rodadas** sem contar leitura ou escrita de disco.

* O Bellman-Ford com fila de vértices ativos apresentou: **23,6241 milissegundos** no `W_2`, **0,242549 segundos** no `W_3`, **6,69436 segundos** no `W_4` e **22,6695 segundos** no `W_5`, todos em média por execução da fonte medida. W1 não teve benchmark por conter um ciclo negativo alcançável.

* No subgrafo de 1.000 vértices, o Floyd-Warshall rodou entre **1,97 ms e 23,07 ms** por rodada, beneficiado pela poda de estados infinitos na matriz esparsa.

## Slide 8 — Estudo de Caso (Item 4): Todos os Pares — Bellman-Ford vs. Floyd-Warshall (6:15 – 7:20)

* Por fim, no Item 4 comparamos o cálculo de todos os pares rodando Bellman-Ford para todas as $V$ fontes versus Floyd-Warshall.

* Usando o tempo médio real de uma fonte medido em 10 rodadas e multiplicando por $V$, obtemos estimativas para Bellman-Ford todos os pares: **590,603 s** no `W_2`, **24254,9 s** no `W_3`, **6694360 s** no `W_4` e **226695000 s** no `W_5`. Essas linhas são estimativas, não médias executadas. Floyd-Warshall não foi medido no grafo completo; medimos apenas o subgrafo de 1.000 vértices.

* **Respondendo à pergunta do enunciado: quem foi mais eficiente e por quê?**
  * Nos grafos da série $W$, **Bellman-Ford para todos os pares é muito mais eficiente** porque os grafos são **esparsos** (grau médio entre $4$ e $61$, ou seja, $E \ll V^2$). Com a otimização por fila, cada busca explora um custo próximo de linear em $E$, evitando tanto a explosão cúbica $\Theta(V^3)$ quanto o estouro de memória $\Theta(V^2)$ do Floyd-Warshall.
  * Por outro lado, o Floyd-Warshall só vence quando o grafo é **denso** ($E$ próximo de $V^2$) e pequeno o suficiente para caber em memória, cenário em que o Bellman-Ford todos os pares degrada para $\mathcal{O}(V^4)$ enquanto o Floyd-Warshall se mantém em $\Theta(V^3)$ com laços simples e amigos da cache.

## Slide 9 — Conclusões (7:20 – 7:50)

* Em resumo, concluímos a biblioteca com suporte unificado a grafos direcionados em CSR, Bellman-Ford todos-para-um sobre o grafo transposto com detecção rápida de ciclos negativos, e Floyd-Warshall otimizado em memória ($8V^2$ bytes).

* Os experimentos confirmaram na prática como a esparsidade e o limite de memória RAM determinam a escolha entre algoritmos de caminho mínimo para todos os pares em larga escala.

* O código-fonte completo está disponível no GitHub indicado no slide. Muito obrigado, professor! Estamos abertos a perguntas.