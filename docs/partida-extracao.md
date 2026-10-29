# Bloco: partida e extração

O servidor controla os estados Active, Extracting, Extracted e Eliminated. Um snapshot replicado contém o estado e o instante final da contagem. O HUD calcula o tempo restante com o relógio sincronizado do GameState.

A zona verde está no piso em (1400,0,0) do mapa Lvl_FirstPerson. A cápsula inteira deve caber no volume. Permanecer 5 segundos conclui; sair reinicia, trocar de zona reinicia, morrer elimina. Uma zona removida ou a perda do controlador cancela a contagem. Combate e recarga continuam permitidos durante a contagem.

Concluir só pode ocorrer no servidor após revalidar presença e prazo. Não existe RPC de concluir extração. Após extrair, o personagem fica oculto, sem colisão e sem movimento/tiro/recarga/dano; a recarga pendente é cancelada. A câmera e a tela de resultado permanecem. Cada jogador tem resultado independente; o outro continua jogando.

Esta sessão de teste termina ao parar o Play. Ainda não há reinício automático, duração global, retorno à base, inventário ou persistência. Não há garantia de loot nesta etapa porque os itens ainda não foram implementados. Cinco segundos e dimensões da zona são provisórios.

## Validação

Development Editor / Win64 compilado e sete testes Marau6 aprovados. Extraction cobre prazo, saída/reentrada, combate durante contagem, extração terminal, bloqueios após extrair, morte durante contagem, independência entre jogadores, zona removida e cancelamento real de recarga antes de transferir munição. Os testes usam mundo autoritativo local; recepção do snapshot e contagem entre clientes devem ser verificadas em PIE.

## Teste integrado com dois clientes

1. Use Number of Players 2, Play As Client e New Editor Window (PIE).
2. Encontre o quadrado verde no piso junto à lateral da arena. Entre, veja a contagem e saia antes do fim: deve voltar ao estado ativo. Reentre: deve começar de 5.
3. Fique até concluir. Deve aparecer EXTRAIDO COM SUCESSO; personagem desaparece para o outro, deixa de bloquear/receber tiros e não anda/atira/recarrega. O outro continua ativo.
4. Reinicie o Play. Durante a contagem de um jogador, elimine-o com o outro: resultado ELIMINADO, sem extração posterior.
5. Reinicie e confira uma recarga iniciada perto do fim da contagem: extrair cancela a transferência. Confira também combate, som e efeitos fora da área.

O comando exclusivo do editor Marau6.PreviewExtraction permite conferir o HUD local com tempo desacelerado, sem salvar configuração. Não é usado no jogo normal.

## Ajuste do ambiente

Fab desativado somente em Marau6.uproject após erro FabToolset na instalação local. O teste de compilação confirmou que desativá-lo resolve o bloqueio. A engine e os assets originais permanecem intactos. O catálogo Fab pode ser reativado futuramente após corrigir sua instalação.

Validação manual integrada confirmada pelo usuário em 29/10/2026 com dois jogadores.
