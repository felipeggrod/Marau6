# Recarga, corrida, rifle inicial e contador

Pedido em 03/11/2026.

## Rifle inicial

Ao entrar na partida com o slot de arma vazio, o servidor entrega um rifle novo (20/40, 100%) via `UMarau6ItemLedger::GrantStarterRifle`. O backend só aceita itens novos nascidos no mundo, então o rifle é gravado primeiro em recuperação e depois equipado numa segunda gravação. Se a segunda falhar, ele aparece no chão aos pés do jogador. Quem entra armado mantém a própria arma. Teste: `Marau6.Inventory.StarterRifle`.

## Contador de munição

Canto inferior direito: `carregador / reserva`, vermelho com carregador vazio e "RECARREGANDO" acima durante a recarga.

## Animações

- Corrida: `BS_Marau6_Locomotion` é uma cópia de `BS_Idle_Walk_Run` com o jog em 5 m/s (o template só chegava ao jog em 6 m/s; a 5 m/s parecia caminhada). Mirando (3 m/s) ainda caminha; a corrida com Shift continua no jog.
- Recarga: `MM_Rifle_Reload` toca no ramo superior do corpo (`DefaultSlot`), acelerada para caber nos 2 s; as pernas continuam a locomoção. `bReloadPose` replica para todos, pois o estado de munição só vai ao dono. A primeira pessoa copia a pose do corpo, então a mesma montagem aparece nas duas visões. O carregador do lasgun não se move.

As duas mudanças de asset são feitas por `Marau6.SetupLocomotion`, com o editor fechado e depois de compilar:

`UnrealEditor-Cmd.exe Marau6.uproject -ExecCmds="Marau6.SetupLocomotion,QUIT_EDITOR" -unattended -nullrhi`

O log deve mostrar `MARAU6_ANIM body saved=1 locomotion=1 slots=1`. Assets do template não são alterados. Teste: `Marau6.Combat.LocomotionAndReload` (falha até o comando rodar).
