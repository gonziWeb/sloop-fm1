# SLOOP 3 songs (version modifiée)

Cette version de SLOOP garde **3 songs** sur le FM-1 au lieu d'une seule. Chaque song a ses propres sections **A–D** et son propre ordre (page SONG).

## Utilisation

Maintiens **SAVE** (le layer « song »). Les touches blanches 9, 10 et 11 deviennent **song 1**, **song 2** et **song 3**. Le song en cours est affiché en blanc.

- Il faut d'abord **arrêter la lecture**. Sinon l'écran affiche `STOP FIRST`.
- Au changement, les sections du song que tu quittes sont d'abord écrites dans la mémoire.
- Ensuite la première section (A, B…) du nouveau song devient la boucle en cours. L'écran affiche `SONG 2`.
- Si le nouveau song est vide, l'écran affiche `NEW SONG 2`. La boucle en cours reste telle quelle, et tu peux la ranger dans ce song avec SAVE + 5…8.
- Le song choisi est gardé quand tu éteins l'appareil.

Tout le reste fonctionne comme avant, mais **dans le song en cours** : SAVE + 1…4 (jouer une section), SAVE + 5…8 (ranger), la chaîne rapide, SONG REC, la page SONG, ERASE.

## Le compromis

La mémoire flash du FM-1 était pleine. Les songs 2 et 3 utilisent la place du **slot de samples USR4**.

- **USR4 reste vide** : tu ne peux plus y charger de samples depuis l'éditeur.
- Le kit **USR3+4** ne joue plus que les sons de USR3.
- USR1, USR2 et USR3 ne changent pas.
- Si tu avais des samples dans USR4, ils seront écrasés dès que tu enregistres dans le song 2. Exporte-les avant.

## À savoir

- Le **song 1** est à l'endroit exact où SLOOP officiel range son song. Si tu reviens à SLOOP officiel, **repasse d'abord sur le song 1**.
- Le **backup de l'éditeur web** sauvegarde le song en cours seulement. Pour garder les 3 songs, fais un backup de chacun : passe sur le song, puis fais le backup.
- Le code n'a été testé que sur ordinateur (tests automatiques), **pas encore sur un vrai FM-1**. Fais un backup complet avant d'installer. Si ça ne démarre plus, maintiens **OCT−** à l'allumage (mode secours) et réinstalle.

## Pour les développeurs

`FELUCCA_SONGS` (1 à 3, défaut 3, dans `firmware/src/felucca.c`). La valeur 1 redonne SLOOP tel quel, avec USR4.

Carte flash :
- song 1 : `0x97000`
- songs 2–3 : `0xE7000` et `0xEF000`, 32 Kio chacun (A/B)
- liste des songs : `0xF7000` / `0xF8000`

Les tests sont dans `tests/songs_test.c` et dans `tests/storage_test.c` compilé avec `-DFELUCCA_SONGS=3`.
