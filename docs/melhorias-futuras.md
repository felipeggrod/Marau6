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

## FUT-002 — Equipamento e comportamento avançado dos bots

- **Origem:** sugestões do assistente no desenho aprovado do bloco de bots básicos, em30/10/2026.
- **Tipo:** evolução candidata.
- **Estado:** adiado no escopo aprovado; não implementar automaticamente.
- **Descrição:** substituir o rifle apenas visual por equipamento individual saqueável, com munição, recarga e transferência de custódia. Avaliar inventário completo de cadáver, audição, cobertura e coordenação tática em blocos posteriores.
- **Decisão pendente:** definir quais recursos entram primeiro e como equilibrar a oferta de equipamentos. A aprovação dos bots básicos não autoriza estas ampliações.
- **Critérios sugeridos:** manter GUID e munição em transferências, uma concessão por morte, autoridade do servidor, reinício sem duplicações e custo de IA compatível com a quantidade de bots desejada.

## FUT-003 — Explosões e dano radial no cenário

- **Origem:** ideia do usuário em01/11/2026 durante o planejamento de destruição.
- **Tipo:** funcionalidade futura.
- **Estado:** sugerido para o futuro; fora do bloco atual.
- **Descrição:** permitir que explosões danifiquem jogadores, bots e várias seções do cenário, com validação do servidor.
- **Decisão pendente:** definir explosivos, alcance, queda de dano com distância e bloqueio por obstáculos. O suporte atual a dano genérico não implementa explosivos nem propagação radial.

## FUT-004 — Materiais que exigem tipos específicos de dano

- **Origem:** ideia do usuário em01/11/2026 durante o planejamento de destruição.
- **Tipo:** funcionalidade futura.
- **Estado:** sugerido para o futuro; fora do bloco atual.
- **Descrição:** paredes ou materiais que só podem ser destruídos por determinados danos ou equipamentos.
- **Decisão pendente:** definir materiais, danos aceitos e comunicação visual. As seções do primeiro bloco aceitarão qualquer dano válido aplicado pelo servidor.
