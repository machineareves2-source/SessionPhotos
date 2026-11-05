# Session Photos 0.1 — première version de test

Plugin indépendant de documentation de session pour Benjamin Mandeau.
Code et interface originaux ; aucun code ni élément graphique de Big Notes n'est utilisé.

## État de cette livraison

Ce paquet est le **projet source**, pas un installateur Windows. Le VST3 Windows doit être compilé avec Visual Studio ou le workflow GitHub Actions fourni. Les validations réalisées sont consignées dans VALIDATION.md. La validation réelle dans REAPER Windows et sur le Samsung reste nécessaire.

## Utilisation après compilation

1. Copier le dossier complet **Session Photos.vst3** dans `C:\Program Files\Common Files\VST3\` (ou un dossier VST3 déjà scanné par REAPER).
2. Dans REAPER, relancer la recherche des plugins si nécessaire, puis insérer **Session Photos** sur une piste. Le plugin ne modifie pas les échantillons audio.
3. Dans la colonne de droite, choisir l'adresse du PC sur le réseau du studio et cliquer sur **Activer le transfert**.
4. Si Windows affiche une demande de pare-feu pour REAPER, autoriser le réseau **privé** du studio.
5. Connecter le téléphone au même réseau local, scanner le QR code avec l'appareil photo, puis ouvrir le lien dans Chrome/Samsung Internet.
6. Choisir **Prendre une photo** ou **Galerie**. La page affiche la confirmation de réception. Pas d'application Android à installer.
7. Dans le plugin, sélectionner une photo pour modifier son titre et ses notes. Molette ou boutons +/− pour zoomer ; glisser pour déplacer ; **Tourner** par pas de 90° ; **Ajuster** pour recentrer.
8. **Enregistrer le projet dans REAPER (Ctrl+S).** Les images reçues et les notes sont incluses dans l'état du plugin enregistré par le DAW.

Fermer le navigateur n'enregistre pas le projet. L'accusé de réception signifie que le plugin détient la photo en mémoire, pas que le projet est déjà enregistré sur disque.

## Conservation des images

- Les octets de chaque image reçue sont embarqués dans la session. Aucun chemin de fichier externe n'est nécessaire pour la rouvrir.
- Depuis le téléphone, la copie envoyée est convertie en JPEG qualité 90 %, redimensionnée à 800 (défaut), 1 200, 1 600, 2 400 ou 3 600 pixels de côté maximal. L'original du téléphone reste intact. C'est **la copie envoyée** qui est embarquée, pas la photo originale pleine résolution.
- L'import depuis le PC conserve les octets JPEG/PNG d'origine. L'orientation EXIF des imports directs n'est pas appliquée automatiquement dans cette version : utiliser **Tourner** si nécessaire.
- Maximum **64 photos et 64 Mio de données d'image par instance**, **8 Mio / 24 mégapixels par photo reçue**. Les projets REAPER peuvent devenir plus gros que ce total à cause de leur encodage.
- Les notes (16 000 caractères par image), titres (160 caractères) et rotations sont conservés. Le zoom/pan est un réglage temporaire de consultation.
- Une session transférée à un autre PC nécessite également l'installation du plugin.

## Connexion

Le PC peut être en Ethernet et le téléphone en Wi-Fi, à condition qu'ils se voient sur le réseau. Un Wi-Fi invité avec isolation des clients empêche le transfert.

Le serveur démarre uniquement sur demande et sur l'adresse privée choisie (10.x, 192.168.x ou 172.16–31.x). Chaque instance et chaque nouvelle activation possède un lien aléatoire distinct. Le lien n'est pas sauvegardé dans le projet. Le service reste actif si seule la fenêtre du plugin est fermée ; **Fermer le transfert** ou retirer le plugin l'arrête.

Le transfert utilise HTTP sur le réseau local, sans chiffrement TLS ; utiliser le réseau de confiance du studio. Aucune redirection de port Internet n'est nécessaire. Pas de serveur cloud, compte, suivi ou ressources web externes.

Si plusieurs adresses apparaissent, éviter les cartes VPN/virtuelles et sélectionner l'adresse de la carte reliée au studio. Si la page ne s'ouvre pas : vérifier le Wi-Fi, le pare-feu, l'adresse choisie, puis fermer/réactiver le transfert et rescanner le QR.

## Test conseillé dans REAPER

Avant d'utiliser cette version pour un recall important : créer un projet d'essai, envoyer deux photos depuis le Samsung, ajouter des notes, sauvegarder, fermer complètement REAPER et rouvrir le projet. Vérifier ensuite la lisibilité des inscriptions avec le zoom et la présence des notes. Essayer deux instances simultanées pour confirmer la bonne destination des photos.

## Compilation

Dépendances : JUCE 8.0.6, cpp-httplib 0.18.3, QR-Code-generator 1.8.0. CMake télécharge ces versions au premier lancement. Le code original de ce projet est fourni sous AGPL-3.0-or-later ; voir LICENSE et THIRD_PARTY.md.

Sous Windows : Visual Studio 2022 avec « Développement Desktop en C++ », CMake et Git.

```sh
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Résultat : `build/SessionPhotos_artefacts/Release/VST3/Session Photos.vst3`.

Le workflow `.github/workflows/build.yml` réalise la compilation Windows et les tests, puis fournit le VST3 dans les artefacts. Ce projet n'est pas publié automatiquement sur un compte GitHub.

## Architecture et garanties vérifiables

Le callback audio est un passage direct sans traitement, verrou, allocation, décodage d'image ou réseau. Les requêtes réseau utilisent deux workers et une file d'attente bornée. Les modifications déclenchent une notification de changement d'état au DAW depuis le thread d'interface.

La sauvegarde sérialise un instantané cohérent ; la restauration valide entièrement un état temporaire avant de remplacer les photos existantes. Un état tronqué ou invalide est rejeté, sans remplacement partiel. Les dimensions d'images sont contrôlées avant décodage. Les uploads sont limités en taille. Aucune API réseau de suppression ou d'édition des notes n'est exposée dans cette version ; les notes se modifient sur le PC.

La page téléphone est une interface web locale intégrée au binaire, pas une application Play Store ni une PWA hors ligne.

### Choisir une instance depuis le téléphone

Le menu « Envoyer vers » liste les réceptions actives sur la même adresse réseau et dans le même processus du DAW. La liste est actualisée toutes les 5 secondes. Le nom inclut le titre de la première photo pour identifier la destination. Les instances isolées dans des processus séparés nécessitent leur propre QR code. Le changement est bloqué pendant un envoi.
