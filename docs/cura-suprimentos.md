# Cura e suprimentos

## Uso
Cada item Suprimentos funciona como um kit de uso único: recupera até40 de vida ao concluir4 segundos, com limite100. Mesmo faltando somente10 de vida, um kit completo é consumido; não existem cargas parciais neste bloco. Com vida cheia, a tentativa é recusada sem consumo.

Na partida: TAB abre o inventário, setas selecionam o Suprimento da mochila ou safe e H começa a cura. O inventário fecha ao aceitar. O HUD mostra tempo e progresso; H novamente cancela. Sem inventário aberto, H não escolhe automaticamente outro item.

Fique parado e no chão. Movimento/pulo, dano aplicado, troca de equipamento, mudança/descarte/perda do item usado, início de extração, morte, desconexão ou retorno à base interrompem antes da conclusão. Cancelar não consome o kit nem aplica cura parcial; o item depois segue suas regras normais de propriedade/morte/safe. Abrir o inventário ou mover apenas a câmera não cancela por si só. Tiro e recarga ficam bloqueados durante o tratamento e não disparam automaticamente depois. Não iniciar durante recarga nem contagem de extração.

## Persistência e autoridade
Cliente solicita o GUID; servidor decide validade, duração, interrupção, consumo e vida. UMarau6HealingComponent mantém estado e tempo, Marau6ItemLedger.ConsumeForHealing grava consumo antes de aplicar vida e antes de sincronizar callbacks de inventário. Falha de gravação preserva item e vida, interrompe a tentativa e mantém o bloqueio de erro já existente.

Consumido é uma custódia terminal: GUID permanece registrado, sem dono/slot utilizável, mas sai do inventário. Backend só permite Suprimento já pertencente ao inventário de um participante ativo; rejeita identidade nova já consumida, tipo incorreto, item do mundo ou resultado finalizado. Não permite omitir/restaurar o registro nem reutilizar seu GUID em outra rodada. Extração, morte, abandono e reinício não devolvem kits consumidos, inclusive os usados a partir do safe.

Não mudou a numeração dos tipos de item nem a estrutura SQL. Não foi preciso apagar ou recriar perfis. Vida continua sendo estado da partida; não há cura na base ou saúde persistente entre partidas. Animações médicas, sons próprios, múltiplos medicamentos, sangramento, cura de terceiros e cargas parciais ficam fora deste marco.

## Teste manual
1. Use Docker/Iniciar-Marau6.cmd como antes e reabra a Unreal para carregar o novo componente.
2. Coloque Suprimentos na mochila ou safe, entre na partida e receba dano.
3. Procure cobertura, pare, abra TAB, selecione o item e pressione H. Confira vida e remoção do kit somente após4 segundos.
4. Repita cancelando com H, movimento ou dano recebido; confira que o item não foi consumido e a vida não aumentou.
5. Confira recusa com vida cheia e bloqueio de tiro/recarga durante uso. Extraia ou morra, volte à base e verifique os suprimentos restantes.

## Verificação automática e entrega
Compilação Marau6Editor concluída.25 testes Unreal aprovados (incluindo os21 anteriores),20 verificações de contrato do backend e reinício aprovados. Dois clientes PIE reais validaram dano/interrupção por bot, uso no safe e mochila, bloqueio de disparo, cura40/max100 vista pelo outro cliente, rejeição de reuso/GUID alheio, extração e morte; após reiniciar a API, dois kits usados continuaram consumidos e o safe não usado do segundo perfil foi preservado.

Regressões de bots com dois clientes, loot persistente após reinício e ciclo anterior de persistência/recuperação também passaram. Revisão independente sem bloqueios remanescentes; instrução visual duplicada foi removida. API local atualizada e health confirmado; assinatura do estado persistente public.marau6_state idêntica antes e depois da inicialização, sem reset de dados.

Desenho aprovado pelo usuário em31/10/2026. Validação manual do novo bloco confirmada pelo usuário. PR e merge autorizados para fechar este marco na main.