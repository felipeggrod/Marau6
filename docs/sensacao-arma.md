# Sensação da arma

Cadência, recuo, corrida, mira, movimento da arma em primeira pessoa e efeitos do disparo. Tudo o que é apresentação roda só nos clientes; regras de tiro e velocidade continuam no servidor.

## Regras

- Cadência de 450 disparos por minuto (0,133 s), validada pelo servidor.
- Recuo: cada disparo aceito localmente sobe a mira 0,55° e desvia até ±0,2° para os lados (30% menos mirando). O servidor dispara pela rotação de controle replicada, então o recuo move a mira real e o jogador compensa puxando para baixo. Antes de cada disparo o cliente envia os movimentos pendentes (`FlushServerMoves`), para o servidor usar a mira e o estado de corrida atuais.
- Shift corre a 1,6× apenas andando para frente e no chão. Não dispara nem mira correndo; o servidor recusa tiros durante a corrida. Correr cancela a mira; recarregar interrompe a corrida. Depois da corrida o disparo volta quando a arma já subiu.
- Botão direito segurado mira: 0,6× de velocidade, zoom de 20% e sensibilidade do mouse proporcional ao zoom. A mira fica indisponível durante a recarga e com o inventário aberto.
- `UMarau6MovementComponent` leva os desejos de correr e mirar em cada movimento salvo (flags `FLAG_Custom_0` e `FLAG_Custom_1`), de modo que o servidor aplica a mesma velocidade prevista pelo cliente.

## Arma em primeira pessoa

Os braços só se deslocam no espaço da câmera: `FirstPersonArmsOffset`, `FirstPersonAimOffset`, `SprintArmsOffset`, balanço ao andar e recuo para trás. A rotação passa pela câmera inativa `ViewModelAim`: o control rig do template aponta o rifle ao longo dela e mantém as duas mãos na arma, então girá-la inclina a arma sem brigar com o rig. A pose de corrida é `SprintAimRotation`; o atraso ao virar e a subida do cano no disparo também giram essa câmera. Os valores ficam nos defaults do personagem e foram calibrados com capturas; a mira alinha a alça e a massa do SKM_Rifle ao centro da tela. Outro modelo de arma exige recalibrar `FirstPersonAimOffset`.

## Efeitos do disparo

`AMarau6ShotFX` complementa `AMarau6ShotEffect` em cada disparo aceito, de jogadores e de bots:

- Clarão em três lâminas aditivas na boca do cano e uma luz breve.
- Camada de som do rifle (`S_RifleShot`) sobre o som atual: sem posição para o dono, espacial para os outros.
- Traçante a 500 m/s e rastro de fumaça que alarga, sobe e some em 1,4 s. Para o dono, o rastro começa na boca do cano desenhada (escala de primeira pessoa).
- No impacto em superfície: poeira, faíscas e marca de bala por 20 s. Em personagem: névoa e gotas de sangue e manchas na parede atrás e no chão abaixo, só em geometria estática, por 30 s.

Os materiais (`M_MuzzleFlash`, `M_Tracer`, `M_ShotSmoke`, `M_ImpactPuff`, `M_BulletHole`, `M_BloodDecal`) usam só nós matemáticos do motor. O som é sintetizado por `scripts/Generate-RifleShot.py`. Para regenerar:

1. `python -I scripts/Generate-RifleShot.py Saved/SourceAudio/S_RifleShot.wav`
2. Com o editor fechado: `UnrealEditor-Cmd.exe Marau6.uproject -ExecCmds="Marau6.SetupShotFX" -unattended -nullrhi`, encerrando após `S_RifleShot saved=1` no log.

Na primeira execução os shaders novos compilam em segundo plano; os primeiros disparos podem aparecer sem efeito.

## Morte

Jogadores e bots viram ragdoll (`Marau6Ragdoll`): a cápsula deixa de bloquear em todas as máquinas e só os clientes simulam o corpo. O jogador é empurrado na direção do último tiro (`LastHitDirection`, replicado). Uma nova rodada revive o mesmo pawn e devolve o corpo à cápsula.

## Inspeção

- `Marau6.CaptureViewModel` salva capturas em `LocalSettings/viewmodel_*.png`. Use um mapa fora da base persistente, onde o personagem já começa armado: `/Game/Variant_Shooter/Lvl_Shooter?game=/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode.BP_FirstPersonGameMode_C`. `LocalSettings/viewmodel_shots.txt` pode trocar a lista: uma linha `nome;pitch;comando|comando;espera`.
- `Marau6.ViewModelPose 1|2` força a pose de mira ou de corrida; `Marau6.ShotFXAge` congela os efeitos numa idade; `Marau6.PreviewShotFX` mostra um disparo do dono sem tiro no servidor.
- Testes: `Marau6.Combat.SprintMovement` e `Marau6.Combat.Ragdoll`.
