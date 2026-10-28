# Arma visual provisória

O personagem começa com o SKM_Rifle do template equipado. Dois componentes sem colisão usam o socket HandGrip_R: FirstPersonRifle é visível somente ao dono; ThirdPersonRifle é oculto ao dono e visível aos demais. O combate continua usando o hitscan do servidor, sem reutilizar AShooterWeapon.

As poses são carregadas em BeginPlay para evitar dependência circular durante a construção dos defaults. Os ossos continuam atualizados quando o corpo está oculto. O rifle não adiciona dano, munição ou RPCs.

## Poses do projeto

Content/Marau6/Anims contém ABP_Marau6_FP_Rifle e ABP_Marau6_TP_Rifle, cópias adaptadas das poses originais. As adaptações usam AMarau6Character, GetBaseAimRotation, validade do pawn remoto e PitchN ligado ao eixo Y do aim offset. Os assets originais permanecem intactos.

O comando exclusivo do editor Marau6.SetupWeaponVisuals permite reproduzir essas adaptações. Os assets prontos já fazem parte do projeto, portanto não é necessário executá-lo para jogar. Marau6.PreviewWeapon cria uma prévia local temporária para inspeção; não salva o mapa.

## Verificação em 28/10/2026

- Development Editor / Win64 compilado com sucesso.
- Marau6.Combat: Health, ServerFire, AmmoReload e WeaponVisuals aprovados.
- WeaponVisuals verifica componentes, socket, modelo, visibilidade, ausência de colisão e avaliação das poses sem referências nulas. Também confirma mudança da pose com mira vertical em um pawn sem controlador.
- Prévia gráfica local conferida: arma em primeira pessoa e personagem à frente segurando rifle com mira elevada.
- O teste automatizado usa um mundo local; arma e mira entre dois clientes também foram confirmadas pelo usuário.

## Teste com dois clientes

Abra Lvl_FirstPerson. Use Number of Players 2, Play As Client e New Editor Window (PIE). Confira o rifle na própria janela e nas mãos do outro personagem. Em uma janela, mire para cima e para baixo; observe a arma pela outra. Confira também movimento e salto. Repita disparo e recarga: dano 25, carregador 10, reserva 30 e recarga 2 segundos.

Ainda não há efeitos novos, animação de disparo ou animação de recarga. O visual é provisório.

Validação manual confirmada pelo usuário em 28/10/2026, com dois jogadores.
