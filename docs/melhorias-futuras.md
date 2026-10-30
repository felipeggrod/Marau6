# Melhorias futuras do Marau6

Esta lista reúne pedidos do usuário, ideias, observações e bugs adiados. Registrar uma entrada não autoriza sua implementação nem define a ordem do trabalho. Antes de implementar, revisar escopo, prioridade e critérios de teste. Sugestões do assistente devem ser identificadas como sugestões e não como decisões aprovadas.

## FUT-001 — Queda e apoio dos itens descartados

- **Origem:** relato do usuário durante o teste integrado de inventário e loot, em 29/10/2026.
- **Tipo:** comportamento observado / melhoria candidata.
- **Estado:** adiado por decisão explícita do usuário; manter o comportamento atual.
- **Observação:** ao descartar vários itens parado, sem mudar a mira, eles podem se empilhar. Itens superiores podem encobrir os inferiores; em certos ângulos é possível coletar o item do meio ou da base, deixando o superior flutuando. Em um teste, o quarto descarte apareceu à frente da pilha.
- **Possível função futura:** física e gravidade para os itens caírem e se acomodarem, inclusive quando o objeto que os sustenta for coletado.
- **Decisão pendente:** física não é requisito aprovado nem obrigatório para a versão final. Avaliar novamente conforme a interação desejada para loot. Distribuir descartes em posições livres ou usar contêineres também são alternativas, não decisões aprovadas.
- **Cuidados se implementada:** servidor determina o estado dos itens; manter identidade única, custódia e munição; sincronizar posição entre clientes; avaliar repouso dos objetos e custo de processamento. Esses pontos são recomendações técnicas do assistente.
- **Critérios sugeridos de validação:** descartes consecutivos, retirada do apoio de uma pilha, rampas e bordas, coleta por dois jogadores e ausência de duplicação ou perda de itens.

## Como manter esta lista

Novas entradas devem ter identificador, origem (usuário ou assistente), tipo, descrição, estado e decisão pendente. Usar estados como sugerido, adiado, aprovado para planejamento, em implementação e concluído. Pedidos adiados não devem ser retomados automaticamente. Funcionalidades já previstas no projeto não precisam ser duplicadas aqui, salvo quando houver uma observação nova a registrar.
